#include <assert.h>
#include <float.h>
#include <math.h>
#include <platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysctl.h>
#include "bsp.h"
#include "dmac.h"
#include "kpu.h"
#include "printf.h"
#include "nncase.h"

#define LAYER_BURST_SIZE 12

#define KPU_DEBUG 0
#define USE_CACHED_AI_RAM 0

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define ALIGN_UP(x, align) ((x + (align - 1)) & (~(align - 1)))

static int ai_step(void *userdata);
static int kpu_kmodel_done(kpu_model_context_t *ctx);

volatile kpu_config_t *const kpu = (volatile kpu_config_t *)AI_BASE_ADDR;
static volatile uint32_t kpu_status;

typedef struct kpu_context
{
    kpu_task_t kpu_task;
    uint32_t kpu_status;
} kpu_context_t;

volatile kpu_context_t g_kpu_context;

static int kpu_run_all_done(void *_task)
{
    atomic_swap(&g_kpu_context.kpu_status, 0);
    kpu_task_t *task = (kpu_task_t *)_task;
    task->callback(task);
    return 0;
}

int kpu_continue(void *_task)
{
    kpu_task_t *task = (kpu_task_t *)_task;
    int layer_burst_size = 1;

    kpu->interrupt_clear.data = (kpu_config_interrupt_t){
        .calc_done_int = 1,
        .layer_cfg_almost_empty_int = 1,
        .layer_cfg_almost_full_int = 1};

    if(task->remain_layers_length == 0)
    {
        return 0;
    }
    if(task->remain_layers_length <= layer_burst_size)
    {
        for(uint32_t i = 0; i < task->remain_layers_length; i++)
        {
            kpu->layer_argument_fifo = task->remain_layers[i].interrupt_enabe.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].image_addr.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].image_channel_num.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].image_size.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_pool_type_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_load_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_offset.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_calc_type_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].write_back_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].conv_value.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].conv_value2.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].dma_parameter.reg;
        }
        task->remain_layers_length = 0;
    } else
    {
        for(uint32_t i = 0; i < layer_burst_size; i++)
        {
            kpu->layer_argument_fifo = task->remain_layers[i].interrupt_enabe.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].image_addr.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].image_channel_num.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].image_size.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_pool_type_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_load_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_offset.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_calc_type_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].write_back_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].conv_value.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].conv_value2.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].dma_parameter.reg;
        }
        task->remain_layers += layer_burst_size;
        task->remain_layers_length -= layer_burst_size;
    }
    return 0;
}

static int kpu_run_dma_output(uint32_t dma_ch, void *dst, uint32_t length, plic_irq_callback_t cb, void *_task)
{
    sysctl_dma_select(dma_ch, SYSCTL_DMA_SELECT_AI_RX_REQ);
    dmac_irq_register(dma_ch, kpu_run_all_done, _task, 1);
    dmac_set_single_mode(dma_ch, (void *)(&kpu->fifo_data_out), (void *)(dst), DMAC_ADDR_NOCHANGE, DMAC_ADDR_INCREMENT,
                         DMAC_MSIZE_8, DMAC_TRANS_WIDTH_64, (length + 7) / 8);
    return 0;
}

static int kpu_run_dma_input_done_push_layers(void *_task)
{
    kpu_task_t *task = (kpu_task_t *)_task;
    kpu->interrupt_clear.reg = 7;
    dmac->channel[task->dma_ch].intclear = 0xFFFFFFFF;
    kpu->fifo_threshold.data = (kpu_config_fifo_threshold_t){
        .fifo_full_threshold = 10, .fifo_empty_threshold = 1};
    kpu->eight_bit_mode.data = (kpu_config_eight_bit_mode_t){
        .eight_bit_mode = task->eight_bit_mode};

    kpu_layer_argument_t *last_layer = &task->layers[task->layers_length - 1];

    kpu_run_dma_output(task->dma_ch, task->dst, last_layer->dma_parameter.data.dma_total_byte + 1, kpu_run_all_done, task);

    kpu->interrupt_mask.data = (kpu_config_interrupt_t){
        .calc_done_int = 0,
        .layer_cfg_almost_empty_int = 0,
        .layer_cfg_almost_full_int = 1};
    kpu_continue(task);
    return 0;
}

static void kpu_run_dma_input(uint32_t dma_ch, const void *src, plic_irq_callback_t cb, void *_task)
{
    kpu_task_t *task = _task;
    kpu_layer_argument_t *first_layer = &task->layers[0];
    uint64_t input_size = first_layer->kernel_calc_type_cfg.data.channel_switch_addr * 64 * (first_layer->image_channel_num.data.i_ch_num + 1);
    dmac_irq_register(dma_ch, cb, _task, 1);
    dmac_set_single_mode(dma_ch, (void *)src, (void *)(AI_IO_BASE_ADDR), DMAC_ADDR_INCREMENT, DMAC_ADDR_INCREMENT,
                         DMAC_MSIZE_16, DMAC_TRANS_WIDTH_64, input_size / 8);
}

int kpu_run(kpu_task_t *v_task, dmac_channel_number_t dma_ch, const void *src, void *dest, plic_irq_callback_t callback)
{
    if(atomic_cas(&g_kpu_context.kpu_status, 0, 1))
        return -1;

    memcpy((void *)&g_kpu_context.kpu_task, v_task, sizeof(kpu_task_t));
    kpu_task_t *task = (kpu_task_t *)&g_kpu_context.kpu_task;

    kpu_layer_argument_t *last_layer = &task->layers[task->layers_length - 1];

    uint64_t output_size = last_layer->dma_parameter.data.dma_total_byte + 1;

    last_layer->dma_parameter.data.send_data_out = 1;
    last_layer->interrupt_enabe.data.int_en = 1;

    task->dma_ch = dma_ch;
    task->dst = dest;
    task->dst_length = output_size;
    task->callback = callback;
    task->remain_layers_length = task->layers_length;
    task->remain_layers = task->layers;

    plic_set_priority(IRQN_AI_INTERRUPT, 1);
    plic_irq_register(IRQN_AI_INTERRUPT, kpu_continue, task);
    plic_irq_enable(IRQN_AI_INTERRUPT);

    kpu_run_dma_input(dma_ch, src, kpu_run_dma_input_done_push_layers, task);

    return 0;
}

uint8_t *kpu_get_output_buf(kpu_task_t *task)
{
    kpu_layer_argument_t *last_layer = &task->layers[task->layers_length - 1];
    size_t output_size = ((last_layer->dma_parameter.data.dma_total_byte + 1) + 7) / 8 * 8;
    return malloc(output_size);
}

void kpu_release_output_buf(uint8_t *output_buf)
{
    if(output_buf != NULL)
        free(output_buf);
}

static int kpu_done(void *ctx)
{
    atomic_swap(&kpu_status, 0);
    kpu_task_t *task = (kpu_task_t *)ctx;
    task->callback(task->ctx);
    return 0;
}

static int kpu_config_input(void *ctx)
{
    kpu_task_t *task = (kpu_task_t *)ctx;
    kpu->interrupt_clear.reg = 7;
    if(task->remain_layers_length <= LAYER_BURST_SIZE)
    {
        for(uint32_t i = 0; i < task->remain_layers_length; i++)
        {
            kpu->layer_argument_fifo = task->remain_layers[i].interrupt_enabe.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].image_addr.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].image_channel_num.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].image_size.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_pool_type_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_load_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_offset.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_calc_type_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].write_back_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].conv_value.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].conv_value2.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].dma_parameter.reg;
        }
        task->remain_layers_length = 0;
        kpu->interrupt_mask.reg = 7;
    } else
    {
        for(uint32_t i = 0; i < LAYER_BURST_SIZE; i++)
        {
            kpu->layer_argument_fifo = task->remain_layers[i].interrupt_enabe.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].image_addr.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].image_channel_num.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].image_size.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_pool_type_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_load_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_offset.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].kernel_calc_type_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].write_back_cfg.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].conv_value.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].conv_value2.reg;
            kpu->layer_argument_fifo = task->remain_layers[i].dma_parameter.reg;
        }
        task->remain_layers += LAYER_BURST_SIZE;
        task->remain_layers_length -= LAYER_BURST_SIZE;
    }
    return 0;
}

