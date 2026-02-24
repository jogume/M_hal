/**
 * @file    hal_spi_example.c
 * @brief   Example usage of SPI HAL
 * @details Demonstrates how to use the SPI HAL API with Bridge pattern
 * @author  EswPla HAL Team
 * @date    2026-02-21
 */

#include "hal_spi.h"
#include "hal_init.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Example: Basic SPI communication
 */
void example_basic_spi(void)
{
    hal_status_t status;
    hal_spi_device_t device = HAL_SPI_DEV_0;
    
    printf("\n=== Basic SPI Example ===\n");
    
    /* 1. Configure SPI */
    hal_spi_config_t config = {
        .baudrate = 1000000,                        /* 1 MHz */
        .mode = HAL_SPI_MODE_0,                     /* CPOL=0, CPHA=0 */
        .bit_order = HAL_SPI_BIT_ORDER_MSB_FIRST,   /* MSB first */
        .data_bits = 8                              /* 8-bit data */
    };
    
    /* 2. Initialize SPI device */
    status = hal_spi_init(device, &config);
    if (status != HAL_OK) {
        printf("ERROR: Failed to initialize SPI device %d\n", device);
        return;
    }
    printf("SPI device %d initialized\n", device);
    
    /* 3. Send data */
    uint8_t tx_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    status = hal_spi_send(device, tx_data, sizeof(tx_data), 1000);
    if (status == HAL_OK) {
        printf("Sent %d bytes: ", (int)sizeof(tx_data));
        for (size_t i = 0; i < sizeof(tx_data); i++) {
            printf("0x%02X ", tx_data[i]);
        }
        printf("\n");
    }
    
    /* 4. Receive data */
    uint8_t rx_data[5] = {0};
    status = hal_spi_receive(device, rx_data, sizeof(rx_data), 1000);
    if (status == HAL_OK) {
        printf("Received %d bytes: ", (int)sizeof(rx_data));
        for (size_t i = 0; i < sizeof(rx_data); i++) {
            printf("0x%02X ", rx_data[i]);
        }
        printf("\n");
    }
    
    /* 5. Full-duplex transfer */
    uint8_t tx_transfer[] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t rx_transfer[4] = {0};
    status = hal_spi_transfer(device, tx_transfer, rx_transfer, sizeof(tx_transfer), 1000);
    if (status == HAL_OK) {
        printf("Transfer complete\n");
        printf("  TX: ");
        for (size_t i = 0; i < sizeof(tx_transfer); i++) {
            printf("0x%02X ", tx_transfer[i]);
        }
        printf("\n  RX: ");
        for (size_t i = 0; i < sizeof(rx_transfer); i++) {
            printf("0x%02X ", rx_transfer[i]);
        }
        printf("\n");
    }
    
    /* 6. Get status */
    hal_spi_status_t spi_status;
    status = hal_spi_get_status(device, &spi_status);
    if (status == HAL_OK) {
        printf("SPI Status:\n");
        printf("  State: %d\n", spi_status.state);
        printf("  TX Count: %u\n", spi_status.tx_count);
        printf("  RX Count: %u\n", spi_status.rx_count);
        printf("  Errors: %u\n", spi_status.error_count);
        printf("  Busy: %s\n", spi_status.is_busy ? "Yes" : "No");
    }
    
    /* 7. Deinitialize */
    hal_spi_deinit(device);
    printf("SPI device %d deinitialized\n", device);
}

/**
 * @brief Example: Runtime reconfiguration
 */
void example_reconfigure_spi(void)
{
    hal_status_t status;
    hal_spi_device_t device = HAL_SPI_DEV_1;
    
    printf("\n=== SPI Reconfiguration Example ===\n");
    
    /* Initial configuration */
    hal_spi_config_t config = {
        .baudrate = 500000,
        .mode = HAL_SPI_MODE_0,
        .bit_order = HAL_SPI_BIT_ORDER_MSB_FIRST,
        .data_bits = 8
    };
    
    hal_spi_init(device, &config);
    printf("Initial config: 500kHz, Mode 0\n");
    
    /* Send some data */
    uint8_t data[] = {0x11, 0x22, 0x33};
    hal_spi_send(device, data, sizeof(data), 1000);
    
    /* Reconfigure to different speed and mode */
    config.baudrate = 2000000;  /* 2 MHz */
    config.mode = HAL_SPI_MODE_3;
    
    status = hal_spi_set_config(device, &config);
    if (status == HAL_OK) {
        printf("Reconfigured: 2MHz, Mode 3\n");
    }
    
    /* Send data with new configuration */
    hal_spi_send(device, data, sizeof(data), 1000);
    
    hal_spi_deinit(device);
}

/**
 * @brief Example: Using multiple SPI devices
 */
void example_multiple_devices(void)
{
    printf("\n=== Multiple SPI Devices Example ===\n");
    
    /* Configure first device (e.g., sensor) */
    hal_spi_config_t sensor_config = {
        .baudrate = 1000000,
        .mode = HAL_SPI_MODE_0,
        .bit_order = HAL_SPI_BIT_ORDER_MSB_FIRST,
        .data_bits = 8
    };
    hal_spi_init(HAL_SPI_DEV_0, &sensor_config);
    printf("Device 0 (Sensor): 1MHz, Mode 0\n");
    
    /* Configure second device (e.g., display) */
    hal_spi_config_t display_config = {
        .baudrate = 10000000,  /* 10 MHz */
        .mode = HAL_SPI_MODE_2,
        .bit_order = HAL_SPI_BIT_ORDER_MSB_FIRST,
        .data_bits = 8
    };
    hal_spi_init(HAL_SPI_DEV_1, &display_config);
    printf("Device 1 (Display): 10MHz, Mode 2\n");
    
    /* Communicate with both devices */
    uint8_t sensor_cmd[] = {0x80, 0x00};  /* Read register */
    uint8_t sensor_data[2];
    hal_spi_transfer(HAL_SPI_DEV_0, sensor_cmd, sensor_data, 2, 100);
    printf("Sensor read: 0x%02X 0x%02X\n", sensor_data[0], sensor_data[1]);
    
    uint8_t display_data[] = {0xFF, 0x00, 0xFF, 0x00};  /* Pattern */
    hal_spi_send(HAL_SPI_DEV_1, display_data, sizeof(display_data), 100);
    printf("Display updated\n");
    
    /* Cleanup */
    hal_spi_deinit(HAL_SPI_DEV_0);
    hal_spi_deinit(HAL_SPI_DEV_1);
}

/**
 * @brief Main example entry point
 */
int hal_spi_example_main(void)
{
    printf("=================================================\n");
    printf("SPI HAL Example - Bridge Pattern Implementation\n");
    printf("=================================================\n");
    
    /* Initialize HAL */
    if (hal_init() != HAL_OK) {
        printf("ERROR: Failed to initialize HAL\n");
        return -1;
    }
    
    printf("Using implementation: %s\n", hal_get_implementation_name());
    
    /* Run examples */
    example_basic_spi();
    example_reconfigure_spi();
    example_multiple_devices();
    
    printf("\n=================================================\n");
    printf("All examples completed\n");
    printf("=================================================\n");
    
    return 0;
}
