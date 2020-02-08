#include "fpioa-config.h"

__attribute__((constructor)) void initialize_kendryte_ide_hook() {
ide_config_fpioa();
}