static void kpu_data_output(kpu_task_t *task)
{
    sysctl_dma_select(task->dma_ch, SYSCTL_DMA_SELECT_AI_RX_REQ);
    dmac_irq_register(task->dma_ch, kpu_done, task, 1);
    dmac_set_single_mode(task->dma_ch, (void *)(&kpu->fifo_data_out), (void *)(task->dst), DMAC_ADDR_NOCHANGE, DMAC_ADDR_INCREMENT,
                         DMAC_MSIZE_8, DMAC_TRANS_WIDTH_64, task->dst_length);
}

static int kpu_data_ready(void *ctx)
{
    kpu_task_t *task = (kpu_task_t *)ctx;

    dmac->channel[task->dma_ch].intclear = 0xFFFFFFFF;
    kpu_data_output(task);

    kpu->eight_bit_mode.reg = task->eight_bit_mode;
    kpu->interrupt_mask.reg = 7;
    kpu->interrupt_clear.reg = 7;
    kpu->fifo_threshold.data = (kpu_config_fifo_threshold_t){
        .fifo_full_threshold = 12, .fifo_empty_threshold = 1};

    plic_set_priority(IRQN_AI_INTERRUPT, 2);
    plic_irq_register(IRQN_AI_INTERRUPT, kpu_config_input, task);
    plic_irq_enable(IRQN_AI_INTERRUPT);
    kpu_config_input(task);
    kpu->interrupt_mask.data = (kpu_config_interrupt_t){
        .calc_done_int = 1,
        .layer_cfg_almost_empty_int = 0,
        .layer_cfg_almost_full_int = 1};
    return 0;
}

static void kpu_data_input(kpu_task_t *task)
{
    if(task->src == NULL)
    {
        kpu_data_ready(task);
        return;
    }
    dmac_irq_register(task->dma_ch, kpu_data_ready, task, 1);
    kpu_layer_argument_t *layer = &task->layers[0];
    dmac_set_single_mode(task->dma_ch, (void *)(uintptr_t)task->src, (void *)(uintptr_t)(AI_IO_BASE_ADDR + layer->image_addr.data.image_src_addr * 64), DMAC_ADDR_INCREMENT, DMAC_ADDR_INCREMENT,
                         DMAC_MSIZE_16, DMAC_TRANS_WIDTH_64, task->src_length);
}

int kpu_single_task_init(kpu_task_t *task)
{
    sysctl_clock_enable(SYSCTL_CLOCK_AI);
    kpu_layer_argument_t *first_layer = &task->layers[0];
    kpu_layer_argument_t *last_layer = &task->layers[task->layers_length - 1];

    last_layer->dma_parameter.data.send_data_out = 1;
    last_layer->interrupt_enabe.data.int_en = 1;
    task->src_length = first_layer->kernel_calc_type_cfg.data.channel_switch_addr * 64 * (first_layer->image_channel_num.data.i_ch_num + 1) / 8;
    task->dst_length = ((last_layer->dma_parameter.data.dma_total_byte + 1) + 7) / 8;
    task->dst = (uint64_t *)malloc(task->dst_length * 8);
    memset(task->dst, 0, task->dst_length * 8);
    if(task->dst == NULL)
        return 1;
    return 0;
}

int kpu_single_task_deinit(kpu_task_t *task)
{
    free(task->dst);
    return 0;
}

int kpu_model_load_from_buffer(kpu_task_t *task, uint8_t *buffer, kpu_model_layer_metadata_t **meta)
{
    uintptr_t base_addr = (uintptr_t)buffer;
    kpu_model_header_t *header = (kpu_model_header_t *)buffer;
    kpu_model_layer_metadata_t *layer_meta = (kpu_model_layer_metadata_t *)(base_addr + sizeof(kpu_model_header_t));
    kpu_layer_argument_t *layers = (kpu_layer_argument_t *)(base_addr + header->layers_argument_start);

    if(header->version != 1)
        return -1;
    uint32_t layers_length = header->layers_length;
    task->layers_length = layers_length;
    task->eight_bit_mode = header->flags & 1;
    task->layers = layers;
    task->output_scale = layer_meta[layers_length - 1].output_scale;
    task->output_bias = layer_meta[layers_length - 1].output_bias;
    size_t i;
    for(i = 0; i < layers_length; i++)
    {
        layers[i].kernel_load_cfg.data.para_start_addr = (uint64_t)(base_addr + layer_meta[i].weigths_offset);
        layers[i].kernel_pool_type_cfg.data.bwsx_base_addr = (uint64_t)(base_addr + layer_meta[i].bn_offset);
        layers[i].kernel_calc_type_cfg.data.active_addr = (uint64_t)(base_addr + layer_meta[i].act_offset);
    }

    if(meta)
        *meta = layer_meta;
    return 0;
}

int kpu_start(kpu_task_t *task)
{
    if(atomic_cas(&kpu_status, 0, 1))
        return -1;

    task->remain_layers_length = task->layers_length;
    task->remain_layers = task->layers;
    kpu_data_input(task);
    return 0;
}

static void kpu_send_layer(const kpu_layer_argument_t *layer)
{
    kpu->layer_argument_fifo = layer->interrupt_enabe.reg;
    kpu->layer_argument_fifo = layer->image_addr.reg;
    kpu->layer_argument_fifo = layer->image_channel_num.reg;
    kpu->layer_argument_fifo = layer->image_size.reg;
    kpu->layer_argument_fifo = layer->kernel_pool_type_cfg.reg;
    kpu->layer_argument_fifo = layer->kernel_load_cfg.reg;
    kpu->layer_argument_fifo = layer->kernel_offset.reg;
    kpu->layer_argument_fifo = layer->kernel_calc_type_cfg.reg;
    kpu->layer_argument_fifo = layer->write_back_cfg.reg;
    kpu->layer_argument_fifo = layer->conv_value.reg;
    kpu->layer_argument_fifo = layer->conv_value2.reg;
    kpu->layer_argument_fifo = layer->dma_parameter.reg;
}

void kpu_init(int eight_bit_mode, plic_irq_callback_t callback, void *userdata)
{
    kpu->interrupt_clear.reg = 7;
    kpu->fifo_threshold.data = (kpu_config_fifo_threshold_t){
        .fifo_full_threshold = 10, .fifo_empty_threshold = 1};
    kpu->eight_bit_mode.data = (kpu_config_eight_bit_mode_t){
        .eight_bit_mode = eight_bit_mode};
    kpu->interrupt_mask.data = (kpu_config_interrupt_t){
        .calc_done_int = 1,
        .layer_cfg_almost_empty_int = 0,
        .layer_cfg_almost_full_int = 1};

    plic_set_priority(IRQN_AI_INTERRUPT, 1);
    plic_irq_register(IRQN_AI_INTERRUPT, callback, userdata);
    plic_irq_enable(IRQN_AI_INTERRUPT);
}

void kpu_input_dma(const kpu_layer_argument_t *layer, const uint8_t *src, dmac_channel_number_t dma_ch, plic_irq_callback_t callback, void *userdata)
{
    uint64_t input_size = layer->kernel_calc_type_cfg.data.channel_switch_addr * 64 * (layer->image_channel_num.data.i_ch_num + 1);
    dmac_set_irq(dma_ch, callback, userdata, 1);
    dmac_set_single_mode(dma_ch, (void *)src, (void *)(uintptr_t)(AI_IO_BASE_ADDR + layer->image_addr.data.image_src_addr * 64), DMAC_ADDR_INCREMENT, DMAC_ADDR_INCREMENT,
                         DMAC_MSIZE_16, DMAC_TRANS_WIDTH_64, input_size / 8);
}

static void kpu_conv2d_core(kpu_layer_argument_t *layer)
{
    kpu_send_layer(layer);
}

void kpu_conv2d(kpu_layer_argument_t *layer)
{
    kpu->interrupt_clear.data = (kpu_config_interrupt_t){
        .calc_done_int = 1,
        .layer_cfg_almost_empty_int = 1,
        .layer_cfg_almost_full_int = 1};
    kpu->interrupt_mask.data = (kpu_config_interrupt_t){
        .calc_done_int = 1,
        .layer_cfg_almost_empty_int = 0,
        .layer_cfg_almost_full_int = 1};
    kpu_conv2d_core(layer);
}

