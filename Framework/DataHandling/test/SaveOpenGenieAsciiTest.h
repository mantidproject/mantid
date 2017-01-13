#ifndef SAVEOPENGENIEASCIITEST_H_
#define SAVEOPENGENIEASCIITEST_H_

#include "MantidDataHandling/SaveOpenGenieAscii.h"
#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataHandling;

class SaveOpenGenieASCIITest : public CxxTest::TestSuite {
public:


private:
  MatrixWorkspace_sptr workspaceCreator(const int numHist, const int numBins) {
    return WorkspaceCreationHelper::create2DWorkspaceBinned(numHist, numBins,
                                                            1.0);
  }
};

#endif /* SAVEOPENGENIEASCIITEST_H_ */