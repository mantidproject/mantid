#ifndef MANTID_CUSTOMINTERFACES_PROJECTSAVEMOCKOBJECTS_H
#define MANTID_CUSTOMINTERFACES_PROJECTSAVEMOCKOBJECTS_H

#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtCustomInterfaces/IProjectSaveView.h"
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces;

GCC_DIAG_OFF_SUGGEST_OVERRIDE

/**** Views ****/

class MockProjectSaveView : public IProjectSaveView {
public:
};

#endif
