/**
 * @file    hal_spi.h
 * @brief   SPI Hardware Abstraction Layer - Abstract Interface
 * @details This file defines the abstract interface for SPI HAL using Bridge pattern.
 *          Supports STM32-Nucleo, RH850, simulation, and socket-based implementations.
 * @author  EswPla HAL Team
 * @date    2026-02-21
 */

#ifndef HAL_SPI_H
#define HAL_SPI_H

#include "hal_types.h"

/**
 * @brief Maximum number of SPI interfaces (limited to 7 as per requirements)
 */
#define HAL_SPI_MAX_INTERFACES  7

/**
 * @brief SPI device identifier
 */
typedef enum {
    HAL_SPI_DEV_0 = 0,
    HAL_SPI_DEV_1 = 1,
    HAL_SPI_DEV_2 = 2,
    HAL_SPI_DEV_3 = 3,
    HAL_SPI_DEV_4 = 4,
    HAL_SPI_DEV_5 = 5,
    HAL_SPI_DEV_6 = 6
} hal_spi_device_t;

/**
 * @brief SPI mode configuration
 */
typedef enum {
    HAL_SPI_MODE_0 = 0,  /**< CPOL=0, CPHA=0 */
    HAL_SPI_MODE_1 = 1,  /**< CPOL=0, CPHA=1 */
    HAL_SPI_MODE_2 = 2,  /**< CPOL=1, CPHA=0 */
    HAL_SPI_MODE_3 = 3   /**< CPOL=1, CPHA=1 */
} hal_spi_mode_t;

/**
 * @brief SPI bit order
 */
typedef enum {
    HAL_SPI_BIT_ORDER_MSB_FIRST = 0,
    HAL_SPI_BIT_ORDER_LSB_FIRST = 1
} hal_spi_bit_order_t;

/**
 * @brief SPI configuration structure
 */
typedef struct {
    uint32_t              baudrate;      /**< SPI clock frequency in Hz */
    hal_spi_mode_t        mode;          /**< SPI mode (0-3) */
    hal_spi_bit_order_t   bit_order;     /**< Bit order (MSB/LSB first) */
    uint8_t               data_bits;     /**< Data bits (8, 16, 32) */
} hal_spi_config_t;

/**
 * @brief SPI status structure
 */
typedef struct {
    hal_state_t  state;           /**< Current state */
    uint32_t     tx_count;        /**< Total transmitted bytes */
    uint32_t     rx_count;        /**< Total received bytes */
    uint32_t     error_count;     /**< Total errors */
    bool         is_busy;         /**< Busy flag */
} hal_spi_status_t;

/**
 * @brief Forward declaration of operations structure
 */
typedef struct hal_spi_ops hal_spi_ops_t;

/**
 * @brief SPI operations interface (Bridge Pattern - Implementor)
 * @details These function pointers are implemented by concrete hardware drivers
 */
struct hal_spi_ops {
    /**
     * @brief Initialize SPI device
     * @param device SPI device identifier
     * @param config Configuration parameters
     * @return HAL_OK on success, error code otherwise
     */
    hal_status_t (*init)(hal_spi_device_t device, const hal_spi_config_t* config);
    
    /**
     * @brief Deinitialize SPI device
     * @param device SPI device identifier
     * @return HAL_OK on success, error code otherwise
     */
    hal_status_t (*deinit)(hal_spi_device_t device);
    
    /**
     * @brief Full-duplex SPI transfer (simultaneous read/write)
     * @param device SPI device identifier
     * @param tx_data Data to transmit
     * @param rx_data Buffer for received data
     * @param length Number of bytes to transfer
     * @param timeout_ms Timeout in milliseconds (0 = no timeout)
     * @return HAL_OK on success, error code otherwise
     */
    hal_status_t (*transfer)(hal_spi_device_t device, 
                             const uint8_t* tx_data, 
                             uint8_t* rx_data, 
                             uint16_t length, 
                             uint32_t timeout_ms);
    
