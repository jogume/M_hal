/**
 * @file    hal_spi.c
 * @brief   SPI HAL Bridge Implementation
 * @details Implements the abstraction layer that delegates to concrete implementations
 * @author  EswPla HAL Team
 * @date    2026-02-21
 */

#include "yolpiya.h"
#include "hal_spi.h"

/*============================================================================*/
/* Private Variables                                                          */
/*============================================================================*/

/**
 * @brief Registered SPI operations (Bridge Pattern - pointer to Implementor)
 */
static const hal_spi_ops_t* g_spi_ops = NULL;

/*============================================================================*/
/* Public API Implementation                                                  */
/*============================================================================*/

/**
 * @brief Register SPI operations implementation
 */
hal_status_t hal_spi_register_ops(const hal_spi_ops_t* ops)
{
    if (ops == NULL) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    /* Verify all required operations are implemented */
    if (ops->init == NULL || ops->deinit == NULL || 
        ops->transfer == NULL || ops->send == NULL ||
        ops->receive == NULL || ops->set_config == NULL ||
        ops->get_status == NULL) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    g_spi_ops = ops;
    return HAL_OK;
}

/**
 * @brief Initialize SPI device
 */
hal_status_t hal_spi_init(hal_spi_device_t device, const hal_spi_config_t* config)
{
    if (g_spi_ops == NULL) {
        return HAL_ERROR_NOT_INIT;
    }
    
    if (device >= HAL_SPI_MAX_INTERFACES || config == NULL) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    return g_spi_ops->init(device, config);
}

/**
 * @brief Deinitialize SPI device
 */
hal_status_t hal_spi_deinit(hal_spi_device_t device)
{
    if (g_spi_ops == NULL) {
        return HAL_ERROR_NOT_INIT;
    }
    
    if (device >= HAL_SPI_MAX_INTERFACES) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    return g_spi_ops->deinit(device);
}

/**
 * @brief Full-duplex SPI transfer
 */
hal_status_t hal_spi_transfer(hal_spi_device_t device, 
                              const uint8_t* tx_data, 
                              uint8_t* rx_data, 
                              uint16_t length, 
                              uint32_t timeout_ms)
{
    if (g_spi_ops == NULL) {
        return HAL_ERROR_NOT_INIT;
    }
    
    if (device >= HAL_SPI_MAX_INTERFACES || tx_data == NULL || rx_data == NULL || length == 0) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    return g_spi_ops->transfer(device, tx_data, rx_data, length, timeout_ms);
}

/**
 * @brief Send data only
 */
hal_status_t hal_spi_send(hal_spi_device_t device, 
                          const uint8_t* data, 
                          uint16_t length, 
                          uint32_t timeout_ms)
{
    if (g_spi_ops == NULL) {
        return HAL_ERROR_NOT_INIT;
    }
    
    if (device >= HAL_SPI_MAX_INTERFACES || data == NULL || length == 0) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    return g_spi_ops->send(device, data, length, timeout_ms);
}

/**
 * @brief Receive data only
 */
hal_status_t hal_spi_receive(hal_spi_device_t device, 
                             uint8_t* data, 
                             uint16_t length, 
                             uint32_t timeout_ms)
{
    if (g_spi_ops == NULL) {
        return HAL_ERROR_NOT_INIT;
    }
    
    if (device >= HAL_SPI_MAX_INTERFACES || data == NULL || length == 0) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    return g_spi_ops->receive(device, data, length, timeout_ms);
}

/**
 * @brief Set SPI configuration
 */
hal_status_t hal_spi_set_config(hal_spi_device_t device, 
                                const hal_spi_config_t* config)
{
    if (g_spi_ops == NULL) {
        return HAL_ERROR_NOT_INIT;
    }
    
    if (device >= HAL_SPI_MAX_INTERFACES || config == NULL) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    return g_spi_ops->set_config(device, config);
}

/**
 * @brief Get SPI device status
 */
hal_status_t hal_spi_get_status(hal_spi_device_t device, 
                                hal_spi_status_t* status)
{
    if (g_spi_ops == NULL) {
        return HAL_ERROR_NOT_INIT;
    }
    
    if (device >= HAL_SPI_MAX_INTERFACES || status == NULL) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    return g_spi_ops->get_status(device, status);
}
