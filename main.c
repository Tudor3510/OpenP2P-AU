#include <stdio.h>
#include "openp2pau.h"
#include <windows.h>

static int no_characters_printed = 0;
static const char* CONNECTING_MESSAGE = "Connecting";
static int run_connecting_thread = 1;

#define DOT_INTERVAL 500
#define THREAD_ID 1

#ifdef _WIN32

static void print_connecting(LPVOID lpParam)
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


#endif

int main()
{
    // Declare a critical section
    CRITICAL_SECTION critical_section;
    InitializeCriticalSection(&critical_section);

    HANDLE h_thread;

    h_thread = CreateThread(NULL, 0, print_connecting, &critical_section, 0, NULL);


    int error_code = connect_AU("192.168.0.100", "6779", "Something");

    EnterCriticalSection(&critical_section);
    run_connecting_thread = 0;
    LeaveCriticalSection(&critical_section);

    WaitForSingleObject(h_thread, INFINITE);
    CloseHandle(h_thread);
    DeleteCriticalSection(&critical_section);

    printf("%c", '\r');
    for (int i=1; i<=no_characters_printed; i++)
    {
        printf("%c", ' ');
    }

    printf("%c", '\r');
    printf("%s\n", get_error_message_AU(error_code));

    return 0;
}