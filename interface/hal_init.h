/**
 * @file    hal_init.h
 * @brief   HAL Initialization and Configuration Header
 * @author  EswPla HAL Team
 * @date    2026-02-21
 */

#ifndef HAL_INIT_H
#define HAL_INIT_H

#include "hal_types.h"

/**
 * @brief Initialize HAL subsystem
 * @details Registers the appropriate HAL implementation based on build configuration
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_init(void);

/**
 * @brief Get the name of the current HAL implementation
 * @return String describing the current implementation
 */
const char* hal_get_implementation_name(void);

#endif /* HAL_INIT_H */
