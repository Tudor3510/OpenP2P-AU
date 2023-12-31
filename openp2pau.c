#include <stdio.h>
#include "openp2pau.h"

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#else

#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#define US_MULTIPLIER 1000
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#endif

#define MAX_DATA_SIZE 150
#define AU_PORT 5555
#define MAXIMUM_RETRIES 6
#define PAUSE_TIME 950


#define SUCCESS 0
const static char* SUCCESS_TEXT = "Connected successfully";

#define SOCKET_INIT_ERROR 1
const static char* SOCKET_INIT_ERROR_TEXT = "Error initializing the socket";

#define SOCKET_BIND_ERROR 2
const static char* SOCKET_BIND_ERROR_TEXT = "Error binding the socket to the required port"
                                            "\nMake sure that Among Us is not running in the local or online games menu"
                                            "\nFor safety, you can completely close Among Us while connecting";

#define HOSTNAME_RESOLUTION_ERROR 3
const static char* HOSTNAME_RESOLUTION_ERROR_TEXT = "Could not resolve the hostname";

#define NO_PACKET_SENT 4
const static char* NO_PACKET_SENT_TEXT = "Can not send the request for initializing connection";

#define CONNECTION_FAILED 5
const static char* CONNECTION_FAILED_TEXT = "Connection failed";


const static char* INITIAL_MESSAGE = "connect~";
const static char* RECEIVE_MESSAGE = ".....";


static char send_data[MAX_DATA_SIZE + 1];
static char received_data[MAX_DATA_SIZE + 1];

#ifdef _WIN32

static void close_socket(SOCKET* socket)
{
    if (socket != NULL)
    {
        closesocket(*socket);
    }
    WSACleanup();
}

static int initialize_socket_lib()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return 1;
    }
    return 0;
}

static int set_nonblocking_socket(SOCKET* socket)
{
    u_long nonBlockingMode = 1;
    return ioctlsocket(*socket, FIONBIO, &nonBlockingMode);
}

static void custom_pause(unsigned int time)
{
    Sleep(time);
}

#else

static void close_socket(SOCKET* socket)
{
    if (socket != NULL)
        close(*socket);
}

static int initialize_socket_lib()
{
    return 0;
}

static int set_nonblocking_socket(SOCKET* socket)
{
    int flags = fcntl(*socket, F_GETFL, 0);
    if (flags == -1) {
        return SOCKET_ERROR;
    }

    flags |= O_NONBLOCK;

    if (fcntl(*socket, F_SETFL, flags) == -1) {
        return SOCKET_ERROR;
    }

    return 0;
}

static void custom_pause(unsigned int time)
{
    usleep(time * US_MULTIPLIER);
}

#endif

int connect_AU(const char* ip, const char* port, const char* client_name)
{
    if (initialize_socket_lib() != 0)
        return SOCKET_INIT_ERROR;

    // Create a UDP socket
    SOCKET udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket == INVALID_SOCKET) {
        close_socket(NULL);
        return SOCKET_INIT_ERROR;
    }

    // Set the socket to non-blocking mode
    
    if (set_nonblocking_socket(&udp_socket) == SOCKET_ERROR) {
        close_socket(&udp_socket);
        return SOCKET_INIT_ERROR;
    }

    // Set up the local address information to bind to AU port
    struct sockaddr_in localAddr;
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(AU_PORT); // AU port
    localAddr.sin_addr.s_addr = INADDR_ANY; // Bind to any available local address

    // Bind the UDP socket to the local address
    if (bind(udp_socket, (struct sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        close_socket(&udp_socket);
        return SOCKET_BIND_ERROR;
    }


    struct addrinfo* addr_result = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_ADDRCONFIG|AI_V4MAPPED;
    hints.ai_family = AF_INET; // Use AF_INET for IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    int getAddressResult = getaddrinfo(ip, port, &hints, &addr_result);
    if (getAddressResult != 0 || addr_result == NULL) {
        close_socket(&udp_socket);
        return HOSTNAME_RESOLUTION_ERROR;
    }

    struct sockaddr* server_addr = addr_result->ai_addr;

    // Data to be sent
    int data_size = strlen(INITIAL_MESSAGE);
    strcpy(send_data, INITIAL_MESSAGE);
    strncpy(send_data + data_size, client_name, MAX_DATA_SIZE - data_size);
    send_data[MAX_DATA_SIZE] = 0;
    data_size = strlen(send_data);

    int no_failed_packets = 0;
    int sent_bytes;
    int received_bytes;

    // Send the UDP packets
    int connected = 0;
    for (int i=1; i <= MAXIMUM_RETRIES && !connected; i++)
    {
        sent_bytes = sendto(udp_socket, send_data, data_size, 0, server_addr, sizeof(*server_addr));
        if (sent_bytes <= 0)
        {
            no_failed_packets += 1;
        }

        custom_pause(PAUSE_TIME);

        received_bytes = recvfrom(udp_socket, received_data, MAX_DATA_SIZE, 0, NULL, NULL);
        if (received_bytes > 0)
        {
            int bytes_to_compare =  received_bytes;
            if (bytes_to_compare > strlen(RECEIVE_MESSAGE))
            {
                bytes_to_compare = strlen(RECEIVE_MESSAGE);
            }

            if (strncmp(received_data, RECEIVE_MESSAGE, bytes_to_compare) == 0)
            {
                connected = 1;
            }
        }

    }

    close_socket(&udp_socket);
    freeaddrinfo(addr_result);


    if (no_failed_packets == MAXIMUM_RETRIES)
        return NO_PACKET_SENT;

    if (!connected)
        return CONNECTION_FAILED;

    return SUCCESS;
}

const char* get_error_message_AU(int error_code)
{
    switch (error_code)
    {
    case SUCCESS:
        return SUCCESS_TEXT;

    case SOCKET_INIT_ERROR:
        return SOCKET_INIT_ERROR_TEXT;

    case SOCKET_BIND_ERROR:
        return SOCKET_BIND_ERROR_TEXT;

    case HOSTNAME_RESOLUTION_ERROR:
        return HOSTNAME_RESOLUTION_ERROR_TEXT;

    case NO_PACKET_SENT:
        return NO_PACKET_SENT_TEXT;
    
    case CONNECTION_FAILED:
        return CONNECTION_FAILED_TEXT;
    
    default:
        return NULL;
    }
}