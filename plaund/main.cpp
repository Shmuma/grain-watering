#include <QtCore>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "serial.h"
#include "device.h"
#include "shell.h"


int main ()
{
    try {
//         SerialPort* port = new SerialRecorder (new FileSerialPort ("input.dat", "output.dat"), 
//                                                QString ("trace.dat"));
	SerialPort* port = new FileSerialPort ("input.dat", "output.dat");
        Device device (port);

//         while (1) {
//             if (!device.initialize ())
//                 printf ("Device not initialized, sleep for 1 second\n");
//             else 
//                 break;
//             sleep (1);
//         }
        
//         printf ("Device initialized\n");

//         while (1) {
//             device.updateState ();
            
//             if (device.isManualMode ())
//                 printf ("Device is in manual mode, sleep for 1 second\n");
//             else
//                 break;
//             sleep (1);
//         }

//         printf ("Device in auto mode, do job\n");
	Interpreter interp (&device);

	// prepare descriptors
	QString l, r;
	QTextStream s_in (stdin), s_out (stdout);

	s_out << "\nWelcome to plaund interactive shell.\nUse 'help' to see list of commands.\n\n";

	while (s_in.status () == QTextStream::Ok) {
	    s_out << "> ";
	    s_out.flush ();
	    l = s_in.readLine ();

	    if (l.isNull ())
		break;

	    r = interp.exec (l);
	    s_out << r;
	    s_out.flush ();
	}

	s_out << endl;
        delete port;

    } catch (QString msg) {
        printf ("Error: %s\n", msg.toAscii ().constData ());
    }

    return 0;
}
