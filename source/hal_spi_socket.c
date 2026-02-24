/**
 * @file    hal_spi_socket.c
 * @brief   SPI HAL Socket Implementation
 * @details Concrete implementation using TCP/IP socket for remote/HIL testing
 * @note    Connects to external socket server (Python/C++) to feed/receive SPI data
 * @author  EswPla HAL Team
 * @date    2026-02-21
 */

#include "yolpiya.h"
#include "hal_spi.h"

/* Platform-specific socket includes */
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
    #define SOCKET_INVALID INVALID_SOCKET
    #define socket_close closesocket
    #define socket_error() WSAGetLastError()
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <errno.h>
    typedef int socket_t;
    #define SOCKET_INVALID -1
    #define socket_close close
    #define socket_error() errno
#endif

/*============================================================================*/
/* Private Definitions                                                        */
/*============================================================================*/

#define SOCKET_SERVER_DEFAULT_HOST  "127.0.0.1"
#define SOCKET_SERVER_DEFAULT_PORT  "9000"
#define SOCKET_RX_BUFFER_SIZE       4096
#define SOCKET_CONNECT_RETRY_COUNT  3
#define SOCKET_CONNECT_RETRY_DELAY_MS 1000

/**
 * @brief SPI protocol message types
 */
typedef enum {
    SOCKET_MSG_INIT         = 0x01,
    SOCKET_MSG_DEINIT       = 0x02,
    SOCKET_MSG_TRANSFER     = 0x03,
    SOCKET_MSG_SEND         = 0x04,
    SOCKET_MSG_RECEIVE      = 0x05,
    SOCKET_MSG_SET_CONFIG   = 0x06,
    SOCKET_MSG_GET_STATUS   = 0x07,
    SOCKET_MSG_RESPONSE     = 0x80
} socket_msg_type_t;

/**
 * @brief Socket message header
 */
typedef struct __attribute__((packed)) {
    uint8_t     msg_type;       /**< Message type */
    uint8_t     device_id;      /**< SPI device ID */
    uint16_t    data_length;    /**< Payload length */
    uint32_t    sequence;       /**< Sequence number */
} socket_msg_header_t;

/**
 * @brief Socket SPI device state
 */
typedef struct {
    bool                is_initialized;
    hal_spi_config_t    config;
    hal_spi_status_t    status;
    
    /* Socket-specific data */
    socket_t            socket_fd;      /**< Socket file descriptor */
    bool                is_connected;   /**< Connection state */
    uint32_t            msg_sequence;   /**< Message sequence counter */
    uint8_t             rx_buffer[SOCKET_RX_BUFFER_SIZE];
    char                server_host[64];
    char                server_port[8];
} socket_spi_device_t;

/*============================================================================*/
/* Private Variables                                                          */
/*============================================================================*/

static socket_spi_device_t g_socket_spi_devices[HAL_SPI_MAX_INTERFACES] = {0};
static bool g_socket_initialized = false;

/*============================================================================*/
/* Private Helper Functions                                                   */
/*============================================================================*/

/**
 * @brief Initialize socket subsystem
 */
static hal_status_t socket_initialize_subsystem(void)
{
    if (!g_socket_initialized) {
#ifdef _WIN32
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            printf("[SOCKET-SPI] ERROR: WSAStartup failed\n");
            return HAL_ERROR;
        }
#endif
        g_socket_initialized = true;
        printf("[SOCKET-SPI] Socket subsystem initialized\n");
    }
    return HAL_OK;
}

/**
 * @brief Connect to socket server
 */