void kpu_conv2d_output(kpu_layer_argument_t *layer, dmac_channel_number_t dma_ch, uint8_t *dest, plic_irq_callback_t callback, void *userdata)
{
    kpu->interrupt_clear.data = (kpu_config_interrupt_t){
        .calc_done_int = 1,
        .layer_cfg_almost_empty_int = 1,
        .layer_cfg_almost_full_int = 1};
    kpu->interrupt_mask.data = (kpu_config_interrupt_t){
        .calc_done_int = 1,
        .layer_cfg_almost_empty_int = 1,
        .layer_cfg_almost_full_int = 1};
    layer->dma_parameter.data.send_data_out = 1;
    sysctl_dma_select(dma_ch, SYSCTL_DMA_SELECT_AI_RX_REQ);
    dmac_set_irq(dma_ch, callback, userdata, 1);
    dmac_set_single_mode(dma_ch, (void *)(&kpu->fifo_data_out), dest, DMAC_ADDR_NOCHANGE, DMAC_ADDR_INCREMENT,
                         DMAC_MSIZE_8, DMAC_TRANS_WIDTH_64, (layer->dma_parameter.data.dma_total_byte + 8) / 8);
    kpu_conv2d_core(layer);
}

void kpu_conv2d_output_full_add(kpu_layer_argument_t *layer, dmac_channel_number_t dma_ch, uint64_t *dest, plic_irq_callback_t callback, void *userdata)
{
    uint32_t channels = layer->image_channel_num.data.o_ch_num + 1;
    layer->interrupt_enabe.data.full_add = 1;

    kpu->interrupt_clear.data = (kpu_config_interrupt_t){
        .calc_done_int = 1,
        .layer_cfg_almost_empty_int = 1,
        .layer_cfg_almost_full_int = 1};
    kpu->interrupt_mask.data = (kpu_config_interrupt_t){
        .calc_done_int = 1,
        .layer_cfg_almost_empty_int = 1,
        .layer_cfg_almost_full_int = 1};
    layer->dma_parameter.data.send_data_out = 1;
    sysctl_dma_select(dma_ch, SYSCTL_DMA_SELECT_AI_RX_REQ);
    dmac_set_irq(dma_ch, callback, userdata, 1);
    dmac_set_single_mode(dma_ch, (void *)(&kpu->fifo_data_out), dest, DMAC_ADDR_NOCHANGE, DMAC_ADDR_INCREMENT,
                         DMAC_MSIZE_8, DMAC_TRANS_WIDTH_64, channels);
    kpu_conv2d_core(layer);
}

void kpu_add(const uint8_t *src1, const quantize_param_t *src1_param, const uint8_t *src2, const quantize_param_t *src2_param, size_t count, uint8_t *dest, const quantize_param_t *dest_param)
{
    quantize_param_t q1 = *src1_param, q2 = *src2_param, q3 = *dest_param;

    size_t i;
    for(i = 0; i < count; i++)
    {
        int value = ((*src1++ * q1.scale + q1.bias + *src2++ * q2.scale + q2.bias) - q3.bias) / q3.scale;
        if(value < 0)
            value = 0;
        if(value > 0xFF)
            value = 0xFF;
        *dest++ = value;
    }
}

void kpu_global_average_pool(const uint8_t *src, const quantize_param_t *src_param, int kernel_size, int channels, uint8_t *dest, const quantize_param_t *dest_param)
{
    quantize_param_t q1 = *src_param, q2 = *dest_param;
    size_t oc, y, x;

    if(((uintptr_t)dest) >= AI_IO_BASE_ADDR && ((uintptr_t)dest) < AI_IO_BASE_ADDR + 2 * 1024 * 1024)
    {
        uint32_t row_padding = 16;
        uint32_t row_group = 4;
        uint32_t row_length = 1;
        uint32_t height = 4;

        for(oc = 0; oc < channels; oc++)
        {
            uint8_t *channel_origin = dest + oc / row_group * row_length * height * 64 + oc % row_group * row_padding;
            for(y = 0; y < 1; y++)
            {
                uint8_t *y_origin = channel_origin + y * row_length * 64;
                for(x = 0; x < 1; x++)
                {
                    int64_t sum = 0;
                    size_t i;
                    for(i = 0; i < kernel_size; i++)
                        sum += *src++;

                    int value = ((sum * q1.scale + q1.bias) / kernel_size - q2.bias) / q2.scale;
                    if(value < 0)
                        value = 0;
                    if(value > 0xFF)
                        value = 0xFF;
                    y_origin[x] = value;
                }
            }
        }
    } else
    {
        for(oc = 0; oc < channels; oc++)
        {
            int64_t sum = 0;
            size_t i;
            for(i = 0; i < kernel_size; i++)
                sum += *src++;

            int value = ((sum * q1.scale + q1.bias) / kernel_size - q2.bias) / q2.scale;
            if(value < 0)
                value = 0;
            if(value > 0xFF)
                value = 0xFF;
            dest[oc] = value;
        }
    }
}

void kpu_global_average_pool_float(const uint8_t *src, const quantize_param_t *src_param, int kernel_size, int channels, float *dest)
{
    quantize_param_t q = *src_param;
    size_t oc;

    for(oc = 0; oc < channels; oc++)
    {
        int64_t sum = 0;
        size_t i;
        for(i = 0; i < kernel_size; i++)
            sum += *src++;

        float value = (sum * q.scale + q.bias) / kernel_size;
        dest[oc] = value;
    }
}

void kpu_matmul_end(const uint8_t *src, int channels, float *dest, const quantize_param_t *dest_param)
{
    quantize_param_t q1 = *dest_param;
    size_t i = 0;
    for(i = 0; i < channels; i++)
        *dest++ = src[i * 16] * q1.scale + q1.bias;
}

void kpu_fully_connected(const float *src, const float *weights, const float *biases, float *dest, int input_channels, int output_channels)
{
    int ic, oc;
    for(oc = 0; oc < output_channels; oc++)
    {
        const float *c_weights = weights + oc * input_channels;

        float sum = 0.0f;
        for(ic = 0; ic < input_channels; ic++)
            sum += src[ic] * c_weights[ic];
        dest[oc] = sum + biases[oc];
    }
}

void kpu_dequantize(const uint8_t *src, const quantize_param_t *src_param, size_t count, float *dest)
{
    quantize_param_t q1 = *src_param;
    size_t i = 0;
    for(i = 0; i < count; i++)
        *dest++ = src[i] * q1.scale + q1.bias;
}

