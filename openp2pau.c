#include <stdio.h>
#include "openp2pau.h"

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#else

#define US_MULTIPLIER = 1000
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#endif

#define MAX_DATA_SIZE 150
#define AU_PORT 5555
#define MAXIMUM_RETRIES 6
#define PAUSE_TIME 950

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

static void pause(unsigned int time)
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

static void pause(unsigned int time)
{
    usleep(time * US_MULTIPLIER);
}

#endif

int connect_AU(const char* ip, int port, const char* client_name)
{
    if (initialize_socket_lib() != 0)
        return 1;

    // Create a UDP socket
    SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udpSocket == INVALID_SOCKET) {
        close_socket(NULL);
        return 1;
    }

    // Set the socket to non-blocking mode
    u_long nonBlockingMode = 1;
    if (ioctlsocket(udpSocket, FIONBIO, &nonBlockingMode) == SOCKET_ERROR) {
        close_socket(&udpSocket);
        return 1;
    }

    // Set up the local address information to bind to AU port
    struct sockaddr_in localAddr;
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(AU_PORT); // AU port
    localAddr.sin_addr.s_addr = INADDR_ANY; // Bind to any available local address

    // Bind the UDP socket to the local address
    if (bind(udpSocket, (struct sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        close_socket(&udpSocket);
        return 2;
    }

    // Set up the destination address information
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port); // Replace with the destination port number
    serverAddr.sin_addr.s_addr = inet_addr(ip); // Replace with the destination IP address

    // Data to be sent
    int data_size = strlen(INITIAL_MESSAGE);
    strcpy(send_data, INITIAL_MESSAGE);
    strncpy(send_data + data_size, client_name, MAX_DATA_SIZE - data_size);
    data_size = strlen(send_data);

    int no_failed_packets = 0;
    int sent_bytes;
    int received_bytes;

    // Send the UDP packets
    int connected = 0;
    for (int i=1; i <= MAXIMUM_RETRIES && !connected; i++)
    {
        sent_bytes = sendto(udpSocket, send_data, data_size, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (sent_bytes <= 0)
        {
            no_failed_packets += 1;
        }

        pause(PAUSE_TIME);

        received_bytes = recvfrom(udpSocket, received_data, MAX_DATA_SIZE, 0, NULL, NULL);
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

    close_socket(&udpSocket);


    if (no_failed_packets == MAXIMUM_RETRIES)
        return 3;

    if (!connected)
        return 4;

    return 0;
}

