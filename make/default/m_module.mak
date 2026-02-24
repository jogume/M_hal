###############################################################################
#                                                                             #
#     Module Description:  Hardware Abstraction Layer (HAL) module            #
#     Module Name:         M_hal                                              #
#                                                                             #
###############################################################################

#------------------------- Module information --------------------------------#
MODULE_NAME            = M_hal
MODULE_PATH            = source
MODULE_HEADER_PATH     = interface
MODULE_STATUS          = public
PRE_COMPILE_VARIANT    = default

#---------------------------------------------------------------------------------------------------------------------------#
# Configuration: Select HAL implementation
# Options: SIM (simulation), STM32 (STM32-Nucleo), RH850 (Renesas RH850), SOCKET (socket server)
#---------------------------------------------------------------------------------------------------------------------------#
HAL_IMPLEMENTATION ?= SIM

#---------------------------------------------------------------------------------------------------------------------------#
# Objects - Core HAL files (always compiled)
#---------------------------------------------------------------------------------------------------------------------------#
OBJ_QAC   	 = hal_spi.o \
               hal_init.o

# Uncomment to include example code
# OBJ_QAC += hal_spi_example.o

#---------------------------------------------------------------------------------------------------------------------------#
# Objects - Implementation-specific files (conditional)
#---------------------------------------------------------------------------------------------------------------------------#
ifeq ($(HAL_IMPLEMENTATION),STM32)
    OBJ_QAC += hal_spi_stm32.o
    COMPILER_DEFINE_PROJECT += -DSTM32_TARGET
else ifeq ($(HAL_IMPLEMENTATION),RH850)
    OBJ_QAC += hal_spi_rh850.o
    COMPILER_DEFINE_PROJECT += -DRH850_TARGET
else ifeq ($(HAL_IMPLEMENTATION),SOCKET)
    OBJ_QAC += hal_spi_socket.o
    COMPILER_DEFINE_PROJECT += -DHAL_USE_SOCKET
else
    # Default to simulation
    OBJ_QAC += hal_spi_sim.o
    COMPILER_DEFINE_PROJECT += -DHAL_USE_SIM
endif

#---------------------------------------------------------------------------------------------------------------------------#
# Assembly objects (none for now)
#---------------------------------------------------------------------------------------------------------------------------#
OBJ_ASM      =

#---------------------------------------------------------------------------------------------------------------------------#
# Non-QAC objects (none for now)
#---------------------------------------------------------------------------------------------------------------------------#
OBJ_NOQAC 	 =

#---------------------------------------------------------------------------------------------------------------------------#
# Additional compiler options
#---------------------------------------------------------------------------------------------------------------------------#
# Uncomment for Windows socket support
# LINKER_ADDITIONAL_OPTIONS += -lws2_32
