// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "ALFAlgorithmManager.h"
#include "IALFAlgorithmManagerSubscriber.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/WarningSuppressions.h"

namespace MantidQt {
namespace CustomInterfaces {

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockALFAlgorithmManagerSubscriber : public MantidQt::CustomInterfaces::IALFAlgorithmManagerSubscriber {
public:
  MOCK_METHOD1(notifyAlgorithmError, void(std::string const &message));

  // Algorithm notifiers used when loading and normalising the Sample
  MOCK_METHOD1(notifyLoadComplete, void(Mantid::API::MatrixWorkspace_sptr const &workspace));
  MOCK_METHOD1(notifyNormaliseByCurrentComplete, void(Mantid::API::MatrixWorkspace_sptr const &workspace));
  MOCK_METHOD1(notifyRebinToWorkspaceComplete, void(Mantid::API::MatrixWorkspace_sptr const &workspace));
  MOCK_METHOD1(notifyDivideComplete, void(Mantid::API::MatrixWorkspace_sptr const &workspace));
  MOCK_METHOD1(notifyReplaceSpecialValuesComplete, void(Mantid::API::MatrixWorkspace_sptr const &workspace));
  MOCK_METHOD1(notifyConvertUnitsComplete, void(Mantid::API::MatrixWorkspace_sptr const &workspace));

  // Algorithm notifiers used when producing an Out of plane angle workspace
  MOCK_METHOD1(notifyCreateWorkspaceComplete, void(Mantid::API::MatrixWorkspace_sptr const &workspace));
  MOCK_METHOD1(notifyScaleXComplete, void(Mantid::API::MatrixWorkspace_sptr const &workspace));
  MOCK_METHOD1(notifyRebunchComplete, void(Mantid::API::MatrixWorkspace_sptr const &workspace));

  // Algorithm notifiers used when fitting the extracted Out of plane angle workspace
  MOCK_METHOD1(notifyCropWorkspaceComplete, void(Mantid::API::MatrixWorkspace_sptr const &workspace));
  MOCK_METHOD3(notifyFitComplete, void(Mantid::API::MatrixWorkspace_sptr workspace,
                                       Mantid::API::IFunction_sptr function, std::string fitStatus));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

} // namespace CustomInterfaces
} // namespace MantidQt
