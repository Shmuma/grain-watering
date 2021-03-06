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
    Logger::instance ()->log (Logger::Information, tr ("Connecting to daemon at %1:%2").
                              arg (_host, QString::number (_port)));
    _sock->connectToHost (_host, _port);
}


void Daemon::disconnect ()
{
    Logger::instance ()->log (Logger::Debug, tr ("Disconnect from daemon"));
    _sock->disconnectFromHost ();
}


void Daemon::socketStateChanged (QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        connectedChanged (false);
        _connected = _hw_connected = false;
        Logger::instance ()->log (Logger::Debug, tr ("Disconnected"));
        break;

    case QAbstractSocket::ConnectedState:
        connectedChanged (true);
        _connected = true;
        Logger::instance ()->log (Logger::Debug, tr ("Connected"));

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
    //    QString auto_prefix ("Auto: ");
    QString cleanStarted_prefix ("Cleaning started");
    QString cleanFinished_prefix ("Cleaning finished");
    QString drainFinished_prefix ("Water drained");
    QString check_prefix ("Check: ");
    QString hist_prefix ("History: ");
    QString prompt_prefix ("> ");
    QStringList replies;
    bool s1, s2, s3, s4;
    int rev;

    reply = _data_buf + QString::fromUtf8 (_sock->readAll ());

    if (!reply.contains (prompt_prefix)) {
        _data_buf = reply;
        return;
    }

    rev = reply.lastIndexOf (prompt_prefix);

    if (rev+prompt_prefix.size () < reply.size ()) {
        _data_buf = reply.right (reply.size () - rev - prompt_prefix.size ());
        reply.truncate (rev);
    }
    else
        _data_buf = QString ();

    // split read data using prompt as separator
    replies = reply.split (prompt_prefix, QString::SkipEmptyParts);

    for (int i = 0; i < replies.size (); i++) {
        reply = replies[i].trimmed ();

        if (reply.isEmpty ())
            continue;

        if (reply.startsWith (cleanStarted_prefix)) {
            cleanStarted ();
            continue;
        }

        if (reply.startsWith (cleanFinished_prefix)) {
            cleanFinished ();
            continue;
        }

        if (reply.startsWith (drainFinished_prefix)) {
            drainFinished ();
            continue;
        }

        if (!reply.startsWith (check_prefix)) {
            if (!reply.startsWith (hist_prefix))
                textArrived (reply+"\n"+prompt_prefix);

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
                        Logger::instance ()->log (Logger::Error, tr ("Connection to hardware failed. Reason: '%1'").arg (trDaemon (msg)));
                    }
                    else {
                        _hw_connected = true;
                        Logger::instance ()->log (Logger::Information, tr ("Controller connected to hardware"));
                        hardwareConnected ();
                    }
                    break;
                case DaemonCommand::c_setstages:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot set active stages. Reason: '%1'").arg (trDaemon (msg)));
                    else
                        stagesActivityChanged (_s1, _s2, _s3, _s4);
                    break;
                case DaemonCommand::c_startstage:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot start stage %1. Reason: '%2'").arg (QString::number (cmd.stage ()+1), trDaemon (msg)));
                    else
                        stageStarted (cmd.stage ());
                    break;
                case DaemonCommand::c_stopstage:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot stop stage %1. Reason: '%2'").arg (QString::number (cmd.stage ()+1), trDaemon (msg)));
                    else
                        stageStopped (cmd.stage ());
                    break;
                case DaemonCommand::c_getmetastate:
                    handleMetaState (reply);
                    break;
                case DaemonCommand::c_startwater:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot start water at stage %1. Reason: '%2'").
                                                  arg (QString::number (cmd.stage ()+1), trDaemon (msg)));
                    else
                        waterStarted (cmd.stage ());
                    break;
                case DaemonCommand::c_stopwater:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot stop water at stage %1. Reason: '%2'").
                                                  arg (QString::number (cmd.stage ()+1), trDaemon (msg)));
                    else
                        waterStopped (cmd.stage ());
                    break;
                case DaemonCommand::c_getstages:
                    if (!parseStagesReply (reply, msg, s1, s2, s3, s4))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot get active stages. Reason: '%1'").arg (trDaemon (msg)));
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
                        Logger::instance ()->log (Logger::Error, tr ("Cannot change grain sensors presence. Reason: '%1'").arg (trDaemon (msg)));
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
                case DaemonCommand::c_gethistory:
                    parseHistory (reply.trimmed (), cmd.historyStage (), cmd.historyKind ());
                    break;
                case DaemonCommand::c_getevents:
                    parseEvents (reply.trimmed ());
                    break;
                case DaemonCommand::c_clean:
                    if (!parseGenericReply (reply, msg))
                        Logger::instance ()->log (Logger::Error, tr ("Clean request finished with error. Reason: '%1'").arg (trDaemon (msg)));
                    else
                        cleanRequested ();
                    break;
                case DaemonCommand::c_drain:
                    if (!parseGenericReply (reply.trimmed (), msg))
                        Logger::instance ()->log (Logger::Error, tr ("Drain finished with error. Reason: '%1'").arg (trDaemon (msg)));
                    else
                        drainStarted ();
                    break;
                case DaemonCommand::c_getcleanstate:
                    parseCleanState (reply.trimmed ());
                    break;
                case DaemonCommand::c_startfilterautomat:
                    if (!parseGenericReply (reply.trimmed (), msg))
                        Logger::instance ()->log (Logger::Error, tr ("Filter clean error. Reason: '%1'").arg (trDaemon (msg)));
                    break;
                case DaemonCommand::c_setsensors:
                    break;
                case DaemonCommand::c_settempcoef:
                    break;
                case DaemonCommand::c_setstagemodes:
                    break;
                case DaemonCommand::c_getcleanresult:
                    parseCleanResultReply (reply.trimmed ());
                    break;
                case DaemonCommand::c_settargetwaterflow:
                    if (!parseGenericReply (reply.trimmed (), msg))
                        Logger::instance ()->log (Logger::Error, tr ("Cannot assign target water flow on stage %1. Reason: '%2'")
                                                  .arg (cmd.stage ()+1).arg (trDaemon (msg)));                    
                    else
                        targetFlowUpdated (cmd.stage (), cmd.val ());
                    break;
                case DaemonCommand::c_setminpressure:
                    break;
                case DaemonCommand::c_getminpressure:
                    parseMinPressure (reply);
                    break;
                default:
                    break;
                }
            }
        }
        else 
            handleCheckTick (reply);
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