    /**
     * @brief Send data only (write operation)
     * @param device SPI device identifier
     * @param data Data to transmit
     * @param length Number of bytes to send
     * @param timeout_ms Timeout in milliseconds (0 = no timeout)
     * @return HAL_OK on success, error code otherwise
     */
    hal_status_t (*send)(hal_spi_device_t device, 
                         const uint8_t* data, 
                         uint16_t length, 
                         uint32_t timeout_ms);
    
    /**
     * @brief Receive data only (read operation)
     * @param device SPI device identifier
     * @param data Buffer for received data
     * @param length Number of bytes to receive
     * @param timeout_ms Timeout in milliseconds (0 = no timeout)
     * @return HAL_OK on success, error code otherwise
     */
    hal_status_t (*receive)(hal_spi_device_t device, 
                            uint8_t* data, 
                            uint16_t length, 
                            uint32_t timeout_ms);
    
    /**
     * @brief Set SPI configuration (reconfigure at runtime)
     * @param device SPI device identifier
     * @param config New configuration parameters
     * @return HAL_OK on success, error code otherwise
     */
    hal_status_t (*set_config)(hal_spi_device_t device, 
                               const hal_spi_config_t* config);
    
    /**
     * @brief Get SPI device status
     * @param device SPI device identifier
     * @param status Pointer to status structure to fill
     * @return HAL_OK on success, error code otherwise
     */
    hal_status_t (*get_status)(hal_spi_device_t device, 
                               hal_spi_status_t* status);
};

/*============================================================================*/
/* Public API Functions (Bridge Pattern - Abstraction)                        */
/*============================================================================*/

/**
 * @brief Register SPI operations implementation
 * @details This function allows runtime selection of the SPI implementation
 *          (hardware, simulation, socket, etc.)
 * @param ops Pointer to operations structure
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_spi_register_ops(const hal_spi_ops_t* ops);

/**
 * @brief Initialize SPI device
 * @param device SPI device identifier (0-6)
 * @param config Configuration parameters
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_spi_init(hal_spi_device_t device, const hal_spi_config_t* config);

/**
 * @brief Deinitialize SPI device
 * @param device SPI device identifier
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_spi_deinit(hal_spi_device_t device);

/**
 * @brief Full-duplex SPI transfer
 * @param device SPI device identifier
 * @param tx_data Data to transmit
 * @param rx_data Buffer for received data
 * @param length Number of bytes to transfer
 * @param timeout_ms Timeout in milliseconds
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_spi_transfer(hal_spi_device_t device, 
                              const uint8_t* tx_data, 
                              uint8_t* rx_data, 
                              uint16_t length, 
                              uint32_t timeout_ms);

/**
 * @brief Send data only
 * @param device SPI device identifier
 * @param data Data to transmit
 * @param length Number of bytes to send
 * @param timeout_ms Timeout in milliseconds
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_spi_send(hal_spi_device_t device, 
                          const uint8_t* data, 
                          uint16_t length, 
                          uint32_t timeout_ms);

/**
 * @brief Receive data only
 * @param device SPI device identifier
 * @param data Buffer for received data
 * @param length Number of bytes to receive
 * @param timeout_ms Timeout in milliseconds
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_spi_receive(hal_spi_device_t device, 
                             uint8_t* data, 
                             uint16_t length, 
                             uint32_t timeout_ms);

/**
 * @brief Set SPI configuration
 * @param device SPI device identifier
 * @param config New configuration parameters
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_spi_set_config(hal_spi_device_t device, 
                                const hal_spi_config_t* config);

/**
 * @brief Get SPI device status
 * @param device SPI device identifier
 * @param status Pointer to status structure to fill
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_spi_get_status(hal_spi_device_t device, 
                                hal_spi_status_t* status);

#endif /* HAL_SPI_H */