void kpu_input_with_padding(kpu_layer_argument_t *layer, const uint8_t *src, int width, int height, int channels)
{
    uint8_t *dest = (uint8_t *)(uintptr_t)(AI_IO_BASE_ADDR + layer->image_addr.data.image_src_addr * 64);
    size_t oc, y, x;

    uint32_t row_padding;
    uint32_t row_group;
    uint32_t row_length;

    if(width <= 16)
    {
        row_padding = 16;
        row_group = 4;
        row_length = 1;
    } else if(width <= 32)
    {
        row_padding = 32;
        row_group = 2;
        row_length = 1;
    } else
    {
        row_padding = 64;
        row_group = 1;
        row_length = (width + 63) / 64;
    }

    for(oc = 0; oc < channels; oc++)
    {
        uint8_t *channel_origin = dest + oc / row_group * row_length * height * 64 + oc % row_group * row_padding;
        for(y = 0; y < height; y++)
        {
            uint8_t *y_origin = channel_origin + y * row_length * 64;
            for(x = 0; x < width; x++)
                y_origin[x] = *src++;
        }
    }
}
#if USE_CACHED_AI_RAM
static void kpu_flush_cache(uint32_t addr, size_t lines)
{
    size_t line;
    for(line = 0; line < lines; line++)
    {
        const uint64_t *src = (const uint64_t *)(AI_RAM_BASE_ADDR + (addr + line) * 64);
        uint64_t *dest = (uint64_t *)(AI_IO_BASE_ADDR + (addr + line) * 64);
        size_t i;
        for(i = 0; i < 8; i++)
            dest[i] = src[i];
    }
}
#endif
static int64_t kpu_carry_shift(int64_t value, uint32_t shift)
{
    if(shift > 0)
    {
        value >>= shift - 1;
        if(value & 0x1)
        {
            if(value < 0)
                value = (value >> 1) - 1;
            else
                value = (value >> 1) + 1;
        } else
        {
            value >>= 1;
        }
    }

    return value;
}
static void kpu_upload_core(size_t width, size_t height, size_t channels, const uint8_t *src, uint32_t kpu_addr)
{
    uint8_t *dest = (uint8_t *)(uintptr_t)(AI_IO_BASE_ADDR + kpu_addr * 64);
    size_t oc, y, x;
    uint32_t row_padding;
    uint32_t row_group;
    uint32_t row_length;
    if(width <= 16)
    {
        row_padding = 16;
        row_group = 4;
        row_length = 1;
    } else if(width <= 32)
    {
        row_padding = 32;
        row_group = 2;
        row_length = 1;
    } else
    {
        row_padding = 64;
        row_group = 1;
        row_length = (width + 63) / 64;
    }

    if((uintptr_t)src % 8 == 0 && width % 8 == 0)
    {
#define UPLOAD_BEGIN()                                                                                             \
    for(oc = 0; oc < channels; oc++)                                                                               \
    {                                                                                                              \
        uint8_t *channel_origin = dest + oc / row_group * row_length * height * 64 + oc % row_group * row_padding; \
        for(y = 0; y < height; y++)                                                                                \
        {                                                                                                          \
            uint64_t *y_origin = (uint64_t *)(channel_origin + y * row_length * 64);

#define UPLOAD_END() \
    }                \
    }

        width /= 8;
        const uint64_t *u64_src = (const uint64_t *)src;
        if(width == 1)
        {
            UPLOAD_BEGIN()
            y_origin[0] = *u64_src++;
            UPLOAD_END()
        } else if(width == 2)
        {
            UPLOAD_BEGIN()
            {
                y_origin[0] = *u64_src++;
                y_origin[1] = *u64_src++;
            }
            UPLOAD_END()
        } else if(width == 4)
        {
            UPLOAD_BEGIN()
            {
                y_origin[0] = *u64_src++;
                y_origin[1] = *u64_src++;
                y_origin[2] = *u64_src++;
                y_origin[3] = *u64_src++;
            }
            UPLOAD_END()
        } else
        {
            UPLOAD_BEGIN()
            for(x = 0; x < width; x++)
                y_origin[x] = *u64_src++;
            UPLOAD_END()
        }
    } else
    {
        for(oc = 0; oc < channels; oc++)
        {
            uint8_t *channel_origin = dest + oc / row_group * row_length * height * 64 + oc % row_group * row_padding;
            for(y = 0; y < height; y++)
            {
                uint8_t *y_origin = channel_origin + y * row_length * 64;
                for(x = 0; x < width; x++)
                    y_origin[x] = *src++;
            }
        }
    }
}
static void kpu_kmodel_input_with_padding(const kpu_layer_argument_t *layer, const uint8_t *src)
{
    size_t width = layer->image_size.data.i_row_wid + 1;
    size_t height = layer->image_size.data.i_col_high + 1;
    size_t channels = layer->image_channel_num.data.i_ch_num + 1;

    kpu_upload_core(width, height, channels, src, layer->image_addr.data.image_src_addr);
}

static void kpu_kmodel_input_float(const float *src, float *dest, size_t count)
{
    memcpy(dest, src, count * sizeof(float));
}

static void kpu_float_activation(float *data, size_t count, kpu_model_activation_t act)
{
    size_t i;

    if(act == KLA_RELU)
    {
        for(i = 0; i < count; i++)
            data[i] = max(data[i], 0);
    } else if(act == KLA_RELU6)
    {
        for(i = 0; i < count; i++)
            data[i] = min(max(data[i], 0), 6);
    }
}

static void kpu_kmodel_add(const kpu_model_add_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const float *src_a = (const float *)(ctx->main_buffer + arg->main_mem_in_a_address);
    const float *src_b = (const float *)(ctx->main_buffer + arg->main_mem_in_b_address);
    float *dest = (float *)(ctx->main_buffer + arg->main_mem_out_address);
    size_t i, count = arg->count;

    for(i = 0; i < count; i++)
        dest[i] = src_a[i] + src_b[i];
}

static void kpu_quantized_add(const kpu_model_quant_add_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const uint8_t *src_a = (const uint8_t *)(ctx->main_buffer + arg->main_mem_in_a_address);
    const uint8_t *src_b = (const uint8_t *)(ctx->main_buffer + arg->main_mem_in_b_address);
    size_t count = ALIGN_UP(arg->count, 8) / 8;
    int64_t off_a = arg->in_a_offset, mul_a = arg->in_a_mul, sh_a = arg->in_a_shift;
    int64_t off_b = arg->in_b_offset, mul_b = arg->in_b_mul, sh_b = arg->in_b_shift;
    int64_t off_o = arg->out_offset, mul_o = arg->out_mul, sh_o = arg->out_shift;

    uint8_t *dest = (uint8_t *)(ctx->main_buffer + arg->main_mem_out_address);
    size_t i;

    if(sh_a == sh_b)
    {
#define QADD_UNROLL_1(x)     \
    int64_t a##x = *src_a++; \
    int64_t b##x = *src_b++;

#define QADD_UNROLL_2(x) \
    a##x += off_a;       \
    b##x += off_b;

#define QADD_UNROLL_3(x) \
    a##x *= mul_a;       \
    b##x *= mul_b;

#define QADD_UNROLL_4(x) \
    int64_t v##x = a##x + b##x;

#define QADD_UNROLL_5(x) \
    v##x >>= sh_a;

#define QADD_UNROLL_6(x) \
    v##x *= mul_o;

#define QADD_UNROLL_7(x) \
    v##x = kpu_carry_shift(v##x, sh_o);

#define QADD_UNROLL_8(x) \
    v##x += off_o;

#define QADD_UNROLL_9(x) \
    v##x = min(0xFF, max(0, v##x));

#define QADD_UNROLL_10(x) \
    *dest++ = v##x;

#define QADD_UNROLL_S(x)                       \
    QADD_UNROLL_##x(0)                         \
        QADD_UNROLL_##x(1)                     \
            QADD_UNROLL_##x(2)                 \
                QADD_UNROLL_##x(3)             \
                    QADD_UNROLL_##x(4)         \
                        QADD_UNROLL_##x(5)     \
                            QADD_UNROLL_##x(6) \
                                QADD_UNROLL_##x(7)

        for(i = 0; i < count; i++)
        {
            QADD_UNROLL_S(1);
            QADD_UNROLL_S(2);
            QADD_UNROLL_S(3);
            QADD_UNROLL_S(4);
            QADD_UNROLL_S(5);
            QADD_UNROLL_S(6);
            QADD_UNROLL_S(7);
            QADD_UNROLL_S(8);
            QADD_UNROLL_S(9);
            QADD_UNROLL_S(10);
        }
    } else
    {
#undef QADD_UNROLL_1
#define QADD_UNROLL_1(x)     \
    int64_t a##x = *src_a++; \
    int64_t b##x = *src_b++;

#undef QADD_UNROLL_2
#define QADD_UNROLL_2(x) \
    a##x += off_a;       \
    b##x += off_b;

#undef QADD_UNROLL_3
#define QADD_UNROLL_3(x) \
    a##x *= mul_a;       \
    b##x *= mul_b;

#undef QADD_UNROLL_4
#define QADD_UNROLL_4(x) \
    a##x >>= sh_a;       \
    b##x >>= sh_b;

#undef QADD_UNROLL_5
#define QADD_UNROLL_5(x) \
    int64_t v##x = a##x + b##x;

#undef QADD_UNROLL_6
#define QADD_UNROLL_6(x) \
    v##x *= mul_o;

#undef QADD_UNROLL_7
#define QADD_UNROLL_7(x) \
    v##x = kpu_carry_shift(v##x, sh_o);

#undef QADD_UNROLL_8
#define QADD_UNROLL_8(x) \
    v##x += off_o;

#undef QADD_UNROLL_9
#define QADD_UNROLL_9(x) \
    v##x = min(0xFF, max(0, v##x));

#undef QADD_UNROLL_10
#define QADD_UNROLL_10(x) \
    *dest++ = v##x;

#undef QADD_UNROLL_S
#define QADD_UNROLL_S(x)                       \
    QADD_UNROLL_##x(0)                         \
        QADD_UNROLL_##x(1)                     \
            QADD_UNROLL_##x(2)                 \
                QADD_UNROLL_##x(3)             \
                    QADD_UNROLL_##x(4)         \
                        QADD_UNROLL_##x(5)     \
                            QADD_UNROLL_##x(6) \
                                QADD_UNROLL_##x(7)

        for(i = 0; i < count; i++)
        {
            QADD_UNROLL_S(1);
            QADD_UNROLL_S(2);
            QADD_UNROLL_S(3);
            QADD_UNROLL_S(4);
            QADD_UNROLL_S(5);
            QADD_UNROLL_S(6);
            QADD_UNROLL_S(7);
            QADD_UNROLL_S(8);
            QADD_UNROLL_S(9);
            QADD_UNROLL_S(10);
        }
    }
}

