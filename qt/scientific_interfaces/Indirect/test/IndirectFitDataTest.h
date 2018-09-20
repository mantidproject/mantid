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
    // NOT FINISHED - set number of spectra to zero
    TS_ASSERT_EQUALS(data.zeroSpectra(), true);
  }

  void test_that_false_is_returned_if_data_contains_one_or_more_spectra() {
    for (auto i = 1; i < 10; ++i) {
      const auto data = getIndirectFitData(i, 3);
      TS_ASSERT_EQUALS(data.zeroSpectra(), false);
    }
  }

  void
  test_that_correct_excludeRegion_is_returned_when_regions_are_in_correct_order() {
    /// When each pair of numbers in the string are in order, then the whole
    /// string is in the correct order (unordered: 5,6,9,7     ordered:5,6,7,9)
    auto data = getIndirectFitData(4, 3);

    data.setExcludeRegionString("1,8", 0);
    data.setExcludeRegionString("2,5", 1);
    data.setExcludeRegionString("1,2,5,6,3,4", 2);

    std::vector<double> regionVector1;
    regionVector1.emplace_back(1.0);
    regionVector1.emplace_back(8.0);
    std::vector<double> regionVector2;
    regionVector2.emplace_back(2.0);
    regionVector2.emplace_back(5.0);

    TS_ASSERT_EQUALS(data.getExcludeRegion(0), "1,8");
    TS_ASSERT_EQUALS(data.getExcludeRegion(1), "2,5");
    TS_ASSERT_EQUALS(data.getExcludeRegion(2), "1,2,5,6,3,4");
    TS_ASSERT_EQUALS(data.getExcludeRegion(3), "");
    TS_ASSERT_EQUALS(data.excludeRegionsVector(0), regionVector1);
    TS_ASSERT_EQUALS(data.excludeRegionsVector(1), regionVector2);
    TS_ASSERT_EQUALS(data.excludeRegionsVector(3).empty(), true);
  }

  void test_that_excludeRegion_is_stored_in_correct_order() {
    auto data = getIndirectFitData(3, 3);

    data.setExcludeRegionString("6,2", 0);
    data.setExcludeRegionString("6,2,1,2,3,4,7,6", 1);
    data.setExcludeRegionString("1,2,2,3,20,18,21,22,7,8", 2);

    std::vector<double> regionVector;
    regionVector.emplace_back(2.0);
    regionVector.emplace_back(6.0);

    TS_ASSERT_EQUALS(data.getExcludeRegion(0), "2,6");
    TS_ASSERT_EQUALS(data.getExcludeRegion(1), "2,6,1,2,3,4,6,7");
    TS_ASSERT_EQUALS(data.getExcludeRegion(2), "1,2,2,3,18,20,21,22,7,8");
    TS_ASSERT_EQUALS(data.excludeRegionsVector(0), regionVector);
  }

  void
  test_that_excludeRegion_is_stored_correctly_when_there_are_many_spaces_in_input_string() {
    auto data = getIndirectFitData(3, 3);

    data.setExcludeRegionString("  6,     2", 0);
    data.setExcludeRegionString("6,  2,1  ,2,  3,4  ,7,6", 1);
    data.setExcludeRegionString("1,2 ,2,3,  20,  18,21,22,7, 8   ", 2);

    TS_ASSERT_EQUALS(data.getExcludeRegion(0), "2,6");
    TS_ASSERT_EQUALS(data.getExcludeRegion(1), "2,6,1,2,3,4,6,7");
    TS_ASSERT_EQUALS(data.getExcludeRegion(2), "1,2,2,3,18,20,21,22,7,8");
  }

  void test_throws_when_setSpectra_is_provided_an_out_of_range_spectra() {
    auto data = getIndirectFitData(10, 3);

    std::vector<Spectra> spectraPairs;
    spectraPairs.emplace_back(std::make_pair(0u, 11u));
    spectraPairs.emplace_back(std::make_pair(0u, 1000000000000000000u));
    spectraPairs.emplace_back(std::make_pair(10u, 10u));
    std::vector<std::string> spectraStrings;
    spectraStrings.emplace_back("-1");
    spectraStrings.emplace_back("10");
    spectraStrings.emplace_back("100000000000000000000000000000");
    spectraStrings.emplace_back("1,5,10,20,30,60,80,100");
    spectraStrings.emplace_back("1,2,3,4,5,6,22");

    for (auto i = 0u; i < spectraPairs.size(); ++i)
      TS_ASSERT_THROWS(data.setSpectra(spectraPairs[i]), std::runtime_error);
    for (auto i = 0u; i < spectraStrings.size(); ++i)
      TS_ASSERT_THROWS(data.setSpectra(spectraStrings[i]), std::runtime_error);
  }

  void test_no_throw_when_setSpectra_is_provided_a_valid_spectra() {
    auto data = getIndirectFitData(10, 3);

    std::vector<Spectra> spectraPairs;
    spectraPairs.emplace_back(std::make_pair(0u, 9u));
    spectraPairs.emplace_back(std::make_pair(4u, 4u));
    spectraPairs.emplace_back(std::make_pair(7u, 4u));
    std::vector<std::string> spectraStrings;
    spectraStrings.emplace_back("0");
    spectraStrings.emplace_back("9");
    spectraStrings.emplace_back("0,9,6,4,5");
    spectraStrings.emplace_back("1,2,3,4,5,6");

    for (auto i = 0u; i < spectraPairs.size(); ++i)
      TS_ASSERT_THROWS_NOTHING(data.setSpectra(spectraPairs[i]));
    for (auto i = 0u; i < spectraStrings.size(); ++i)
      TS_ASSERT_THROWS_NOTHING(data.setSpectra(spectraStrings[i]));
  }

  void test_throws_when_you_try_setStartX_with_no_workspace() {
    auto data = getIndirectFitData(1, 3);
    // NOT FINISHED - delete workspace
    TS_ASSERT_THROWS(data.setStartX(0.0, 50), std::runtime_error);
  }

  void
  test_no_throw_when_setStartX_is_provided_a_valid_xValue_and_spectrum_number() {
    /// A spectrum number above the workspace numberOfSpectra wouldn't be
    /// allowed when called from the interface - hence a check isn't required
    auto data = getIndirectFitData(10, 3);

    TS_ASSERT_THROWS_NOTHING(data.setStartX(0.0, 0));
    TS_ASSERT_THROWS_NOTHING(data.setStartX(-5.0, 0));
    TS_ASSERT_THROWS_NOTHING(data.setStartX(5000000, 10));
  }
};

#endif
