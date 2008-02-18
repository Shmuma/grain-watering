#include <QtCore>
#include "controller.h"


// --------------------------------------------------
// ControllerState
// --------------------------------------------------
ControllerState::ControllerState ()
    : QObject ()
{
}


void ControllerState::setStageEnabled (int stage, bool enabled)
{
    if (stage <= 0 || stage > 4)
        return;

    if (stages[stage-1].enabled () != enabled) {
        stages[stage-1].setEnabled (enabled);
        stageEnabledChanged (stage, enabled);
    }
}