static void kpu_global_average_pool2d(const kpu_model_gap2d_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const float *src = (const float *)(ctx->main_buffer + arg->main_mem_in_address);
    float *dest = (float *)(ctx->main_buffer + arg->main_mem_out_address);
    size_t oc, channels = arg->channels, kernel_size = arg->kernel_size;

    for(oc = 0; oc < channels; oc++)
    {
        float sum = 0.f;
        size_t i;
        for(i = 0; i < kernel_size; i++)
            sum += *src++;

        dest[oc] = sum / kernel_size;
    }
}

static void kpu_quantized_max_pool2d(const kpu_model_quant_max_pool2d_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const uint8_t *src = (const uint8_t *)(ctx->main_buffer + arg->main_mem_in_address);
    uint8_t *dest = (uint8_t *)(ctx->main_buffer + arg->main_mem_out_address);
    kpu_model_shape_t in_shape = arg->in_shape, out_shape = arg->out_shape;
    uint32_t kernel_width = arg->kernel_width, kernel_height = arg->kernel_height;
    uint32_t stride_width = arg->stride_width, stride_height = arg->stride_height;
    uint32_t padding_width = arg->padding_width, padding_height = arg->padding_height;

    uint32_t out_y, out_x, oc;

    for(oc = 0; oc < out_shape.channels; oc++)
    {
        const uint8_t *channel_src = src + in_shape.width * in_shape.height * oc;
        for(out_y = 0; out_y < out_shape.height; out_y++)
        {
            for(out_x = 0; out_x < out_shape.width; out_x++)
            {
                int32_t in_x_origin = (int32_t)(out_x * stride_width) - padding_width;
                int32_t in_y_origin = (int32_t)(out_y * stride_height) - padding_height;
                int32_t kernel_x_start = max(0, -in_x_origin);
                int32_t kernel_x_end = min(kernel_width, in_shape.width - in_x_origin);
                int32_t kernel_y_start = max(0, -in_y_origin);
                int32_t kernel_y_end = min(kernel_height, in_shape.height - in_y_origin);
                uint8_t value = 0;

                int32_t kernel_y, kernel_x;
                for(kernel_y = kernel_y_start; kernel_y < kernel_y_end; kernel_y++)
                {
                    for(kernel_x = kernel_x_start; kernel_x < kernel_x_end; kernel_x++)
                    {
                        int32_t in_x = in_x_origin + kernel_x;
                        int32_t in_y = in_y_origin + kernel_y;
                        value = max(value, channel_src[in_y * in_shape.width + in_x]);
                    }
                }

                *dest++ = value;
            }
        }
    }
}

static void kpu_average_pool2d(const kpu_model_ave_pool2d_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const float *src = (const float *)(ctx->main_buffer + arg->main_mem_in_address);
    float *dest = (float *)(ctx->main_buffer + arg->main_mem_out_address);
    kpu_model_shape_t in_shape = arg->in_shape, out_shape = arg->out_shape;
    uint32_t kernel_width = arg->kernel_width, kernel_height = arg->kernel_height;
    uint32_t stride_width = arg->stride_width, stride_height = arg->stride_height;
    uint32_t padding_width = arg->padding_width, padding_height = arg->padding_height;

    uint32_t out_y, out_x, oc;

    for(oc = 0; oc < out_shape.channels; oc++)
    {
        const float *channel_src = src + in_shape.width * in_shape.height * oc;
        for(out_y = 0; out_y < out_shape.height; out_y++)
        {
            for(out_x = 0; out_x < out_shape.width; out_x++)
            {
                int32_t in_x_origin = (int32_t)(out_x * stride_width) - padding_width;
                int32_t in_y_origin = (int32_t)(out_y * stride_height) - padding_height;
                int32_t kernel_x_start = max(0, -in_x_origin);
                int32_t kernel_x_end = min(kernel_width, in_shape.width - in_x_origin);
                int32_t kernel_y_start = max(0, -in_y_origin);
                int32_t kernel_y_end = min(kernel_height, in_shape.height - in_y_origin);
                float value = 0;
                float kernel_count = 0;

                int32_t kernel_y, kernel_x;
                for(kernel_y = kernel_y_start; kernel_y < kernel_y_end; kernel_y++)
                {
                    for(kernel_x = kernel_x_start; kernel_x < kernel_x_end; kernel_x++)
                    {
                        int32_t in_x = in_x_origin + kernel_x;
                        int32_t in_y = in_y_origin + kernel_y;
                        value += channel_src[in_y * in_shape.width + in_x];
                        kernel_count++;
                    }
                }

                *dest++ = value / kernel_count;
            }
        }
    }
}

static void kpu_quantize(const kpu_model_quantize_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    size_t count = arg->count;
    const float *src = (const float *)(ctx->main_buffer + arg->main_mem_in_address);
    ;
    const kpu_model_quant_param_t q = arg->quant_param;
    float scale = 1.f / q.scale;

    uint8_t *dest = (uint8_t *)(ctx->main_buffer + arg->mem_out_address);
    size_t i;
    for(i = 0; i < count; i++)
    {
        int value = roundf((*src++ - q.bias) * scale);
        if(value < 0)
            value = 0;
        if(value > 0xFF)
            value = 0xFF;
        *dest++ = (uint8_t)value;
    }
}

static void kpu_kmodel_dequantize(const kpu_model_dequantize_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const uint8_t *src = (const uint8_t *)(ctx->main_buffer + arg->main_mem_in_address);
    float *dest = (float *)(ctx->main_buffer + arg->main_mem_out_address);
    size_t oc, count = arg->count;
    const kpu_model_quant_param_t q = arg->quant_param;

    for(oc = 0; oc < count; oc++)
        dest[oc] = *src++ * q.scale + q.bias;
}

static void kpu_kmodel_channelwise_dequantize(const kpu_model_channelwise_dequant_argument_t *arg, kpu_model_context_t *ctx)
{
    const uint8_t *src = (const uint8_t *)(ctx->main_buffer + arg->main_mem_in_address);
    float *dest = (float *)(ctx->main_buffer + arg->main_mem_out_address);
    size_t oc, i, channels = arg->channels, count = arg->channel_size;

    for(oc = 0; oc < channels; oc++)
    {
        const kpu_model_quant_param_t q = arg->quant_params[oc];

        for(i = 0; i < count; i++)
            *dest++ = *src++ * q.scale + q.bias;
    }
}

static void kpu_requantize(const kpu_model_requantize_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const uint8_t *src = (const uint8_t *)(ctx->main_buffer + arg->main_mem_in_address);
    uint8_t *dest = (uint8_t *)(ctx->main_buffer + arg->main_mem_out_address);
    size_t oc, count = arg->count;
    const uint8_t *table = arg->table;

    if(false && count % 8 == 0)
    {
        for(oc = 0; oc < count;)
        {
            dest[oc++] = table[*src++];
            dest[oc++] = table[*src++];
            dest[oc++] = table[*src++];
            dest[oc++] = table[*src++];
            dest[oc++] = table[*src++];
            dest[oc++] = table[*src++];
            dest[oc++] = table[*src++];
            dest[oc++] = table[*src++];
        }
    } else
    {
        for(oc = 0; oc < count; oc++)
            dest[oc] = table[src[oc]];
    }
}

