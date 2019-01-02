// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "../../../ISISReflectometry/GUI/Save/ISaveView.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace MantidQt {
namespace CustomInterfaces {

class MockSaveView : public ISaveView {
public:
  MOCK_METHOD1(subscribe, void(SaveViewSubscriber *));
  MOCK_CONST_METHOD0(getSavePath, std::string());
  MOCK_CONST_METHOD1(setSavePath, void(const std::string &));
  MOCK_CONST_METHOD0(getPrefix, std::string());
  MOCK_CONST_METHOD0(getFilter, std::string());
  MOCK_CONST_METHOD0(getRegexCheck, bool());
  MOCK_CONST_METHOD0(getCurrentWorkspaceName, std::string());
  MOCK_CONST_METHOD0(getSelectedWorkspaces, std::vector<std::string>());
  MOCK_CONST_METHOD0(getSelectedParameters, std::vector<std::string>());
  MOCK_CONST_METHOD0(getFileFormatIndex, int());
  MOCK_CONST_METHOD0(getTitleCheck, bool());
  MOCK_CONST_METHOD0(getQResolutionCheck, bool());
  MOCK_CONST_METHOD0(getSeparator, std::string());

  MOCK_CONST_METHOD0(clearWorkspaceList, void());
  MOCK_CONST_METHOD0(clearParametersList, void());
  MOCK_CONST_METHOD1(setWorkspaceList, void(const std::vector<std::string> &));
  MOCK_CONST_METHOD1(setParametersList, void(const std::vector<std::string> &));
  MOCK_METHOD0(disallowAutosave, void());

  MOCK_METHOD0(disableAutosaveControls, void());
  MOCK_METHOD0(enableAutosaveControls, void());

  MOCK_METHOD0(enableFileFormatAndLocationControls, void());
  MOCK_METHOD0(disableFileFormatAndLocationControls, void());

  MOCK_METHOD0(showFilterEditValid, void());
  MOCK_METHOD0(showFilterEditInvalid, void());
  MOCK_METHOD0(errorInvalidSaveDirectory, void());
  MOCK_METHOD0(warnInvalidSaveDirectory, void());
  MOCK_METHOD0(noWorkspacesSelected, void());
  MOCK_METHOD0(cannotSaveWorkspaces, void());
  MOCK_METHOD1(cannotSaveWorkspaces, void(std::string const &));
};
} // namespace CustomInterfaces
} // namespace MantidQt
GNU_DIAG_ON_SUGGEST_OVERRIDE
