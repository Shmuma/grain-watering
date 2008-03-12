#include <QtCore>
#include <QtNetwork>
#include "daemon.h"
#include "logger.h"


// --------------------------------------------------
// Daemon
// --------------------------------------------------
Daemon::Daemon (const QString& host, int port)
    : QObject (),
      _host (host),
      _port (port),
      _sock (new QTcpSocket (this)),
      _connected (false),
      _hw_connected (false),
      _last (c_empty)
{
    QObject::connect (_sock, SIGNAL (stateChanged (QAbstractSocket::SocketState)), this, SLOT (socketStateChanged (QAbstractSocket::SocketState)));
    QObject::connect (_sock, SIGNAL (readyRead ()), this, SLOT (socketReadyRead ()));
}


void Daemon::connect ()
{
    Logger::instance ()->log (Logger::Information, QString ("Connecting to daemon at %1:%2").arg (_host, QString::number (_port)));
    _sock->connectToHost (_host, _port);
}


void Daemon::disconnect ()
{
    Logger::instance ()->log (Logger::Debug, "Disconnect from daemon");
    _sock->disconnectFromHost ();
}


void Daemon::socketStateChanged (QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        connectedChanged (false);
        _connected = _hw_connected = false;
        Logger::instance ()->log (Logger::Debug, "Disconnected");
        break;

    case QAbstractSocket::ConnectedState:
        connectedChanged (true);
        _connected = true;
        Logger::instance ()->log (Logger::Debug, "Connected");

        // right after the connect to socket, daemon sends us welcome
        // message and then waits for our input. This state waits for
        // this banner, and after that tryes to check hardware
        // communication. See socketReadyRead function for this.
        _last = c_init;
        break;

    default:
        break;
    }
}


void Daemon::socketReadyRead ()
{
    // read data from socket
    QString reply (_sock->readAll ());
    QString msg;
    int value;
    lastcommand_t p_last = _last;
    QString auto_prefix ("Auto: ");

    if (!reply.startsWith (auto_prefix))
        textArrived (reply);
    else
        autoTextArrived (reply.remove (auto_prefix).trimmed ());
    //    Logger::instance ()->log (Logger::Debug, QString ("Got data from socket. Last command %1, text %2").arg (QString::number (_last), reply));

    _last = c_empty;

    switch (p_last) {
    case c_empty:                 // no command was issued, check for auto mode message
        if (reply.startsWith (auto_prefix)) {
            bool state;
            int pres;
            
            if (parseAutoModeTick (reply, &state, &pres))
                autoModeTickGot (state, pres);
        }
        break;

    case c_init:
        // check hardware communication
        _hw_connected = false;
        sendCommand (QString ("connect\n"));
        _last = c_connect;
        break;

    case c_connect:
        if (!parseGenericReply (reply, msg)) {
            // connect to hardware failed
            _hw_connected = false;
            Logger::instance ()->log (Logger::Error, QString ("Connection to hardware failed. Reason: '%1'").arg (msg));
        }
        else {
            _hw_connected = true;
            Logger::instance ()->log (Logger::Information, QString ("Controller connected to hardware"));
            hardwareConnected ();
        }
        break;
    case c_setstages:
        if (!parseGenericReply (reply, msg))
            Logger::instance ()->log (Logger::Error, QString ("Cannot set active stages. Reason: '%1'").arg (msg));
        else
            stagesActivityChanged (_s1, _s2, _s3, _s4);
        break;
    case c_getgrainflow:
        if (!parseNumberReply (reply, msg, &value))
            Logger::instance ()->log (Logger::Error, QString ("Cannot get grain flow. Reason: '%1'").arg (msg));
        else
            grainFlowGot (_stage, value);
        break;
    case c_startautomode:
        if (!parseGenericReply (reply, msg))
            Logger::instance ()->log (Logger::Error, QString ("Cannot start auto mode. Reason: '%1'").arg (msg));
        else
            autoModeStarted ();
        break;
    case c_stopautomode:
        if (!parseGenericReply (reply, msg))
            Logger::instance ()->log (Logger::Error, QString ("Cannot stop auto mode. Reason: '%1'").arg (msg));
        else
            autoModeStopped ();
        break;
    case c_toggleautomode:
        if (!parseGenericReply (reply, msg))
            Logger::instance ()->log (Logger::Error, QString ("Cannot toggle auto mode. Reason: '%1'").arg (msg));
        else
            autoModeToggled (!msg.contains ("unpaused"));
        break;
    case c_getautomode:
        autoModeGot (reply.split (",")[0] == "active", reply.split (",")[1] == "paused");
        break;
    case c_getmetastate:
        handleMetaState (reply);
        break;
    }
}