static void kpu_l2_normalization(const kpu_model_l2_norm_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const float *src = (const float *)(ctx->main_buffer + arg->main_mem_in_address);
    float *dest = (float *)(ctx->main_buffer + arg->main_mem_out_address);
    size_t oc, channels = arg->channels;

    float sum = 0.f;
    const float epsilon = 1e-10f;
    for(oc = 0; oc < channels; oc++)
        sum += src[oc] * src[oc];
    if(sum < epsilon)
        sum = epsilon;
    sum = 1.f / sqrtf(sum);
    for(oc = 0; oc < channels; oc++)
        dest[oc] = src[oc] * sum;
}

static void kpu_softmax(const kpu_model_softmax_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const float *src = (const float *)(ctx->main_buffer + arg->main_mem_in_address);
    float *dest = (float *)(ctx->main_buffer + arg->main_mem_out_address);
    size_t oc, channels = arg->channels;

    float max = FLT_MIN;
    for(oc = 0; oc < channels; oc++)
        max = fmaxf(max, src[oc]);

    float sum = 0.f;
    for(oc = 0; oc < channels; oc++)
    {
        float value = expf(src[oc] - max);
        sum += value;
        dest[oc] = value;
    }

    for(oc = 0; oc < channels; oc++)
        dest[oc] /= sum;
}

static void kpu_concat(const kpu_model_concat_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    uint8_t *dest = (uint8_t *)(ctx->main_buffer + arg->main_mem_out_address);
    uint32_t count = arg->input_count, i;

    for(i = 0; i < count; i++)
    {
        kpu_model_memory_range_t input = arg->inputs_mem[i];
        const uint8_t *src = (const uint8_t *)(ctx->main_buffer + input.start);
        memcpy(dest, src, input.size);
        dest += input.size;
    }
}

static void kpu_kmodel_fully_connected(const kpu_model_fully_connected_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const float *src = (const float *)(ctx->main_buffer + arg->main_mem_in_address);
    float *dest = (float *)(ctx->main_buffer + arg->main_mem_out_address);
    uint32_t in_channels = arg->in_channels, out_channels = arg->out_channels, ic, oc;
    const float *weights = arg->weights, *bias = arg->weights + in_channels * out_channels;

    if(in_channels % 8 == 0)
    {
#define FC_UNROLL_1(x)     \
    float i##x = *c_src++; \
    float w##x = *c_weights++;

#define FC_UNROLL_2(x) \
    sum += i##x * w##x;

#define FC_UNROLL_S(x)                       \
    FC_UNROLL_##x(0)                         \
        FC_UNROLL_##x(1)                     \
            FC_UNROLL_##x(2)                 \
                FC_UNROLL_##x(3)             \
                    FC_UNROLL_##x(4)         \
                        FC_UNROLL_##x(5)     \
                            FC_UNROLL_##x(6) \
                                FC_UNROLL_##x(7)

        for(oc = 0; oc < out_channels; oc++)
        {
            const float *c_src = src;
            const float *c_weights = weights + oc * in_channels;

            float sum = 0.0f;
            for(ic = 0; ic < in_channels / 8; ic++)
            {
                FC_UNROLL_S(1);
                FC_UNROLL_S(2);
            }

            dest[oc] = sum + bias[oc];
        }
    } else
    {
        for(oc = 0; oc < out_channels; oc++)
        {
            const float *c_weights = weights + oc * in_channels;

            float sum = 0.0f;
            for(ic = 0; ic < in_channels; ic++)
                sum += src[ic] * c_weights[ic];
            dest[oc] = sum + bias[oc];
        }
    }

    kpu_float_activation(dest, out_channels, arg->act);
}

static void kpu_tf_flatten(const kpu_model_tf_flatten_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const float *src = (const float *)(ctx->main_buffer + arg->main_mem_in_address);
    float *dest = (float *)(ctx->main_buffer + arg->main_mem_out_address);
    kpu_model_shape_t in_shape = arg->shape;
    uint32_t oc, oy, ox;

    for(oy = 0; oy < in_shape.height; oy++)
        for(ox = 0; ox < in_shape.width; ox++)
            for(oc = 0; oc < in_shape.channels; oc++)
                *dest++ = src[(oc * in_shape.height + oy) * in_shape.width + ox];
}

static void kpu_resize_nearest_neighbor(const kpu_model_resize_nearest_neighbor_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const float *src = (const float *)(ctx->main_buffer + arg->main_mem_in_address);
    float *dest = (float *)(ctx->main_buffer + arg->main_mem_out_address);
    kpu_model_shape_t in_shape = arg->in_shape;
    uint32_t out_width = arg->out_width, out_height = arg->out_height;
    uint32_t oc, oy, ox;

    float height_scale = (float)in_shape.height / out_height;
    float width_scale = (float)in_shape.width / out_width;

    for(oc = 0; oc < in_shape.channels; oc++)
    {
        const float *channel_src = src + in_shape.width * in_shape.height * oc;
        for(oy = 0; oy < out_height; oy++)
        {
            uint32_t in_y = (uint32_t)min(floorf(oy * height_scale), in_shape.height - 1);
            const float *y_origin = channel_src + in_y * in_shape.width;
            for(ox = 0; ox < out_width; ox++)
            {
                uint32_t in_x = (uint32_t)min(floorf(ox * width_scale), in_shape.width - 1);
                *dest++ = y_origin[in_x];
            }
        }
    }
}

static void kpu_quant_resize_nearest_neighbor(const kpu_model_quant_resize_nearest_neighbor_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const uint8_t *src = (const uint8_t *)(ctx->main_buffer + arg->main_mem_in_address);
    uint8_t *dest = (uint8_t *)(ctx->main_buffer + arg->main_mem_out_address);
    kpu_model_shape_t in_shape = arg->in_shape;
    uint32_t out_width = arg->out_width, out_height = arg->out_height;
    uint32_t oc, oy, ox;

    float height_scale = (float)in_shape.height / out_height;
    float width_scale = (float)in_shape.width / out_width;

    for(oc = 0; oc < in_shape.channels; oc++)
    {
        const uint8_t *channel_src = src + in_shape.width * in_shape.height * oc;
        for(oy = 0; oy < out_height; oy++)
        {
            uint32_t in_y = (uint32_t)min(floorf(oy * height_scale), in_shape.height - 1);
            const uint8_t *y_origin = channel_src + in_y * in_shape.width;
            for(ox = 0; ox < out_width; ox++)
            {
                uint32_t in_x = (uint32_t)min(floorf(ox * width_scale), in_shape.width - 1);
                *dest++ = y_origin[in_x];
            }
        }
    }
}

static void kpu_logistic(const kpu_model_logistic_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const float *src = (const float *)(ctx->main_buffer + arg->main_mem_in_address);
    float *dest = (float *)(ctx->main_buffer + arg->main_mem_out_address);
    size_t oc, channels = arg->channels;

    for(oc = 0; oc < channels; oc++)
        dest[oc] = 1.f / (1.f + expf(-src[oc]));
}

