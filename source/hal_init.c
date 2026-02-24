/**
 * @file    hal_init.c
 * @brief   HAL Initialization and Configuration
 * @details Initializes the HAL with the appropriate implementation based on build configuration
 * @author  EswPla HAL Team
 * @date    2026-02-21
 */

#include "yolpiya.h"
#include "hal_spi.h"

/*============================================================================*/
/* External Operations Declarations                                           */
/*============================================================================*/

/* Declare all available operations */
extern const hal_spi_ops_t hal_spi_stm32_ops;
extern const hal_spi_ops_t hal_spi_rh850_ops;
extern const hal_spi_ops_t hal_spi_sim_ops;
extern const hal_spi_ops_t hal_spi_socket_ops;

/*============================================================================*/
/* Public Functions                                                           */
/*============================================================================*/

/**
 * @brief Initialize HAL subsystem
 * @details Registers the appropriate HAL implementation based on build configuration
 * @return HAL_OK on success, error code otherwise
 */
hal_status_t hal_init(void)
{
    hal_status_t status;
    const char* impl_name = "Unknown";
    
    /* Select implementation based on compile-time configuration */
#if defined(STM32_TARGET)
    status = hal_spi_register_ops(&hal_spi_stm32_ops);
    impl_name = "STM32-Nucleo";
    
#elif defined(RH850_TARGET)
    status = hal_spi_register_ops(&hal_spi_rh850_ops);
    impl_name = "RH850";
    
#elif defined(HAL_USE_SOCKET)
    status = hal_spi_register_ops(&hal_spi_socket_ops);
    impl_name = "Socket";
    
#else
    /* Default to simulation */
    status = hal_spi_register_ops(&hal_spi_sim_ops);
    impl_name = "Simulation";
#endif
    
    if (status == HAL_OK) {
        printf("[HAL] Initialized with %s implementation\n", impl_name);
    } else {
        printf("[HAL] ERROR: Failed to initialize HAL\n");
    }
    
    return status;
}

/**
 * @brief Get the name of the current HAL implementation
 * @return String describing the current implementation
 */
const char* hal_get_implementation_name(void)
{
#if defined(STM32_TARGET)
    return "STM32-Nucleo";
#elif defined(RH850_TARGET)
    return "RH850";
#elif defined(HAL_USE_SOCKET)
    return "Socket";
#else
    return "Simulation";
#endif
}
