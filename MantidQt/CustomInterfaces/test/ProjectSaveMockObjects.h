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
  MOCK_METHOD0(getWindows, std::vector<MantidQt::API::IProjectSerialisable*>());
  MOCK_METHOD1(updateWorkspacesList, void(const std::set<std::string> &));
  MOCK_METHOD1(updateIncludedWindowsList, void(const std::set<std::string> &));
  MOCK_METHOD1(updateExcludedWindowsList, void(const std::set<std::string> &));
};

#endif
