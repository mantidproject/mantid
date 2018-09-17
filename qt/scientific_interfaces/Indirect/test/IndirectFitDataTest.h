#ifndef MANTID_INDIRECTFITDATATEST_H_
#define MANTID_INDIRECTFITDATATEST_H_

#include <cxxtest/TestSuite.h>

#include "IndirectFitData.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <iostream>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::IDA;

class IndirectFitDataTest : public CxxTest::TestSuite {
public:
  static IndirectFitDataTest *createSuite() {
    return new IndirectFitDataTest();
  }

  static void destroySuite(IndirectFitDataTest *suite) { delete suite; }

  void test_data_is_instantiated_correctly() {
    auto workspace = WorkspaceCreationHelper::create2DWorkspace123(1, 3);
    const Spectra spec =
        std::make_pair(0u, workspace->getNumberHistograms() - 1);

    workspace->setTitle("Test Title");
    IndirectFitData data(workspace, spec);

    TS_ASSERT_EQUALS(data.workspace()->getTitle(), "Test Title");
    TS_ASSERT_EQUALS(data.workspace()->getNumberHistograms(), 1);
  }

  void test_displayName_returns_correct_name() {
    auto workspace = WorkspaceCreationHelper::create2DWorkspace123(1, 3);
    const Spectra spec =
        std::make_pair(0u, workspace->getNumberHistograms() - 1);
    IndirectFitData data(workspace, spec);

    const std::string formatString = "%1%_s%2%_Result";
    const std::string rangeDelimiter = "_to_";

    TS_ASSERT_EQUALS(data.displayName(formatString, rangeDelimiter), "_s0_Result");
  }
};

#endif
