#include <stdio.h>
#include "openp2pau.h"


int main()
{
    printf("%d\n", connect_AU("192.168.0.100", "6779", "Something"));

    return 0;
}