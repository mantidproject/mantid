// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONPARAMMOCK_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONPARAMMOCK_H_

#include "../EnggDiffraction/IEnggDiffractionParam.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

using namespace MantidQt::CustomInterfaces;

class MockEnggDiffractionParam : public IEnggDiffractionParam {
public:
  MOCK_CONST_METHOD1(outFilesUserDir, Poco::Path(const std::string &addToDir));

  MOCK_CONST_METHOD1(userHDFRunFilename, std::string(const std::string runNumber));

  MOCK_CONST_METHOD1(userHDFMultiRunFilename,
                     std::string(const std::vector<RunLabel> &runLabels));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONPARAMMOCK_H_