bool Daemon::parseGenericReply (const QString& reply, QString& msg)
{
    bool res = reply.startsWith ("OK");

    if (!res)
        msg = QString (reply).remove ("ERROR:").remove ('>').trimmed ();
    else
        msg = QString (reply).remove ("OK:").remove ('>').trimmed ();

    return res;
}


bool Daemon::parseNumberReply (const QString& reply, QString& msg, int* val)
{
    if (reply.startsWith ("ERROR:")) {
        msg = QString (reply).remove ("ERROR:").remove ('>').trimmed ();
        return false;
    }
        
    bool ok;
    QString str = QString (reply).remove (">").trimmed ();
    *val = str.toInt (&ok);

    if (!ok)
        msg = QString (tr ("Expected number, got '%1'")).arg (reply);

    return ok;
}


bool Daemon::parseAutoModeTick (const QString& reply, bool* state, int* press)
{
    QStringList l = QString (reply).remove (" ").split (",");

    *state = l[0].split (":")[1] == "OK";
    *press = l[1].split (":")[1].toInt ();

    return true;
}


void Daemon::sendCommand (const QString& cmd)
{
    commandSent (cmd.trimmed ());
    _sock->write (cmd.toAscii ());
}


void Daemon::sendRawCommand (const QString& text)
{
    sendCommand (text);
}


void Daemon::setStages (bool s1, bool s2, bool s3, bool s4)
{
    _s1 = s1; _s2 = s2; _s3 = s3; _s4 = s4;
    sendCommand (QString ().sprintf ("setstages %d %d %d %d\n", s1 ? 1 : 0, s2 ? 1 : 0, s3 ? 1 : 0, s4 ? 1 : 0));
    _last = c_setstages;
}


void Daemon::getGrainFlow (int stage)
{
    _stage = stage;
    sendCommand (QString ("getgrainflow %d\n").arg (QString::number (_stage)));
    _last = c_getgrainflow;
}


void Daemon::startAutoMode ()
{
    sendCommand ("startautomode\n");
    _last = c_startautomode;
}


void Daemon::stopAutoMode ()
{
    sendCommand ("stopautomode\n");
    _last = c_stopautomode;
}


void Daemon::toggleAutoMode ()
{
    sendCommand ("toggleautomode\n");
    _last = c_toggleautomode;
}


void Daemon::getAutoMode ()
{
    sendCommand ("getautomode\n");
    _last = c_getautomode;
}


void Daemon::refreshState ()
{
    sendCommand (QString ().sprintf ("getmetastate %d %d %d %d\n", _s1 ? 1 : 0, _s2 ? 1 : 0, _s3 ? 1 : 0, _s4 ? 1 : 0));
    _last = c_getmetastate;
}


bool Daemon::handleMetaState (const QString& msg)
{
    QStringList l = QString (msg).trimmed ().split (",");
    int water_pres = 0;
    QString s;
    QStringList ll;
    QMap<int, QList<int> > vals;

    QStringList::iterator it = l.begin ();

    while (it != l.end ()) {
        s = *it;
        ll = s.trimmed ().split (":", QString::SkipEmptyParts);
        
        int index = ll[0].toInt ();
        s = ll[1];
        ll = s.trimmed ().split (" ", QString::SkipEmptyParts);
        
        QStringList::iterator it2 = ll.begin ();
        
        while (it2 != ll.end ()) {
            int val = (*it2).split ("=")[1].toInt ();
            
            if (index == 0)
                water_pres = val;
            else
                vals[index].push_back (val);

            it2++;
        }

        it++;
    }

    metaStateGot (water_pres, vals);

    return true;
}
