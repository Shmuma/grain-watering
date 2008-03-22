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
      _hw_connected (false)
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
        _queue.push_back (DaemonCommand (DaemonCommand::c_init));
        break;

    default:
        break;
    }
}


void Daemon::socketReadyRead ()
{
    QString reply, msg;
    double value;
    QString auto_prefix ("Auto: ");
    QString prompt_prefix ("> ");
    QStringList replies;
    bool s1, s2, s3, s4;

    reply = _data_buf + _sock->readAll ();

    if (!reply.contains (prompt_prefix)) {
        _data_buf = reply;
        return;
    }

    // split read data using prompt as separator
    replies = reply.split (prompt_prefix, QString::SkipEmptyParts);
    
    for (int i = 0; i < replies.size (); i++) {
        reply = replies[i];

        if (!reply.startsWith (auto_prefix)) {
            textArrived (reply+prompt_prefix);

            if (!_queue.isEmpty ()) {
                DaemonCommand cmd = _queue.front ();
                _queue.pop_front ();

                switch (cmd.kind ()) {
                case DaemonCommand::c_empty:
                    break;

                case DaemonCommand::c_init:
                    _hw_connected = false;
                    sendCommand (QString ("connect\n"));
                    _queue.push_back (DaemonCommand (DaemonCommand::c_connect));
                    break;

                case DaemonCommand::c_connect:
                    if (!parseGenericReply (reply, msg)) {
                        // connect to hardware failed
                        _hw_connected = false;
                        Logger::instance ()->log (Logger::Error, tr ("Connection to hardware failed. Reason: '%1'").arg (msg));
                    }
                    else {
                        _hw_connected = true;
                        Logger::instance ()->log (Logger::Information, tr ("Controller connected to hardware"));
                        hardwareConnected ();
                    }
                    break;
                case DaemonCommand::c_setstages:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot set active stages. Reason: '%1'").arg (msg));
                    else
                        stagesActivityChanged (_s1, _s2, _s3, _s4);
                    break;
                case DaemonCommand::c_getgrainflow:
                    if (!parseNumberReply (reply, msg, &value))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot get grain flow. Reason: '%1'").arg (msg));
                    else
                        grainFlowGot (cmd.stage (), value);
                    break;
                case DaemonCommand::c_startautomode:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot start auto mode. Reason: '%1'").arg (msg));
                    else
                        autoModeStarted ();
                    break;
                case DaemonCommand::c_stopautomode:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot stop auto mode. Reason: '%1'").arg (msg));
                    else
                        autoModeStopped ();
                    break;
                case DaemonCommand::c_toggleautomode:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot toggle auto mode. Reason: '%1'").arg (msg));
                    else
                        autoModeToggled (!msg.contains ("unpaused"));
                    break;
                case DaemonCommand::c_getautomode:
                    autoModeGot (reply.split (",")[0] == "active", reply.split (",")[1] == "paused");
                    break;
                case DaemonCommand::c_getmetastate:
                    handleMetaState (reply);
                    break;
                case DaemonCommand::c_startwater:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot start water at stage %1. Reason: '%2'").arg (QString::number (cmd.stage ()), msg));
                    else
                        waterStarted (cmd.stage ());
                    break;
                case DaemonCommand::c_stopwater:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot stop water at stage %1. Reason: '%2'").arg (QString::number (cmd.stage ()), msg));
                    else
                        waterStopped (cmd.stage ());
                    break;
                case DaemonCommand::c_getstages:
                    if (!parseStagesReply (reply, msg, s1, s2, s3, s4))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot get active stages. Reason: '%1'").arg (msg));
                    else {
                        stagesActivityChanged (s1, s2, s3, s4);
                        _s1 = s1; _s2 = s2; _s3 = s3; _s4 = s4;
                    }
                    break;
                case DaemonCommand::c_getgrainsensors:
                    grainSensorsPresenceGot (reply.startsWith ("TRUE"));
                    break;
                case DaemonCommand::c_setgrainsensors:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot change grain sensors presence. Reason: '%1'").arg (msg));
                    else 
                        grainSensorsPresenceGot (cmd.stage () != 0);
                    break;
                case DaemonCommand::c_isgrainpresent:
                    grainPresenceGot (cmd.stage (), reply.startsWith ("TRUE"));
                    break;
                }
            }
        }
        else {
            bool state;
            double pres;

            autoTextArrived (QString (reply).remove (auto_prefix).trimmed ());
            if (parseAutoModeTick (reply, &state, &pres))
                autoModeTickGot (state, pres);
        }

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


bool Daemon::parseNumberReply (const QString& reply, QString& msg, double* val)
{
    if (reply.startsWith ("ERROR:")) {
        msg = QString (reply).remove ("ERROR:").remove ('>').trimmed ();
        return false;
    }
        
    bool ok;
    QString str = QString (reply).remove (">").trimmed ();
    *val = str.toDouble (&ok);

    if (!ok)
        msg = tr ("Expected number, got '%1'").arg (reply);

    return ok;
}


bool Daemon::parseStagesReply (const QString& reply, QString& msg, bool& s1, bool& s2, bool& s3, bool& s4)
{
    if (reply.startsWith ("ERROR:")) {
        msg = QString (reply).remove ("ERROR:").remove ('>').trimmed ();
        return false;
    }

    QStringList lst = QString (reply).remove ('>').trimmed ().split (',', QString::SkipEmptyParts);
    bool ok;
    int val;

    s1 = s2 = s3 = s4 = false;

    for (int i = 0; i < lst.size (); i++) {
        val = lst[i].toInt (&ok);

        if (!ok) {
            msg = tr ("Expected number, got '%1'").arg (lst[i]);
            return false;
        }

        switch (val) {
        case 1:
            s1 = true; break;
        case 2:
            s2 = true; break;
        case 3:
            s3 = true; break;
        case 4:
            s4 = true; break;
        }
    }

    return true;
}



bool Daemon::parseAutoModeTick (const QString& reply, bool* state, double* press)
{
    QStringList l = QString (reply).remove (" ").split (",");

    *state = l[0].split (":")[1] == "OK";
    *press = l[1].split (":")[1].toDouble ();

    return true;
}


void Daemon::sendCommand (const QString& cmd)
{
    commandSent (cmd.trimmed () + "\n");
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
    _queue.push_back (DaemonCommand (DaemonCommand::c_setstages));
}


void Daemon::getGrainFlow (int stage)
{
    sendCommand (QString ("getgrainflow %d\n").arg (QString::number (stage)));
    _queue.push_back (DaemonCommand (DaemonCommand::c_getgrainflow, stage));
}


void Daemon::startAutoMode ()
{
    sendCommand ("startautomode\n");
    _queue.push_back (DaemonCommand (DaemonCommand::c_startautomode));
}


void Daemon::stopAutoMode ()
{
    sendCommand ("stopautomode\n");
    _queue.push_back (DaemonCommand (DaemonCommand::c_stopautomode));
}


void Daemon::toggleAutoMode ()
{
    sendCommand ("toggleautomode\n");
    _queue.push_back (DaemonCommand (DaemonCommand::c_toggleautomode));
}


void Daemon::getAutoMode ()
{
    sendCommand ("getautomode\n");
    _queue.push_back (DaemonCommand (DaemonCommand::c_getautomode));
}


void Daemon::refreshState ()
{
    sendCommand (QString ().sprintf ("getmetastate %d %d %d %d\n", _s1 ? 1 : 0, _s2 ? 1 : 0, _s3 ? 1 : 0, _s4 ? 1 : 0));
    _queue.push_back (DaemonCommand (DaemonCommand::c_getmetastate));
}


bool Daemon::handleMetaState (const QString& msg)
{
    QStringList l = QString (msg).trimmed ().split (",");
    double water_pres = 0;
    QString s;
    QStringList ll;
    QMap<int, QList<double> > vals;

    QStringList::iterator it = l.begin ();

    while (it != l.end ()) {
        s = *it;
        ll = s.trimmed ().split (":", QString::SkipEmptyParts);

        if (ll.size () == 2) {
            int index = ll[0].toInt ();
            s = ll[1];
            ll = s.trimmed ().split (" ", QString::SkipEmptyParts);
        
            QStringList::iterator it2 = ll.begin ();
        
            while (it2 != ll.end ()) {
                if (it2->contains ("=")) {
                    double val = (*it2).split ("=")[1].toDouble ();
            
                    if (index == 0)
                        water_pres = val;
                    else
                        vals[index].push_back (val);
                }

                it2++;
            }
        }

        it++;
    }

    metaStateGot (water_pres, vals);

    return true;
}


void Daemon::startWater (int stage)
{
    sendCommand (QString ().sprintf ("startwater %d\n", stage));
    _queue.push_back (DaemonCommand (DaemonCommand::c_startwater, stage));
}


void Daemon::stopWater (int stage)
{
    sendCommand (QString ().sprintf ("stopwater %d\n", stage));
    _queue.push_back (DaemonCommand (DaemonCommand::c_stopwater, stage));
}


bool Daemon::isStageEnabled (int stage)
{
    switch (stage) {
    case 1:
        return _s1;
    case 2:
        return _s2;
    case 3:
        return _s3;
    case 4:
        return _s4;
    default:
        return false;
    }
}


void Daemon::getStages ()
{
    sendCommand (QString ("getstages\n"));
    _queue.push_back (DaemonCommand (DaemonCommand::c_getstages));
}


void Daemon::isGrainSensorsPresent ()
{
    sendCommand (QString ("getgrainsensors\n"));
    _queue.push_back (DaemonCommand (DaemonCommand::c_getgrainsensors));
}


void Daemon::setGrainSensorsEnabled (bool val)
{
    int v = val ? 1 : 0;

    sendCommand (QString ("setgrainsensors %1\n").arg (v));
    _queue.push_back (DaemonCommand (DaemonCommand::c_setgrainsensors, v));
}


void Daemon::isGrainPresent (int stage)
{
    sendCommand (QString ("isgrainpresent %1\n").arg (stage));
    _queue.push_back (DaemonCommand (DaemonCommand::c_isgrainpresent, stage));
}
