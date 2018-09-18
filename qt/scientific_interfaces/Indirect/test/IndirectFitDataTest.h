#ifndef MANTID_INDIRECTFITDATATEST_H_
#define MANTID_INDIRECTFITDATATEST_H_

#include <cxxtest/TestSuite.h>

#include "IndirectFitData.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <iostream>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::IDA;

namespace {

IndirectFitData getIndirectFitData(const std::size_t &numberOfSpectra,
                                   const std::size_t &numberOfBins) {
  const auto workspace = WorkspaceCreationHelper::create2DWorkspace123(
      numberOfSpectra, numberOfBins);
  const Spectra spec = std::make_pair(0u, workspace->getNumberHistograms() - 1);
  IndirectFitData data(workspace, spec);
  return data;
}

} // namespace

class IndirectFitDataTest : public CxxTest::TestSuite {
public:
  static IndirectFitDataTest *createSuite() {
    return new IndirectFitDataTest();
  }

  static void destroySuite(IndirectFitDataTest *suite) { delete suite; }

  void test_data_is_instantiated_correctly() {
    const auto workspace = WorkspaceCreationHelper::create2DWorkspace123(1, 3);
    const Spectra spec =
        std::make_pair(0u, workspace->getNumberHistograms() - 1);

    workspace->setTitle("Test Title");
    const IndirectFitData data(workspace, spec);

    TS_ASSERT_EQUALS(data.workspace(), workspace);
    TS_ASSERT_EQUALS(data.workspace()->getTitle(), "Test Title");
    TS_ASSERT_EQUALS(data.workspace()->getNumberHistograms(), 1);
  }

  void test_displayName_returns_correct_name() {
    const auto data = getIndirectFitData(1, 3);

    std::vector<std::string> formatStrings;
    formatStrings.emplace_back("%1%_s%2%_Result");
    formatStrings.emplace_back("%1%_f%2%,s%2%_Parameter");
    formatStrings.emplace_back("%1%_s%2%_Parameter");
    const std::string rangeDelimiter = "_to_";
    const std::size_t spectrum = 1;

    TS_ASSERT_EQUALS(data.displayName(formatStrings[0], rangeDelimiter),
                     "_s0_Result");
    TS_ASSERT_EQUALS(data.displayName(formatStrings[1], rangeDelimiter),
                     "_f0+s0_Parameter");
    TS_ASSERT_EQUALS(data.displayName(formatStrings[2], spectrum),
                     "_s1_Parameter");
  }

  void test_that_correct_spectrum_number_is_returned() {
    const auto data = getIndirectFitData(4, 3);

    for (auto i = 0; i < data.numberOfSpectra(); ++i) {
      const std::size_t spectrumNum = data.getSpectrum(i);
      TS_ASSERT_EQUALS(spectrumNum, i);
    }
  }

  void test_that_correct_number_of_spectra_is_returned() {
    const auto data = getIndirectFitData(10, 3);
    TS_ASSERT_EQUALS(data.numberOfSpectra(), 10);
  }

  void test_that_true_is_returned_if_data_contains_zero_spectra() {
    auto data = getIndirectFitData(1, 3);
    // NOT FINISHED
    TS_ASSERT_EQUALS(data.zeroSpectra(), true);
  }

  void test_that_false_is_returned_if_data_contains_one_or_more_spectra() {
    for (auto i = 1; i < 10; ++i) {
      const auto data = getIndirectFitData(i, 3);
      TS_ASSERT_EQUALS(data.zeroSpectra(), false);
    }
  }

  void test_that_correct_range_for_a_spectrum_is_returned() {
    auto data = getIndirectFitData(1, 10);

    data.setStartX(0.0, 0);
    data.setEndX(5.0, 0);

    TS_ASSERT_EQUALS(data.getRange(0).first, 0.0);
    TS_ASSERT_EQUALS(data.getRange(0).second, 5.0);
    TS_ASSERT_DIFFERS(data.getRange(1).first, 0.0);
    TS_ASSERT_DIFFERS(data.getRange(1).second, 5.0);
  }
};

#endif
