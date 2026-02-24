/**
 * @file    hal_spi_sim.c
 * @brief   SPI HAL Simulation Implementation
 * @details Concrete implementation for PC-based simulation (no hardware required)
 * @note    Uses in-memory buffers to simulate SPI communication
 * @author  EswPla HAL Team
 * @date    2026-02-21
 */

#include "yolpiya.h"
#include "hal_spi.h"
#include <time.h>

/*============================================================================*/
/* Private Definitions                                                        */
/*============================================================================*/

#define SIM_RX_BUFFER_SIZE  1024

/**
 * @brief Simulated SPI device state
 */
typedef struct {
    bool                is_initialized;
    hal_spi_config_t    config;
    hal_spi_status_t    status;
    
    /* Simulation-specific data */
    uint8_t             rx_buffer[SIM_RX_BUFFER_SIZE];  /**< Simulated RX data */
    uint16_t            rx_buffer_head;                 /**< RX buffer write position */
    uint16_t            rx_buffer_tail;                 /**< RX buffer read position */
    uint32_t            last_transfer_ms;               /**< Timestamp of last transfer */
} sim_spi_device_t;

/*============================================================================*/
/* Private Variables                                                          */
/*============================================================================*/

static sim_spi_device_t g_sim_spi_devices[HAL_SPI_MAX_INTERFACES] = {0};
static bool g_sim_initialized = false;

/*============================================================================*/
/* Private Helper Functions                                                   */
/*============================================================================*/

/**
 * @brief Initialize simulation environment
 */
static void sim_initialize_environment(void)
{
    if (!g_sim_initialized) {
        srand((unsigned int)time(NULL));
        g_sim_initialized = true;
        printf("[SIM-SPI] Simulation environment initialized\n");
    }
}

/**
 * @brief Add data to simulated RX buffer
 */
static void sim_add_rx_data(sim_spi_device_t* dev, const uint8_t* data, uint16_t length)
{
    for (uint16_t i = 0; i < length && dev->rx_buffer_head < SIM_RX_BUFFER_SIZE; i++) {
        dev->rx_buffer[dev->rx_buffer_head++] = data[i];
    }
}

/**
 * @brief Get data from simulated RX buffer
 */
static uint16_t sim_get_rx_data(sim_spi_device_t* dev, uint8_t* data, uint16_t length)
{
    uint16_t bytes_read = 0;
    
    while (bytes_read < length && dev->rx_buffer_tail < dev->rx_buffer_head) {
        data[bytes_read++] = dev->rx_buffer[dev->rx_buffer_tail++];
    }
    
    /* Reset buffer positions if empty */
    if (dev->rx_buffer_tail >= dev->rx_buffer_head) {
        dev->rx_buffer_head = 0;
        dev->rx_buffer_tail = 0;
    }
    
    return bytes_read;
}

/**
 * @brief Simulate SPI transfer delay based on baudrate
 */
static void sim_transfer_delay(const hal_spi_config_t* config, uint16_t length)
{
    /* Calculate transfer time in microseconds */
    /* Time = (bits / baudrate) * 1,000,000 */
    if (config->baudrate > 0) {
        uint32_t bits = length * config->data_bits;
        uint32_t time_us = (bits * 1000000UL) / config->baudrate;
        
        /* For simulation purposes, we don't actually delay */
        /* but we could add: usleep(time_us); on POSIX systems */
        (void)time_us;
    }
}

/*============================================================================*/
/* SPI Operations Implementation (Simulation)                                 */
/*============================================================================*/

static hal_status_t sim_spi_init(hal_spi_device_t device, const hal_spi_config_t* config)
{
    if (device >= HAL_SPI_MAX_INTERFACES || config == NULL) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    sim_initialize_environment();
    
    sim_spi_device_t* dev = &g_sim_spi_devices[device];
    
    if (dev->is_initialized) {
        return HAL_ERROR_BUSY;
    }
    
    /* Store configuration */
    dev->config = *config;
    dev->status.state = HAL_STATE_READY;
    dev->status.tx_count = 0;
    dev->status.rx_count = 0;
    dev->status.error_count = 0;
    dev->status.is_busy = false;
    
    /* Initialize simulation buffers */
    dev->rx_buffer_head = 0;
    dev->rx_buffer_tail = 0;
    memset(dev->rx_buffer, 0, SIM_RX_BUFFER_SIZE);
    dev->last_transfer_ms = 0;
    
    dev->is_initialized = true;
    
    printf("[SIM-SPI] Init device %d: %lu Hz, mode %d, %d-bit\n", 
           device, config->baudrate, config->mode, config->data_bits);
    
    return HAL_OK;
}

static hal_status_t sim_spi_deinit(hal_spi_device_t device)
{
    if (device >= HAL_SPI_MAX_INTERFACES) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    sim_spi_device_t* dev = &g_sim_spi_devices[device];
    
    if (!dev->is_initialized) {
        return HAL_ERROR_NOT_INIT;
    }
    
    printf("[SIM-SPI] Deinit device %d (TX: %u, RX: %u, Errors: %u)\n", 
           device, dev->status.tx_count, dev->status.rx_count, dev->status.error_count);
    
    memset(dev, 0, sizeof(sim_spi_device_t));
    return HAL_OK;
}