static void kpu_conv(const kpu_model_conv_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    volatile kpu_layer_argument_t layer = *(const volatile kpu_layer_argument_t *)(ctx->model_buffer + arg->layer_offset);
    layer.kernel_load_cfg.data.para_start_addr = (uintptr_t)(ctx->model_buffer + arg->weights_offset);
    layer.kernel_pool_type_cfg.data.bwsx_base_addr = (uintptr_t)(ctx->model_buffer + arg->bn_offset);
    layer.kernel_calc_type_cfg.data.active_addr = (uintptr_t)(ctx->model_buffer + arg->act_offset);

    if(arg->flags & KLF_MAIN_MEM_OUT)
    {
        dmac_channel_number_t dma_ch = ctx->dma_ch;
        uint8_t *dest = ctx->main_buffer + arg->main_mem_out_address;
        kpu->interrupt_clear.data = (kpu_config_interrupt_t){
            .calc_done_int = 1,
            .layer_cfg_almost_empty_int = 1,
            .layer_cfg_almost_full_int = 1};
        kpu->interrupt_mask.data = (kpu_config_interrupt_t){
            .calc_done_int = 1,
            .layer_cfg_almost_empty_int = 1,
            .layer_cfg_almost_full_int = 1};
        layer.dma_parameter.data.send_data_out = 1;
        sysctl_dma_select(dma_ch, SYSCTL_DMA_SELECT_AI_RX_REQ);
        if(ctx->current_layer != ctx->layers_length)
            dmac_set_irq(dma_ch, ai_step, ctx, 1);
        else
            dmac_set_irq(dma_ch, (plic_irq_callback_t)kpu_kmodel_done, ctx, 1);
        dmac_set_single_mode(dma_ch, (void *)(&kpu->fifo_data_out), dest, DMAC_ADDR_NOCHANGE, DMAC_ADDR_INCREMENT,
                             DMAC_MSIZE_8, DMAC_TRANS_WIDTH_64, (layer.dma_parameter.data.dma_total_byte + 8) / 8);
    } else
    {
        kpu->interrupt_clear.data = (kpu_config_interrupt_t){
            .calc_done_int = 1,
            .layer_cfg_almost_empty_int = 1,
            .layer_cfg_almost_full_int = 1};

        kpu->interrupt_mask.data = (kpu_config_interrupt_t){
            .calc_done_int = 0,
            .layer_cfg_almost_empty_int = 1,
            .layer_cfg_almost_full_int = 1};
        layer.interrupt_enabe.data.int_en = 1;
    }

    kpu_send_layer((const kpu_layer_argument_t *)&layer);
}

static void kpu_add_padding(const kpu_model_add_padding_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const uint8_t *src = (const uint8_t *)(ctx->main_buffer + arg->main_mem_in_address);
#if USE_CACHED_AI_RAM
    uint8_t *dest = (uint8_t *)(uintptr_t)(AI_RAM_BASE_ADDR + arg->kpu_mem_out_address * 64);
#else
    uint8_t *dest = (uint8_t *)(uintptr_t)(AI_IO_BASE_ADDR + arg->kpu_mem_out_address * 64);
#endif

    uint32_t row_padding = 16;
    uint32_t row_group = 4;
    uint32_t row_length = 1;
    uint32_t height = 4;
    uint32_t oc, x, y, channels = arg->channels;

    for(oc = 0; oc < channels; oc++)
    {
        uint8_t *channel_origin = dest + oc / row_group * row_length * height * 64 + oc % row_group * row_padding;
        for(y = 0; y < 1; y++)
        {
            uint8_t *y_origin = channel_origin + y * row_length * 64;
            for(x = 0; x < 1; x++)
                y_origin[x] = *src++;
        }
    }

#if USE_CACHED_AI_RAM
    uint32_t lines = row_length * height * channels / row_group;
    kpu_flush_cache(arg->kpu_mem_out_address, lines);
#endif
}

static void kpu_remove_padding(const kpu_model_remove_padding_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    const uint8_t *src = (const uint8_t *)(ctx->main_buffer + arg->main_mem_in_address);
    uint8_t *dest = (uint8_t *)(ctx->main_buffer + arg->main_mem_out_address);
    uint32_t oc, channels = arg->channels;

    for(oc = 0; oc < channels; oc++)
        *dest++ = src[oc * 16];
}

static void kpu_upload(const kpu_model_upload_layer_argument_t *arg, kpu_model_context_t *ctx)
{
    size_t width = arg->width;
    size_t height = arg->height;
    size_t channels = arg->channels;

    kpu_upload_core(width, height, channels, ctx->main_buffer + arg->main_mem_in_address, arg->kpu_mem_out_address);
}

int kpu_load_kmodel(kpu_model_context_t *ctx, const uint8_t *buffer)
{
    uintptr_t base_addr = (uintptr_t)buffer;
    const kpu_kmodel_header_t *header = (const kpu_kmodel_header_t *)buffer;

    if(header->version == 3 && header->arch == 0)
    {
        ctx->is_nncase = 0;
        ctx->model_buffer = buffer;
        ctx->output_count = header->output_count;
        ctx->outputs = (const kpu_model_output_t *)(base_addr + sizeof(kpu_kmodel_header_t));
        ctx->layer_headers = (const kpu_model_layer_header_t *)((uintptr_t)ctx->outputs + sizeof(kpu_model_output_t) * ctx->output_count);
        ctx->layers_length = header->layers_length;
        ctx->body_start = (const uint8_t *)((uintptr_t)ctx->layer_headers + sizeof(kpu_model_layer_header_t) * header->layers_length);
        ctx->main_buffer = (uint8_t *)malloc(header->main_mem_usage);
        if(!ctx->main_buffer)
            return -1;
    } else if(header->version == 'KMDL')
    {
        return nncase_load_kmodel(ctx, buffer);
    } else
    {
        return -1;
    }

    return 0;
}

int kpu_get_output(kpu_model_context_t *ctx, uint32_t index, uint8_t **data, size_t *size)
{
    if(ctx->is_nncase)
        return nncase_get_output(ctx, index, data, size);

    if(index >= ctx->output_count)
        return -1;

    const kpu_model_output_t *output = ctx->outputs + index;
    *data = ctx->main_buffer + output->address;
    *size = output->size;
    return 0;
}

void kpu_model_free(kpu_model_context_t *ctx)
{
    if(ctx->is_nncase)
        return nncase_model_free(ctx);

    free(ctx->main_buffer);
    ctx->main_buffer = NULL;
}

#if KPU_DEBUG
static uint64_t last_time;
static uint64_t total_time;
static uint64_t kpu_time;
static uint32_t last_layer_type;

static const char *str_layer_type(uint32_t type)
{
    switch(type)
    {
        case KL_ADD:
            return "Add";
        case KL_QUANTIZED_ADD:
            return "QuantAdd";
        case KL_GLOBAL_AVERAGE_POOL2D:
            return "GAP";
        case KL_QUANTIZED_MAX_POOL2D:
            return "QuantMaxPool2d";
        case KL_AVERAGE_POOL2D:
            return "AveragePool2d";
        case KL_QUANTIZE:
            return "Quantize";
        case KL_DEQUANTIZE:
            return "Dequantize";
        case KL_REQUANTIZE:
            return "Requantize";
        case KL_L2_NORMALIZATION:
            return "L2Norm";
        case KL_SOFTMAX:
            return "Softmax";
        case KL_CONCAT:
            return "Concat";
        case KL_QUANTIZED_CONCAT:
            return "QuantConcat";
        case KL_FULLY_CONNECTED:
            return "FullyConnected";
        case KL_TENSORFLOW_FLATTEN:
            return "TFFlatten";
        case KL_RESIZE_NEAREST_NEIGHBOR:
            return "ResizeNearestNeighbor";
        case KL_QUANTIZED_RESIZE_NEAREST_NEIGHBOR:
            return "QuantResizeNearestNeighbor";
        case KL_CHANNELWISE_DEQUANTIZE:
            return "ChannelwiseDequantize";
        case KL_LOGISTIC:
            return "Logistic";
        case KL_K210_CONV:
            return "K210Conv";
        case KL_K210_ADD_PADDING:
            return "K210AddPad";
        case KL_K210_REMOVE_PADDING:
            return "K210RemovePad";
        case KL_K210_UPLOAD:
            return "K210Upload";
        default:
            return "Unknown";
    }
}
#endif

static int kpu_kmodel_done(kpu_model_context_t *ctx)
{
    kpu->interrupt_clear.data = (kpu_config_interrupt_t){
        .calc_done_int = 1,
        .layer_cfg_almost_empty_int = 1,
        .layer_cfg_almost_full_int = 1};
    kpu->interrupt_mask.data = (kpu_config_interrupt_t){
        .calc_done_int = 1,
        .layer_cfg_almost_empty_int = 1,
        .layer_cfg_almost_full_int = 1};
#if KPU_DEBUG
    uint32_t cnt_layer_id = ctx->current_layer - 1;
    uint64_t time = sysctl_get_time_us();
    if(last_time != 0)
    {
        uint64_t layer_time = time - last_time;
        printf("layer %d [%s]: %f ms\n", cnt_layer_id, str_layer_type(last_layer_type), layer_time / 1000.0);
        total_time += layer_time;
        if(last_layer_type == KL_K210_CONV)
            kpu_time += layer_time;
    }

    printf("KPU: %f ms\n", kpu_time / 1000.0);
    printf("CPU: %f ms\n", (total_time - kpu_time) / 1000.0);
    printf("Model: %f ms\n", total_time / 1000.0);
#endif
    ctx->done_callback(ctx->userdata);
    return 0;
}

