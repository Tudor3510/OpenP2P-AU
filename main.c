#include <stdio.h>
#include "openp2pau.h"


#ifdef _WIN32

#include <windows.h>

#else

#include <pthread.h>
#include <string.h>
#include <unistd.h>


#define US_MULTIPLIER 1000


#endif

#define DOT_INTERVAL 500

static int no_characters_printed = 0;
static const char* CONNECTING_MESSAGE = "Connecting";
static int run_connecting_thread = 1;





void print_connection_message(const char* message)
{
    printf("%c", '\r');
    for (int i=1; i<=no_characters_printed; i++)
    {
        printf("%c", ' ');
    }

    printf("%c", '\r');
    printf("%s\n", message);
}


#ifdef _WIN32

static void print_connecting(void* lpParam)
{
    CRITICAL_SECTION* critical_section = (CRITICAL_SECTION*) lpParam;

    EnterCriticalSection(critical_section);
    run_connecting_thread = 1;
    LeaveCriticalSection(critical_section);

    printf("%s", CONNECTING_MESSAGE);
    no_characters_printed += strlen(CONNECTING_MESSAGE);
    while (1)
    {
        printf(".");
        no_characters_printed += 1;

        Sleep(DOT_INTERVAL);

        EnterCriticalSection(critical_section);
        if (run_connecting_thread == 0)
        {
            LeaveCriticalSection(critical_section);
            break;
        }
        LeaveCriticalSection(critical_section);

    }    
}

int main()
{
    // Declare a critical section
    CRITICAL_SECTION critical_section;
    InitializeCriticalSection(&critical_section);


    HANDLE h_thread = CreateThread(NULL, 0, print_connecting, &critical_section, 0, NULL);
    if (h_thread == NULL)
    {
        printf("Error creating message thread\n");
        DeleteCriticalSection(&critical_section);
        return 1;
    }

    int error_code = connect_AU("192.168.0.100", "6779", "Something");

    EnterCriticalSection(&critical_section);
    run_connecting_thread = 0;
    LeaveCriticalSection(&critical_section);

    WaitForSingleObject(h_thread, INFINITE);
    CloseHandle(h_thread);
    DeleteCriticalSection(&critical_section);

    print_connection_message(get_error_message_AU(error_code));

    return 0;
}

#else

static void print_connecting(void* lpParam)
{
    pthread_mutex_t * critical_section = (pthread_mutex_t *) lpParam;

    pthread_mutex_lock(critical_section);
    run_connecting_thread = 1;
    pthread_mutex_unlock(critical_section);

    printf("%s", CONNECTING_MESSAGE);
    fflush(stdout);
    no_characters_printed += strlen(CONNECTING_MESSAGE);
    while (1)
    {
        printf(".");
        fflush(stdout);
        no_characters_printed += 1;

        usleep(DOT_INTERVAL * US_MULTIPLIER);

        pthread_mutex_lock(critical_section);
        if (run_connecting_thread == 0)
        {
            pthread_mutex_unlock(critical_section);
            break;
        }
        pthread_mutex_unlock(critical_section);

    }    
}

int main()
{
    pthread_t pthread;
    pthread_mutex_t critical_section = PTHREAD_MUTEX_INITIALIZER;


    int creation_result = pthread_create( &pthread, NULL, (void *) print_connecting, &critical_section);
    if (creation_result != 0)
    {
        printf("Error creating message thread\n");
        pthread_mutex_destroy(&critical_section);
        return 1;
    }

    int error_code = connect_AU("192.168.0.100", "6779", "Something");

    pthread_mutex_lock(&critical_section);
    run_connecting_thread = 0;
    pthread_mutex_unlock(&critical_section);

    pthread_join(pthread, NULL);
    pthread_mutex_destroy(&critical_section);

    print_connection_message(get_error_message_AU(error_code));

    return 0;
}

#endif