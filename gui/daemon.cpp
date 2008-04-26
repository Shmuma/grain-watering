#include <QtCore>
#include <QtNetwork>
#include "daemon.h"
#include "logger.h"
#include "settings.h"


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
    Logger::instance ()->log (Logger::Information, QString ("Connecting to daemon at %1:%2").
                              arg (_host, QString::number (_port)));
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
    QString auto_prefix ("Auto: ");
    QString check_prefix ("Check: ");
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

        if (!reply.startsWith (auto_prefix) && !reply.startsWith (check_prefix)) {
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
                case DaemonCommand::c_startautomode:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot start auto mode on stage %1. Reason: '%2'").
                                                  arg (QString::number (cmd.stage ())+1, msg));
                    else
                        autoModeStarted (cmd.stage ());
                    break;
                case DaemonCommand::c_stopautomode:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot stop auto mode on stage %1. Reason: '%2'").
                                                  arg (QString::number (cmd.stage ()+1), msg));
                    else
                        autoModeStopped (cmd.stage ());
                    break;
                case DaemonCommand::c_toggleautomode:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot toggle auto mode on stage %1. Reason: '%2'").
                                                  arg (QString::number (cmd.stage ()+1), msg));
                    else
                        autoModeToggled (cmd.stage (), !msg.contains ("unpaused"));
                    break;
                case DaemonCommand::c_getautomode:
                    autoModeGot (cmd.stage (), reply.split (",")[0] == "active", reply.split (",")[1].startsWith ("paused"));
                    break;
                case DaemonCommand::c_getmetastate:
                    handleMetaState (reply);
                    break;
                case DaemonCommand::c_startwater:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot start water at stage %1. Reason: '%2'").
                                                  arg (QString::number (cmd.stage ()+1), msg));
                    else
                        waterStarted (cmd.stage ());
                    break;
                case DaemonCommand::c_stopwater:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot stop water at stage %1. Reason: '%2'").
                                                  arg (QString::number (cmd.stage ()+1), msg));
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
                case DaemonCommand::c_getsettings:
                    parseSettings (reply);
                    break;
                case DaemonCommand::c_setsettings:
                    break;
                case DaemonCommand::c_gettempcoef:
                    parseTempCoef (reply);
                    break;
                case DaemonCommand::c_calibrate:
                    parseCalibrateReply (cmd.stage (), reply);
                    break;
                }
            }
        }
        else {
            if (reply.startsWith (auto_prefix)) {
                bool state;
                double pres;

                autoTextArrived (QString (reply).remove (auto_prefix).trimmed ());
                if (parseAutoModeTick (reply, &state, &pres))
                    autoModeTickGot (state, pres);
            }
            else {
                handleCheckTick (reply);
                printf ("Check: %s", reply.toAscii ().constData ());
            }
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
    QStringList l = QString (reply).remove (" ").split (",", QString::SkipEmptyParts);

    if (l.size () < 2)
        return false;

    printf ("Auto: %d, %s\n", l.size (), reply.toAscii ().constData ());

    *state = l[0].split (":")[1] == "OK";
    *press = l[1].split (":")[1].toDouble ();

    return true;
}


