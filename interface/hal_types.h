/**
 * @file    hal_types.h
 * @brief   Common HAL type definitions
 * @author  EswPla HAL Team
 * @date    2026-02-21
 */

#ifndef HAL_TYPES_H
#define HAL_TYPES_H

#include "yolpiya.h"

/**
 * @brief HAL return codes
 */
typedef enum {
    HAL_OK                  = 0,    /**< Operation successful */
    HAL_ERROR               = -1,   /**< General error */
    HAL_ERROR_BUSY          = -2,   /**< Device busy */
    HAL_ERROR_TIMEOUT       = -3,   /**< Operation timed out */
    HAL_ERROR_INVALID_PARAM = -4,   /**< Invalid parameter */
    HAL_ERROR_NOT_INIT      = -5,   /**< Device not initialized */
    HAL_ERROR_NO_DATA       = -6    /**< No data available */
} hal_status_t;

/**
 * @brief HAL device state
 */
typedef enum {
    HAL_STATE_RESET         = 0,    /**< Device not initialized */
    HAL_STATE_READY         = 1,    /**< Device ready for operation */
    HAL_STATE_BUSY          = 2,    /**< Device busy */
    HAL_STATE_ERROR         = 3     /**< Device in error state */
} hal_state_t;

#endif /* HAL_TYPES_H */