static int ai_step(void *userdata)
{
    kpu_model_context_t *ctx = (kpu_model_context_t *)userdata;

    uint32_t cnt_layer_id = ctx->current_layer++;
    const uint8_t *layer_body = ctx->current_body;
    const kpu_model_layer_header_t *cnt_layer_header = ctx->layer_headers + cnt_layer_id;
    ctx->current_body += cnt_layer_header->body_size;

#if KPU_DEBUG
    uint64_t time = sysctl_get_time_us();
    if(last_time != 0)
    {
        uint64_t layer_time = time - last_time;
        printf("layer %d [%s]: %f ms\n", cnt_layer_id - 1, str_layer_type(last_layer_type), layer_time / 1000.0);
        total_time += layer_time;
        if(last_layer_type == KL_K210_CONV)
            kpu_time += layer_time;
    }

    last_layer_type = cnt_layer_header->type;
    last_time = sysctl_get_time_us();
#endif

    switch(cnt_layer_header->type)
    {
        case KL_ADD:
            kpu_kmodel_add((const kpu_model_add_layer_argument_t *)layer_body, ctx);
            break;
        case KL_QUANTIZED_ADD:
            kpu_quantized_add((const kpu_model_quant_add_layer_argument_t *)layer_body, ctx);
            break;
        case KL_GLOBAL_AVERAGE_POOL2D:
            kpu_global_average_pool2d((const kpu_model_gap2d_layer_argument_t *)layer_body, ctx);
            break;
        case KL_QUANTIZED_MAX_POOL2D:
            kpu_quantized_max_pool2d((const kpu_model_quant_max_pool2d_layer_argument_t *)layer_body, ctx);
            break;
        case KL_AVERAGE_POOL2D:
            kpu_average_pool2d((const kpu_model_ave_pool2d_layer_argument_t *)layer_body, ctx);
            break;
        case KL_QUANTIZE:
            kpu_quantize((const kpu_model_quantize_layer_argument_t *)layer_body, ctx);
            break;
        case KL_DEQUANTIZE:
            kpu_kmodel_dequantize((const kpu_model_dequantize_layer_argument_t *)layer_body, ctx);
            break;
        case KL_REQUANTIZE:
            kpu_requantize((const kpu_model_requantize_layer_argument_t *)layer_body, ctx);
            break;
        case KL_L2_NORMALIZATION:
            kpu_l2_normalization((const kpu_model_l2_norm_layer_argument_t *)layer_body, ctx);
            break;
        case KL_SOFTMAX:
            kpu_softmax((const kpu_model_softmax_layer_argument_t *)layer_body, ctx);
            break;
        case KL_CONCAT:
        case KL_QUANTIZED_CONCAT:
            kpu_concat((const kpu_model_concat_layer_argument_t *)layer_body, ctx);
            break;
        case KL_FULLY_CONNECTED:
            kpu_kmodel_fully_connected((const kpu_model_fully_connected_layer_argument_t *)layer_body, ctx);
            break;
        case KL_TENSORFLOW_FLATTEN:
            kpu_tf_flatten((const kpu_model_tf_flatten_layer_argument_t *)layer_body, ctx);
            break;
        case KL_RESIZE_NEAREST_NEIGHBOR:
            kpu_resize_nearest_neighbor((const kpu_model_resize_nearest_neighbor_layer_argument_t *)layer_body, ctx);
            break;
        case KL_QUANTIZED_RESIZE_NEAREST_NEIGHBOR:
            kpu_quant_resize_nearest_neighbor((const kpu_model_quant_resize_nearest_neighbor_layer_argument_t *)layer_body, ctx);
            break;
        case KL_CHANNELWISE_DEQUANTIZE:
            kpu_kmodel_channelwise_dequantize((const kpu_model_channelwise_dequant_argument_t *)layer_body, ctx);
            break;
        case KL_LOGISTIC:
            kpu_logistic((const kpu_model_logistic_layer_argument_t *)layer_body, ctx);
            break;
        case KL_K210_CONV:
            kpu_conv((const kpu_model_conv_layer_argument_t *)layer_body, ctx);
            return 0;
        case KL_K210_ADD_PADDING:
            kpu_add_padding((const kpu_model_add_padding_layer_argument_t *)layer_body, ctx);
            break;
        case KL_K210_REMOVE_PADDING:
            kpu_remove_padding((const kpu_model_remove_padding_layer_argument_t *)layer_body, ctx);
            break;
        case KL_K210_UPLOAD:
            kpu_upload((const kpu_model_upload_layer_argument_t *)layer_body, ctx);
            break;
        default:
            assert(!"Layer is not supported.");
    }

    if(cnt_layer_id != (ctx->layers_length - 1))
        ai_step(userdata);
    else
        kpu_kmodel_done(ctx);
    return 0;
}

static void ai_step_not_isr(void *userdata)
{
    sysctl_disable_irq();
    ai_step(userdata);
    sysctl_enable_irq();
}

int kpu_run_kmodel(kpu_model_context_t *ctx, const uint8_t *src, dmac_channel_number_t dma_ch, kpu_done_callback_t done_callback, void *userdata)
{
    if(ctx->is_nncase)
        return nncase_run_kmodel(ctx, src, dma_ch, done_callback, userdata);

    ctx->dma_ch = dma_ch;
    ctx->done_callback = done_callback;
    ctx->userdata = userdata;
    ctx->current_layer = 0;
    ctx->current_body = ctx->body_start;
#if KPU_DEBUG
    last_time = 0;
    total_time = 0;
    kpu_time = 0;
#endif

    kpu_kmodel_header_t *header = (kpu_kmodel_header_t *)ctx->model_buffer;
    kpu->interrupt_clear.reg = 7;
    kpu->fifo_threshold.data = (kpu_config_fifo_threshold_t){
        .fifo_full_threshold = 10, .fifo_empty_threshold = 1};
    kpu->eight_bit_mode.data = (kpu_config_eight_bit_mode_t){
        .eight_bit_mode = header->flags & 1};
    kpu->interrupt_mask.data = (kpu_config_interrupt_t){
        .calc_done_int = 1,
        .layer_cfg_almost_empty_int = 0,
        .layer_cfg_almost_full_int = 1};

    plic_set_priority(IRQN_AI_INTERRUPT, 1);
    plic_irq_register(IRQN_AI_INTERRUPT, ai_step, ctx);
    plic_irq_enable(IRQN_AI_INTERRUPT);

    const kpu_model_layer_header_t *first_layer_header = ctx->layer_headers;

    switch(first_layer_header->type)
    {
        case KL_K210_CONV:
        {
            const kpu_model_conv_layer_argument_t *first_layer = (const kpu_model_conv_layer_argument_t *)ctx->body_start;
            kpu_layer_argument_t layer_arg = *(volatile kpu_layer_argument_t *)(ctx->model_buffer + first_layer->layer_offset);

            if((layer_arg.image_size.data.i_row_wid + 1) % 64 != 0)
            {
                kpu_kmodel_input_with_padding(&layer_arg, src);
                ai_step_not_isr(ctx);
            } else
            {
                kpu_input_dma(&layer_arg, src, ctx->dma_ch, ai_step, ctx);
            }
        }
        break;
        case KL_FULLY_CONNECTED:
        {
            const kpu_model_fully_connected_layer_argument_t *first_layer = (const kpu_model_fully_connected_layer_argument_t *)ctx->body_start;
            kpu_kmodel_input_float((const float *)src, (float *)(ctx->main_buffer + first_layer->main_mem_in_address), first_layer->in_channels);
            ai_step_not_isr(ctx);
        }
        break;
        default:
            return -1;
    }

    return 0;
}