void Daemon::parseTempCoef (const QString& reply)
{
    double k, resist;
    QStringList l = reply.split (" ", QString::SkipEmptyParts);
    
    if (l.size () != 2)
        return;

    k = l[0].toDouble ();
    resist = l[1].toDouble ();

    tempCoefGot (k, resist);
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


void Daemon::startAutoMode (int stage)
{
    sendCommand (QString ("startautomode %1\n").arg (stage+1));
    _queue.push_back (DaemonCommand (DaemonCommand::c_startautomode, stage));
}


void Daemon::stopAutoMode (int stage)
{
    sendCommand (QString ("stopautomode %1\n").arg (stage+1));
    _queue.push_back (DaemonCommand (DaemonCommand::c_stopautomode, stage));
}


void Daemon::toggleAutoMode (int stage)
{
    sendCommand (QString ("toggleautomode %1\n").arg (stage+1));
    _queue.push_back (DaemonCommand (DaemonCommand::c_toggleautomode, stage));
}


void Daemon::getAutoMode (int stage)
{
    sendCommand (QString ("getautomode %1\n").arg (stage+1));
    _queue.push_back (DaemonCommand (DaemonCommand::c_getautomode, stage));
}


void Daemon::refreshState ()
{
    sendCommand (QString ().sprintf ("getmetastate %d %d %d %d\n", _s1 ? 1 : 0, _s2 ? 1 : 0, _s3 ? 1 : 0, _s4 ? 1 : 0));
    _queue.push_back (DaemonCommand (DaemonCommand::c_getmetastate));
}


bool Daemon::handleMetaState (const QString& msg)
{
    QStringList l = QString (msg).trimmed ().split (" ");
    double water_pres = 0;
    QString s;
    QStringList ll;
    QMap<int, QList<double> > vals;

    printf ("%s\n", msg.toAscii ().constData ());

    QStringList::iterator it = l.begin ();

    while (it != l.end ()) {
        s = *it;
        ll = s.trimmed ().split (":", QString::SkipEmptyParts);

        if (ll.size () == 2) {
            int index = ll[0].toInt ();
            s = ll[1];
            ll = s.trimmed ().split (",", QString::SkipEmptyParts);
        
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
    sendCommand (QString ().sprintf ("startwater %d\n", stage+1));
    _queue.push_back (DaemonCommand (DaemonCommand::c_startwater, stage));
}


void Daemon::stopWater (int stage)
{
    sendCommand (QString ().sprintf ("stopwater %d\n", stage+1));
    _queue.push_back (DaemonCommand (DaemonCommand::c_stopwater, stage));
}


bool Daemon::isStageEnabled (int stage)
{
    switch (stage) {
    case 0:
        return _s1;
    case 1:
        return _s2;
    case 2:
        return _s3;
    case 3:
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
    sendCommand (QString ("isgrainpresent %1\n").arg (stage+1));
    _queue.push_back (DaemonCommand (DaemonCommand::c_isgrainpresent, stage));
}


void Daemon::handleCheckTick (const QString& msg)
{
    // parse values
    QStringList stages = msg.split (" ");
    double val;
    bool ok;

    if (stages.count () < 2)
        return;
    
    if (!stages[1].startsWith ("WP="))
        return;

    _lastCheck = QDateTime::currentDateTime ();

    val = stages[1].remove ("WP=").toDouble (&ok);

    if (!ok)
        return;

    waterPressureUpdated (val);
    
    for (int i = 2; i < stages.count (); i++) {
        QString val_str;
        QStringList lst = stages[i].split (":"), l;
        int stage;
        QString key, val;
        
        if (lst.count () != 2) {
            Logger::instance ()->log (Logger::Debug, QString ("CheckTick: Got invalid stage info at part %1").arg (i));
            printf ("Data: %s\n", stages[i].toAscii ().constData ());
            continue;
        }

        val_str = lst[1];

        stage = lst[0].toInt (&ok)-1;
        if (!ok)
            continue;

        lst = val_str.split (",");
        
        for (int j = 0; j < lst.count (); j++) {
            l = lst[j].split ('=');
            if (l.count () != 2)
                continue;
            key = l[0];
            val = l[1];
            
            if (key == "G")
                grainPresentUpdated (stage, val == "1");
            else if (key == "WF")
                waterFlowUpdated (stage, val.toDouble ());
            else if (key == "GF")
                grainFlowUpdated (stage, val.toDouble ());
            else if (key == "GH")
                grainHumidityUpdated (stage, val.toDouble ());
            else if (key == "GN")
                grainNatureUpdated (stage, val.toDouble ());
            else if (key == "GT")
                grainTemperatureUpdated (stage, val.toDouble ());
            else if (key == "CH")
                calculatedHumidityUpdated (stage, val.toDouble ());
            else if (key == "TF")
                targetFlowUpdated (stage, val.toDouble ());
            else if (key == "TS")
                targetSettingUpdated (stage, val.toDouble ());
        }
    }
}


void Daemon::requestSettings ()
{
    sendCommand (QString ("getsettings\n"));
    _queue.push_back (DaemonCommand (DaemonCommand::c_getsettings));
}


void Daemon::parseSettings (const QString& msg)
{
    QStringList l = msg.trimmed ().split (" ", QString::SkipEmptyParts);
    int i;

    for (i = 0; i < 4; i++)
        _sett[i] = StageSettings (l[i]);

    // parse passwords
    _pass.clear ();
    QStringList u = l[4].split (",", QString::SkipEmptyParts);
    
    for (i = 0; i < u.size (); i++) {
        QStringList v = u[i].split ("=", QString::SkipEmptyParts);
        _pass[v[0]] = v[1];
    }

    settingsGot ();
}


void Daemon::setSettings (int stage, const StageSettings& sett)
{
    if (sett.valid ()) {
        _sett[stage] = sett;
        sendCommand (QString ("setsettings %1 %2\n").arg (QString::number (stage+1), sett.toString ()));
    }
}


bool Daemon::checkPass (const QString& user, const QString& pass)
{
    if (_pass.find (user) == _pass.end ())
        return false;
    return _pass[user] == pass;
}


void Daemon::setSensors (bool s1, bool s2, bool s3, bool s4)
{
    sendCommand (QString ().sprintf ("setsensors %d %d %d %d\n", s1 ? 1 : 0, s2 ? 1 : 0, s3 ? 1 : 0, s4 ? 1 : 0));
}


void Daemon::requestTempCoef ()
{
    sendCommand ("gettempcoef\n");
    _queue.push_back (DaemonCommand (DaemonCommand::c_gettempcoef));
}


void Daemon::setTempCoef (double k, double resist)
{
    sendCommand (QString ().sprintf ("settempcoef %f %f\n", k, resist));
}


void Daemon::calibrate (int stage, const QString& key)
{
    sendCommand (QString ("calibrate %1 %2\n").arg (QString::number (stage+1), key));
    _queue.push_back (DaemonCommand (DaemonCommand::c_calibrate, stage));
}


void Daemon::parseCalibrateReply (int stage, const QString& reply)
{
    if (reply.startsWith ("ERROR"))
        Logger::instance ()->log (Logger::Error, tr ("Calibration failed. Reason: %1").arg (QString (reply).remove ("ERROR: ").trimmed ()));
    else {
        QStringList l = reply.trimmed ().split ("=");
        
        if (l.size () != 2)
            Logger::instance ()->log (Logger::Error, tr ("Calibration failed. Unexpected reply: %1").arg (reply.trimmed ()));
        else
            calibrateReply (stage, l[0], l[1].toDouble ());
    }
}