void Daemon::parseCleanResultReply (const QString& reply)
{
    QString msg = QString (reply).remove ("ERROR:").remove ('>').trimmed ();

    if (reply.startsWith ("ERROR:")) {
        Logger::instance ()->log (Logger::Error, tr ("Cannot get clean result. Reason: '%1'").arg (trDaemon (msg)));
        return;
    }
        
    bool ok;
    unsigned int val;
    QString str = QString (reply).remove (">").trimmed ();
    val = str.toUInt (&ok);

    if (!ok)
        msg = tr ("Expected number, got '%1'").arg (reply);

    bool s_w[4], s_r[4];
    
    for (int i = 0; i < 4; i++) {
        s_w[i] = val & (1 << (i*2));
        s_r[i] = val & (2 << (i*2));
    }

    gotCleanResult (s_w, s_r);

    return;
}


bool Daemon::parseStagesReply (const QString& reply, QString& msg, bool& s1, bool& s2, bool& s3, bool& s4)
{
    if (reply.startsWith ("ERROR:")) {
        msg = QString (reply).remove ("ERROR:").remove ('>').trimmed ();
        return false;
    }

    s1 = s2 = s3 = s4 = false;

    if (QString (reply).remove ('>').trimmed () == "empty")
        return true;

    QStringList lst = QString (reply).remove ('>').trimmed ().split (',', QString::SkipEmptyParts);
    bool ok;
    int val;

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


void Daemon::parseCleanState (const QString& reply)
{
    QStringList l = reply.split (" ", QString::SkipEmptyParts);

    if (l.size () != 5) {
        Logger::instance ()->log (Logger::Error, tr ("Cannot get clean state. Reason: '%1'").arg (reply));
        return;
    }

    gotCleanState (l[0] == "1", l[1] == "1", l[2] == "1", l[3] == "1", l[4] == "1");
}


void Daemon::sendCommand (const QString& cmd, bool log)
{
    if (log)
        commandSent (cmd.trimmed () + "\n");
    _sock->write (cmd.toUtf8 ());
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


void Daemon::refreshState ()
{
    sendCommand (QString ().sprintf ("getmetastate %d %d %d %d\n", _s1 ? 1 : 0, _s2 ? 1 : 0, _s3 ? 1 : 0, _s4 ? 1 : 0));
    _queue.push_back (DaemonCommand (DaemonCommand::c_getmetastate));
}


bool Daemon::handleMetaState (const QString& msg)
{
    QStringList l = QString (msg).trimmed ().split (" ", QString::SkipEmptyParts);
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
            ll = s.trimmed ().split (",", QString::SkipEmptyParts);
        
            QStringList::iterator it2 = ll.begin ();
        
            while (it2 != ll.end ()) {
                if (it2->contains ("=")) {
                    QStringList lll = (*it2).split ("=", QString::SkipEmptyParts);

                    if (lll.size () == 2) {
                        double val = lll[1].toDouble ();
            
                        if (index == 0)
                            water_pres = val;
                        else
                            vals[index].push_back (val);
                    }
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
    QStringList results = msg.split (": ", QString::SkipEmptyParts);

    autoTextArrived (msg);

    // it's invalid check value
    if (results.size () < 3)
        return;

    QString res = results[1];

    // critical event - show large banner
    if (res.startsWith ("CRIT")) {
        autoModeError ((error_kind_t)res.remove ("CRIT").toInt (), QString ());
    }

    QStringList stages = results[2].split (" ", QString::SkipEmptyParts);
    double val;
    bool ok;

    if (stages.count () < 2)
        return;
    
    if (!stages[0].startsWith ("WP="))
        return;

    _lastCheck = QDateTime::currentDateTime ();

    val = stages[0].remove ("WP=").toDouble (&ok);

    if (!ok)
        return;

    waterPressureUpdated (val);

    for (int i = 1; i < stages.count (); i++) {
        QString val_str;
        QStringList lst = stages[i].split (":", QString::SkipEmptyParts), l;
        int stage;
        QString key, val;
        bool running = false;
            
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
            l = lst[j].split ('=', QString::SkipEmptyParts);
            if (l.count () != 2)
                continue;
            key = l[0];
            val = l[1];
            
            if (key == "R")
                running = val == "1";
            if (key == "G")
                grainPresentUpdated (stage, val == "1");
            else if (key == "BSU")
                bsuPoweredUpdated (stage, val == "1");
            else if (key == "GL")
                grainLowUpdated (stage, val == "1");
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

        // we must update running state the last, because we can lost some critical errors to display.
        stageRunningUpdated (stage, running);
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

    if (l.size () != 5)
        return;

    for (i = 0; i < 4; i++)
        _sett[i] = StageSettings (l[i]);

    // parse passwords
    _pass.clear ();
    QStringList u = l[4].split (",", QString::SkipEmptyParts);
    
    for (i = 0; i < u.size (); i++) {
        QStringList v = u[i].split ("=", QString::SkipEmptyParts);
        if (v.size () != 2)
            continue;
        _pass[v[0]] = v[1];
    }

    settingsGot ();
}


void Daemon::setSettings (int stage, const StageSettings& sett)
{
    if (sett.valid ()) {
        _sett[stage] = sett;
        sendCommand (QString ("setsettings %1 %2\n").arg (QString::number (stage+1), sett.toString ()));
        _queue.push_back (DaemonCommand (DaemonCommand::c_setsettings, stage));
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
    _queue.push_back (DaemonCommand (DaemonCommand::c_setsensors));
}


void Daemon::requestTempCoef ()
{
    sendCommand ("gettempcoef\n");
    _queue.push_back (DaemonCommand (DaemonCommand::c_gettempcoef));
}


void Daemon::setTempCoef (double k, double resist)
{
    sendCommand (QString ().sprintf ("settempcoef %f %f\n", k, resist));
    _queue.push_back (DaemonCommand (DaemonCommand::c_settempcoef));
}


void Daemon::calibrate (int stage, const QString& key)
{
    sendCommand (QString ("calibrate %1 %2\n").arg (QString::number (stage+1), key));
    _queue.push_back (DaemonCommand (DaemonCommand::c_calibrate, stage));
}


void Daemon::parseCalibrateReply (int stage, const QString& reply)
{
    if (reply.startsWith ("ERROR"))
        Logger::instance ()->log (Logger::Error, tr ("Calibration failed. Reason: %1").arg (trDaemon (QString (reply).remove ("ERROR: ").trimmed ())));
    else {
        QStringList l = reply.trimmed ().split ("=", QString::SkipEmptyParts);
        
        if (l.size () != 2)
            Logger::instance ()->log (Logger::Error, tr ("Calibration failed. Unexpected reply: %1").arg (reply.trimmed ()));
        else
            calibrateReply (stage, l[0], l[1].toDouble ());
    }
}


static const char* mode2label (StageSettings::mode_t m) 
{
    switch (m) {
    case StageSettings::M_Auto:
        return "auto";
    case StageSettings::M_SemiAuto:
        return "semi";
    case StageSettings::M_Fixed:
        return "fixed";
    default:
        return "unknown";
    }
}


void Daemon::setStageModes (StageSettings::mode_t s1, StageSettings::mode_t s2, StageSettings::mode_t s3, StageSettings::mode_t s4)
{
    _sett[0].setMode (s1);
    _sett[1].setMode (s2);
    _sett[2].setMode (s3);
    _sett[3].setMode (s4);
    sendCommand (QString ().sprintf ("setstagemodes %s %s %s %s\n", 
                                     mode2label (s1), mode2label (s2),
                                     mode2label (s3), mode2label (s4)));
    _queue.push_back (DaemonCommand (DaemonCommand::c_setstagemodes));
}


void Daemon::requestHistory (history_stage_t stage, history_kind_t kind, uint from, uint to)
{
    sendCommand (QString ().sprintf ("gethistory %d %d %u %u\n", (int)stage, (int)kind, from, to), false);
    _queue.push_back (DaemonCommand (DaemonCommand::c_gethistory, stage, kind));
}


void Daemon::requestEvents (uint from, uint to, bool clean)
{
    sendCommand (QString ().sprintf ("getevents %d %u %u\n", clean ? 1 : 0, from, to), false);
    _queue.push_back (DaemonCommand (DaemonCommand::c_getevents));
}


void Daemon::parseHistory (const QString& reply, history_stage_t stage, history_kind_t kind)
{
    //    printf ("History catched: %s\n", reply.toAscii ().constData ());

    QStringList l = QString (reply).remove ("History:").trimmed ().split (",", QString::SkipEmptyParts);
    QList< QPair <uint, double> > res;

    for (int i = 0; i < l.size (); i += 2) {
        if (i+1 == l.size ())
            break;
        res.push_back (QPair<uint, double> (l[i].toUInt (), l[i+1].toDouble ()));
    }

    historyGot (res, stage, kind);
}


void Daemon::cleanFilter ()
{
    sendCommand ("startfilterautomat\n");
    _queue.push_back (DaemonCommand (DaemonCommand::c_startfilterautomat));
}


void Daemon::cleanStages (bool s1, bool s2, bool s3, bool s4)
{
    sendCommand (QString ().sprintf ("clean %d %d %d %d\n", s1 ? 1 : 0, s2 ? 1 : 0, s3 ? 1 : 0, s4 ? 1 : 0));
    _queue.push_back (DaemonCommand (DaemonCommand::c_clean));
}


void Daemon::drainWater (bool s1, bool s2, bool s3, bool s4)
{
    sendCommand (QString ().sprintf ("drainwater %d %d %d %d\n", s1 ? 1 : 0, s2 ? 1 : 0, s3 ? 1 : 0, s4 ? 1 : 0));
    _queue.push_back (DaemonCommand (DaemonCommand::c_drain));
}


void Daemon::checkTick ()
{
    sendCommand ("checktick\n");
}


void Daemon::startStage (int stage)
{
    sendCommand (QString ().sprintf ("startstage %d\n", stage+1));
    _queue.push_back (DaemonCommand (DaemonCommand::c_startstage, stage));
}


void Daemon::stopStage (int stage)
{
    sendCommand (QString ().sprintf ("stopstage %d\n", stage+1));
    _queue.push_back (DaemonCommand (DaemonCommand::c_stopstage, stage));
}


void Daemon::logMessage (const QString& msg)
{
    sendCommand (QString ("log ") + msg + "\n");
}


void Daemon::logCleanResult (const QString& msg)
{
    sendCommand (QString ("log_clean ") + msg + "\n");
}


void Daemon::parseEvents (const QString& reply)
{
    QStringList l = QString (reply).remove ("Events:").trimmed ().split (",", QString::SkipEmptyParts);
    QList< QPair <uint, QString> > res;

    if (l.size () < 2)
        return;

    for (int i = 0; i < l.size (); i += 2) {
        if (i+1 == l.size ())
            break;
        res.push_back (QPair<uint, QString> (l[i].toUInt (), l[i+1]));
    }

    eventsGot (res);
}


void Daemon::getCleanState ()
{
    sendCommand (QString ("getcleanstate\n"));
    _queue.push_back (DaemonCommand (DaemonCommand::c_getcleanstate));
}


void Daemon::getCleanResult ()
{
    sendCommand (QString ("getcleanresult\n"));
    _queue.push_back (DaemonCommand (DaemonCommand::c_getcleanresult));
}


void Daemon::setTargetWaterFlow (int stage, double val)
{
    sendCommand (QString ("settgtflow %1 %2\n").arg (stage).arg (val));
    _queue.push_back (DaemonCommand (DaemonCommand::c_settargetwaterflow, stage, val));
}


void Daemon::setMinPressure (double val)
{
    sendCommand (QString ("setminpressure %1\n").arg (val));
    _queue.push_back (DaemonCommand::c_setminpressure);
}


void Daemon::getMinPressure ()
{
    sendCommand (QString ("getminpressure\n"));
    _queue.push_back (DaemonCommand::c_getminpressure);   
}


void Daemon::parseMinPressure (const QString& reply)
{
    double val;
    bool ok;

    val = reply.toDouble (&ok);

    if (!ok)
        Logger::instance ()->log (Logger::Error, tr ("Cannot get min pressure. Reason: %1").arg (trDaemon (QString (reply).remove ("ERROR: ").trimmed ())));
    else
        minPressureGot (val);
}


QString Daemon::trDaemon (const QString& msg)
{
    return QCoreApplication::translate ("Interpreter", msg.toAscii ().constData ());
}
