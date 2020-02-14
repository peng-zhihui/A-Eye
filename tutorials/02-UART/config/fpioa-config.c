#include <fpioa.h>
#include "fpioa-config.h"

int ide_config_fpioa() {
int ret = 0;

ret += fpioa_set_function(4, FUNC_UART1_RX);
ret += fpioa_set_function(5, FUNC_UART1_TX);

return ret;
}