static hal_status_t sim_spi_transfer(hal_spi_device_t device, 
                                     const uint8_t* tx_data, 
                                     uint8_t* rx_data, 
                                     uint16_t length, 
                                     uint32_t timeout_ms)
{
    if (device >= HAL_SPI_MAX_INTERFACES) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    sim_spi_device_t* dev = &g_sim_spi_devices[device];
    
    if (!dev->is_initialized) {
        return HAL_ERROR_NOT_INIT;
    }
    
    if (dev->status.is_busy) {
        return HAL_ERROR_BUSY;
    }
    
    dev->status.is_busy = true;
    
    /* Simulate transfer delay */
    sim_transfer_delay(&dev->config, length);
    
    /* In simulation, echo back the transmitted data with optional modification */
    for (uint16_t i = 0; i < length; i++) {
        /* Add some variation to simulate real device response */
        rx_data[i] = tx_data[i] ^ 0x00;  /* For now, just echo */
    }
    
    dev->status.tx_count += length;
    dev->status.rx_count += length;
    dev->status.is_busy = false;
    dev->last_transfer_ms = (uint32_t)time(NULL);
    
    printf("[SIM-SPI] Transferred %d bytes on device %d (timeout=%u ms)\n", 
           length, device, timeout_ms);
    
    return HAL_OK;
}

static hal_status_t sim_spi_send(hal_spi_device_t device, 
                                 const uint8_t* data, 
                                 uint16_t length, 
                                 uint32_t timeout_ms)
{
    if (device >= HAL_SPI_MAX_INTERFACES) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    sim_spi_device_t* dev = &g_sim_spi_devices[device];
    
    if (!dev->is_initialized) {
        return HAL_ERROR_NOT_INIT;
    }
    
    if (dev->status.is_busy) {
        return HAL_ERROR_BUSY;
    }
    
    dev->status.is_busy = true;
    
    /* Simulate transfer delay */
    sim_transfer_delay(&dev->config, length);
    
    /* In simulation mode, store sent data as potential RX data (loopback) */
    sim_add_rx_data(dev, data, length);
    
    dev->status.tx_count += length;
    dev->status.is_busy = false;
    dev->last_transfer_ms = (uint32_t)time(NULL);
    
    printf("[SIM-SPI] Sent %d bytes on device %d (timeout=%u ms)\n", 
           length, device, timeout_ms);
    
    return HAL_OK;
}

static hal_status_t sim_spi_receive(hal_spi_device_t device, 
                                    uint8_t* data, 
                                    uint16_t length, 
                                    uint32_t timeout_ms)
{
    if (device >= HAL_SPI_MAX_INTERFACES) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    sim_spi_device_t* dev = &g_sim_spi_devices[device];
    
    if (!dev->is_initialized) {
        return HAL_ERROR_NOT_INIT;
    }
    
    if (dev->status.is_busy) {
        return HAL_ERROR_BUSY;
    }
    
    dev->status.is_busy = true;
    
    /* Simulate transfer delay */
    sim_transfer_delay(&dev->config, length);
    
    /* Get data from simulated RX buffer */
    uint16_t bytes_read = sim_get_rx_data(dev, data, length);
    
    /* Fill remaining with random data if buffer was empty */
    for (uint16_t i = bytes_read; i < length; i++) {
        data[i] = (uint8_t)(rand() & 0xFF);
    }
    
    dev->status.rx_count += length;
    dev->status.is_busy = false;
    dev->last_transfer_ms = (uint32_t)time(NULL);
    
    printf("[SIM-SPI] Received %d bytes on device %d (timeout=%u ms)\n", 
           length, device, timeout_ms);
    
    return HAL_OK;
}

static hal_status_t sim_spi_set_config(hal_spi_device_t device, 
                                       const hal_spi_config_t* config)
{
    if (device >= HAL_SPI_MAX_INTERFACES || config == NULL) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    sim_spi_device_t* dev = &g_sim_spi_devices[device];
    
    if (!dev->is_initialized) {
        return HAL_ERROR_NOT_INIT;
    }
    
    if (dev->status.is_busy) {
        return HAL_ERROR_BUSY;
    }
    
    /* Update configuration */
    dev->config = *config;
    
    printf("[SIM-SPI] Reconfigured device %d: %lu Hz, mode %d\n", 
           device, config->baudrate, config->mode);
    
    return HAL_OK;
}

static hal_status_t sim_spi_get_status(hal_spi_device_t device, 
                                       hal_spi_status_t* status)
{
    if (device >= HAL_SPI_MAX_INTERFACES || status == NULL) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    sim_spi_device_t* dev = &g_sim_spi_devices[device];
    
    if (!dev->is_initialized) {
        return HAL_ERROR_NOT_INIT;
    }
    
    *status = dev->status;
    return HAL_OK;
}

/*============================================================================*/
/* Public Operations Structure (Export)                                       */
/*============================================================================*/

const hal_spi_ops_t hal_spi_sim_ops = {
    .init       = sim_spi_init,
    .deinit     = sim_spi_deinit,
    .transfer   = sim_spi_transfer,
    .send       = sim_spi_send,
    .receive    = sim_spi_receive,
    .set_config = sim_spi_set_config,
    .get_status = sim_spi_get_status
};
