#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONPARAMMOCK_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONPARAMMOCK_H_

#include "../EnggDiffraction/IEnggDiffractionParam.h"
#include "MantidKernel/WarningSuppressions.h"

#include <gmock/gmock.h>

GCC_DIAG_OFF_SUGGEST_OVERRIDE

using namespace MantidQt::CustomInterfaces;

class MockEnggDiffractionParam : public IEnggDiffractionParam {
public:
  MOCK_CONST_METHOD1(outFilesUserDir, Poco::Path(const std::string &addToDir));

  MOCK_CONST_METHOD1(userHDFRunFilename, std::string(const int runNumber));
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_IENGGDIFFRACTIONPARAMMOCK_H_
