# Hardware Abstraction Layer (HAL) Module

This module provides a hardware abstraction layer using the **Bridge Design Pattern** to isolate hardware-dependent code from the application layer.

## Architecture

```
Application Layer
      ↓ (uses abstract interface)
HAL Abstraction (hal_spi.c)
      ↓ (delegates to)
HAL Implementation (hw/sim/socket)
      ↓ (accesses)
Hardware / Simulation / Network
```

## Features

- **Bridge Pattern**: Separates abstraction from implementation
- **Multiple Implementations**: STM32, RH850, Simulation, Socket
- **Runtime Selection**: Choose implementation at compile-time
- **7 SPI Interfaces**: Limited to requirements (init, deinit, transfer, send, receive, set_config, get_status)
- **Socket Server**: Python-based server for HIL testing

## Supported Implementations

### 1. Simulation (`hal_spi_sim.c`)
- PC-based simulation (no hardware required)
- In-memory buffers
- Loopback mode
- Default implementation

### 2. STM32-Nucleo (`hal_spi_stm32.c`)
- STM32 microcontroller support
- Template for STM32 HAL mapping
- Requires STM32 HAL libraries

### 3. RH850 (`hal_spi_rh850.c`)
- Renesas RH850 microcontroller support
- CSIH peripheral template
- Requires RH850 device headers

### 4. Socket (`hal_spi_socket.c`)
- TCP/IP socket communication
- Connects to external server (Python/C++)
- Hardware-in-the-Loop (HIL) testing
- Remote device simulation

## Building

Select the HAL implementation by setting `HAL_IMPLEMENTATION`:

```bash
# Build with simulation (default)
make

# Build with STM32 implementation
make HAL_IMPLEMENTATION=STM32

# Build with RH850 implementation
make HAL_IMPLEMENTATION=RH850

# Build with socket implementation
make HAL_IMPLEMENTATION=SOCKET
```

## Socket Server Usage

Start the Python socket server:

```bash
cd M_hal/tools
python spi_socket_server.py --host 127.0.0.1 --port 9000
```

Set environment variables for socket configuration (optional):
```bash
set HAL_SPI_SOCKET_HOST=192.168.1.100
set HAL_SPI_SOCKET_PORT=9000
```

## API Usage

```c
#include "hal_spi.h"

// Register implementation (done at startup)
hal_spi_register_ops(&hal_spi_sim_ops);  // or hal_spi_stm32_ops, etc.

// Configure SPI
hal_spi_config_t config = {
    .baudrate = 1000000,
    .mode = HAL_SPI_MODE_0,
    .bit_order = HAL_SPI_BIT_ORDER_MSB_FIRST,
    .data_bits = 8
};

// Initialize device
hal_spi_init(HAL_SPI_DEV_0, &config);

// Transfer data
uint8_t tx_data[4] = {0x01, 0x02, 0x03, 0x04};
uint8_t rx_data[4];
hal_spi_transfer(HAL_SPI_DEV_0, tx_data, rx_data, 4, 1000);

// Get status
hal_spi_status_t status;
hal_spi_get_status(HAL_SPI_DEV_0, &status);
```

## Directory Structure

```
M_hal/
├── interface/           # Public headers
│   ├── hal_types.h      # Common types
│   └── hal_spi.h        # SPI abstract interface
├── source/              # Implementation files
│   ├── hal_spi.c        # Bridge implementation
│   ├── hal_spi_stm32.c  # STM32 implementation
│   ├── hal_spi_rh850.c  # RH850 implementation
│   ├── hal_spi_sim.c    # Simulation implementation
│   └── hal_spi_socket.c # Socket implementation
├── make/
│   └── default/
│       └── m_module.mak # Build configuration
└── tools/
    └── spi_socket_server.py  # Socket server application
```

## Adding New Hardware Support

1. Create new implementation file (e.g., `hal_spi_custom.c`)
2. Implement all 7 operations in `hal_spi_ops_t` structure
3. Export operations: `const hal_spi_ops_t hal_spi_custom_ops = {...}`
4. Update `m_module.mak` to include new file
5. Register at runtime: `hal_spi_register_ops(&hal_spi_custom_ops)`
