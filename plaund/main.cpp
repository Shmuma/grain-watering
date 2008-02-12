#include <QtCore>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server.h"

int main ()
{
    PlaundServer server (12345);

    server.startProcessing ();

    return 0;
}