static hal_status_t socket_connect(socket_spi_device_t* dev)
{
    struct addrinfo hints, *result = NULL, *ptr = NULL;
    int connect_result;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    /* Resolve server address */
    if (getaddrinfo(dev->server_host, dev->server_port, &hints, &result) != 0) {
        printf("[SOCKET-SPI] ERROR: Failed to resolve %s:%s\n", 
               dev->server_host, dev->server_port);
        return HAL_ERROR;
    }
    
    /* Attempt to connect to an address until one succeeds */
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        /* Create socket */
        dev->socket_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (dev->socket_fd == SOCKET_INVALID) {
            continue;
        }
        
        /* Connect to server */
        connect_result = connect(dev->socket_fd, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (connect_result == 0) {
            break;  /* Success */
        }
        
        socket_close(dev->socket_fd);
        dev->socket_fd = SOCKET_INVALID;
    }
    
    freeaddrinfo(result);
    
    if (dev->socket_fd == SOCKET_INVALID) {
        printf("[SOCKET-SPI] ERROR: Unable to connect to server\n");
        return HAL_ERROR;
    }
    
    dev->is_connected = true;
    printf("[SOCKET-SPI] Connected to %s:%s\n", dev->server_host, dev->server_port);
    
    return HAL_OK;
}

/**
 * @brief Send message to socket server
 */
static hal_status_t socket_send_message(socket_spi_device_t* dev, 
                                        socket_msg_type_t msg_type,
                                        const uint8_t* payload, 
                                        uint16_t payload_length)
{
    if (!dev->is_connected) {
        return HAL_ERROR_NOT_INIT;
    }
    
    /* Prepare message header */
    socket_msg_header_t header;
    header.msg_type = msg_type;
    header.device_id = 0;  /* Will be set by caller if needed */
    header.data_length = payload_length;
    header.sequence = dev->msg_sequence++;
    
    /* Send header */
    int bytes_sent = send(dev->socket_fd, (const char*)&header, sizeof(header), 0);
    if (bytes_sent != sizeof(header)) {
        printf("[SOCKET-SPI] ERROR: Failed to send header\n");
        return HAL_ERROR;
    }
    
    /* Send payload if present */
    if (payload_length > 0 && payload != NULL) {
        bytes_sent = send(dev->socket_fd, (const char*)payload, payload_length, 0);
        if (bytes_sent != payload_length) {
            printf("[SOCKET-SPI] ERROR: Failed to send payload\n");
            return HAL_ERROR;
        }
    }
    
    return HAL_OK;
}

/**
 * @brief Receive message from socket server
 */
static hal_status_t socket_receive_message(socket_spi_device_t* dev, 
                                           uint8_t* data, 
                                           uint16_t* length,
                                           uint32_t timeout_ms)
{
    if (!dev->is_connected) {
        return HAL_ERROR_NOT_INIT;
    }
    
    /* Set receive timeout */
#ifdef _WIN32
    DWORD timeout = timeout_ms;
    setsockopt(dev->socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(dev->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
    
    /* Receive header */
    socket_msg_header_t header;
    int bytes_received = recv(dev->socket_fd, (char*)&header, sizeof(header), 0);
    if (bytes_received != sizeof(header)) {
        return HAL_ERROR_TIMEOUT;
    }
    
    /* Receive payload */
    if (header.data_length > 0 && data != NULL) {
        bytes_received = recv(dev->socket_fd, (char*)data, header.data_length, 0);
        if (bytes_received != header.data_length) {
            return HAL_ERROR;
        }
        *length = header.data_length;
    } else {
        *length = 0;
    }
    
    return HAL_OK;
}

/*============================================================================*/
/* SPI Operations Implementation (Socket)                                     */
/*============================================================================*/

static hal_status_t socket_spi_init(hal_spi_device_t device, const hal_spi_config_t* config)
{
    if (device >= HAL_SPI_MAX_INTERFACES || config == NULL) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    if (socket_initialize_subsystem() != HAL_OK) {
        return HAL_ERROR;
    }
    
    socket_spi_device_t* dev = &g_socket_spi_devices[device];
    
    if (dev->is_initialized) {
        return HAL_ERROR_BUSY;
    }
    
    /* Store configuration */
    dev->config = *config;
    dev->status.state = HAL_STATE_RESET;
    dev->status.tx_count = 0;
    dev->status.rx_count = 0;
    dev->status.error_count = 0;
    dev->status.is_busy = false;
    dev->socket_fd = SOCKET_INVALID;
    dev->is_connected = false;
    dev->msg_sequence = 0;
    
    /* Set default server address (can be overridden via environment variables) */
    const char* host_env = getenv("HAL_SPI_SOCKET_HOST");
    const char* port_env = getenv("HAL_SPI_SOCKET_PORT");
    
    strncpy(dev->server_host, host_env ? host_env : SOCKET_SERVER_DEFAULT_HOST, 
            sizeof(dev->server_host) - 1);
    strncpy(dev->server_port, port_env ? port_env : SOCKET_SERVER_DEFAULT_PORT, 
            sizeof(dev->server_port) - 1);
    
    /* Connect to server */
    if (socket_connect(dev) != HAL_OK) {
        printf("[SOCKET-SPI] WARNING: Running in disconnected mode\n");
        /* Continue anyway - can retry connection later */
    }
    
    /* Send init message to server */
    if (dev->is_connected) {
        socket_send_message(dev, SOCKET_MSG_INIT, (uint8_t*)config, sizeof(hal_spi_config_t));
    }
    
    dev->is_initialized = true;
    dev->status.state = HAL_STATE_READY;
    
    printf("[SOCKET-SPI] Init device %d via socket\n", device);
    
    return HAL_OK;
}

static hal_status_t socket_spi_deinit(hal_spi_device_t device)
{
    if (device >= HAL_SPI_MAX_INTERFACES) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    socket_spi_device_t* dev = &g_socket_spi_devices[device];
    
    if (!dev->is_initialized) {
        return HAL_ERROR_NOT_INIT;
    }
    
    /* Send deinit message */
    if (dev->is_connected) {
        socket_send_message(dev, SOCKET_MSG_DEINIT, NULL, 0);
        socket_close(dev->socket_fd);
    }
    
    printf("[SOCKET-SPI] Deinit device %d\n", device);
    
    memset(dev, 0, sizeof(socket_spi_device_t));
    dev->socket_fd = SOCKET_INVALID;
    
    return HAL_OK;
}

static hal_status_t socket_spi_transfer(hal_spi_device_t device, 
                                        const uint8_t* tx_data, 
                                        uint8_t* rx_data, 
                                        uint16_t length, 
                                        uint32_t timeout_ms)
{
    if (device >= HAL_SPI_MAX_INTERFACES) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    socket_spi_device_t* dev = &g_socket_spi_devices[device];
    
    if (!dev->is_initialized) {
        return HAL_ERROR_NOT_INIT;
    }
    
    if (!dev->is_connected) {
        printf("[SOCKET-SPI] ERROR: Not connected to server\n");
        return HAL_ERROR_NOT_INIT;
    }
    
    if (dev->status.is_busy) {
        return HAL_ERROR_BUSY;
    }
    
    dev->status.is_busy = true;
    
    /* Send transfer request */
    if (socket_send_message(dev, SOCKET_MSG_TRANSFER, tx_data, length) != HAL_OK) {
        dev->status.error_count++;
        dev->status.is_busy = false;
        return HAL_ERROR;
    }
    
    /* Receive response */
    uint16_t rx_length = 0;
    hal_status_t status = socket_receive_message(dev, rx_data, &rx_length, timeout_ms);
    
    if (status != HAL_OK || rx_length != length) {
        dev->status.error_count++;
        dev->status.is_busy = false;
        return (status == HAL_ERROR_TIMEOUT) ? HAL_ERROR_TIMEOUT : HAL_ERROR;
    }
    
    dev->status.tx_count += length;
    dev->status.rx_count += length;
    dev->status.is_busy = false;
    
    printf("[SOCKET-SPI] Transferred %d bytes on device %d\n", length, device);
    
    return HAL_OK;
}

static hal_status_t socket_spi_send(hal_spi_device_t device, 
                                    const uint8_t* data, 
                                    uint16_t length, 
                                    uint32_t timeout_ms)
{
    if (device >= HAL_SPI_MAX_INTERFACES) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    socket_spi_device_t* dev = &g_socket_spi_devices[device];
    
    if (!dev->is_initialized || !dev->is_connected) {
        return HAL_ERROR_NOT_INIT;
    }
    
    if (dev->status.is_busy) {
        return HAL_ERROR_BUSY;
    }
    
    dev->status.is_busy = true;
    
    /* Send data */
    hal_status_t status = socket_send_message(dev, SOCKET_MSG_SEND, data, length);
    
    if (status != HAL_OK) {
        dev->status.error_count++;
        dev->status.is_busy = false;
        return status;
    }
    
    /* Wait for acknowledgment */
    uint16_t rx_length = 0;
    status = socket_receive_message(dev, NULL, &rx_length, timeout_ms);
    
    dev->status.tx_count += length;
    dev->status.is_busy = false;
    
    printf("[SOCKET-SPI] Sent %d bytes on device %d\n", length, device);
    
    return status;
}

static hal_status_t socket_spi_receive(hal_spi_device_t device, 
                                       uint8_t* data, 
                                       uint16_t length, 
                                       uint32_t timeout_ms)
{
    if (device >= HAL_SPI_MAX_INTERFACES) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    socket_spi_device_t* dev = &g_socket_spi_devices[device];
    
    if (!dev->is_initialized || !dev->is_connected) {
        return HAL_ERROR_NOT_INIT;
    }
    
    if (dev->status.is_busy) {
        return HAL_ERROR_BUSY;
    }
    
    dev->status.is_busy = true;
    
    /* Send receive request */
    uint8_t req_data[2] = {(uint8_t)(length >> 8), (uint8_t)(length & 0xFF)};
    if (socket_send_message(dev, SOCKET_MSG_RECEIVE, req_data, 2) != HAL_OK) {
        dev->status.error_count++;
        dev->status.is_busy = false;
        return HAL_ERROR;
    }
    
    /* Receive data */
    uint16_t rx_length = 0;
    hal_status_t status = socket_receive_message(dev, data, &rx_length, timeout_ms);
    
    if (status != HAL_OK) {
        dev->status.error_count++;
        dev->status.is_busy = false;
        return status;
    }
    
    dev->status.rx_count += rx_length;
    dev->status.is_busy = false;
    
    printf("[SOCKET-SPI] Received %d bytes on device %d\n", rx_length, device);
    
    return HAL_OK;
}

static hal_status_t socket_spi_set_config(hal_spi_device_t device, 
                                          const hal_spi_config_t* config)
{
    if (device >= HAL_SPI_MAX_INTERFACES || config == NULL) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    socket_spi_device_t* dev = &g_socket_spi_devices[device];
    
    if (!dev->is_initialized || !dev->is_connected) {
        return HAL_ERROR_NOT_INIT;
    }
    
    if (dev->status.is_busy) {
        return HAL_ERROR_BUSY;
    }
    
    /* Update local configuration */
    dev->config = *config;
    
    /* Send config update to server */
    socket_send_message(dev, SOCKET_MSG_SET_CONFIG, (uint8_t*)config, sizeof(hal_spi_config_t));
    
    printf("[SOCKET-SPI] Reconfigured device %d\n", device);
    
    return HAL_OK;
}

static hal_status_t socket_spi_get_status(hal_spi_device_t device, 
                                          hal_spi_status_t* status)
{
    if (device >= HAL_SPI_MAX_INTERFACES || status == NULL) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    socket_spi_device_t* dev = &g_socket_spi_devices[device];
    
    if (!dev->is_initialized) {
        return HAL_ERROR_NOT_INIT;
    }
    
    *status = dev->status;
    return HAL_OK;
}

/*============================================================================*/
/* Public Operations Structure (Export)                                       */
/*============================================================================*/

const hal_spi_ops_t hal_spi_socket_ops = {
    .init       = socket_spi_init,
    .deinit     = socket_spi_deinit,
    .transfer   = socket_spi_transfer,
    .send       = socket_spi_send,
    .receive    = socket_spi_receive,
    .set_config = socket_spi_set_config,
    .get_status = socket_spi_get_status
};
