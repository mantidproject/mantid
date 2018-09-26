#ifndef MANTID_INDIRECTFITDATATEST_H_
#define MANTID_INDIRECTFITDATATEST_H_

#include <cxxtest/TestSuite.h>

#include "IndirectFitData.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <iostream>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
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

/// Simple class to set up the ADS with the configuration required
class SetUpADSWithWorkspace {
public:
  SetUpADSWithWorkspace(std::string const &inputWSName,
                        IndirectFitData const &data) {
    AnalysisDataService::Instance().addOrReplace(inputWSName, data.workspace());
  };

  ~SetUpADSWithWorkspace() { AnalysisDataService::Instance().clear(); };
};

/// This is used to compare Spectra which is implemented as a boost::variant
class AreSpectraEqual : public boost::static_visitor<bool> {
public:
  template <typename T, typename U>
  bool operator()(const T &, const U &) const {
    return false; // cannot compare different types
  }

  template <typename T> bool operator()(const T &lhs, const T &rhs) const {
    return lhs == rhs;
  }
};

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

  void test_data_is_stored_correctly_in_the_ADS() {
    const auto data = getIndirectFitData(1, 3);
    SetUpADSWithWorkspace ads("WorkspaceName", data);

    TS_ASSERT(AnalysisDataService::Instance().doesExist("WorkspaceName"));
    MatrixWorkspace_sptr workspace =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("WorkspaceName"));
    TS_ASSERT_EQUALS(workspace->getNumberHistograms(), 1);
  }

  void test_displayName_returns_correct_name() {
    const auto data = getIndirectFitData(1, 3);

    std::vector<std::string> formatStrings{
        "%1%_s%2%_Result", "%1%_f%2%,s%2%_Parameter", "%1%_s%2%_Parameter"};
    const std::string rangeDelimiter = "_to_";
    const std::size_t spectrum = 1;

    TS_ASSERT_EQUALS(data.displayName(formatStrings[0], rangeDelimiter),
                     "_s0_Result");
    TS_ASSERT_EQUALS(data.displayName(formatStrings[1], rangeDelimiter),
                     "_f0+s0_Parameter");
    TS_ASSERT_EQUALS(data.displayName(formatStrings[2], spectrum),
                     "_s1_Parameter");
  }

  void test_displayName_removes_correct_part_of_workspace_name() {
    const auto data = getIndirectFitData(1, 3);

    SetUpADSWithWorkspace ads("Workspace_3456_red", data);
    const std::string formatString = "%1%_s%2%_Result";
    const std::string rangeDelimiter = "_to_";

    TS_ASSERT_EQUALS(data.displayName(formatString, rangeDelimiter),
                     "Workspace_3456_s0_Result");
  }

  void test_that_correct_number_of_spectra_is_returned() {
    const auto data = getIndirectFitData(10, 3);
    TS_ASSERT_EQUALS(data.numberOfSpectra(), 10);
  }

  void test_that_correct_spectrum_number_is_returned() {
    const auto data = getIndirectFitData(4, 3);

    for (auto i = 0; i < data.numberOfSpectra(); ++i) {
      const std::size_t spectrumNum = data.getSpectrum(i);
      TS_ASSERT_EQUALS(spectrumNum, i);
    }
  }

  void
  test_that_true_is_returned_from_zeroSpectra_if_data_contains_empty_workspace() {
    auto workspace = boost::make_shared<Workspace2D>();
    const Spectra spec = std::make_pair(0u, 0u);
    const IndirectFitData data(workspace, spec);

    TS_ASSERT_EQUALS(data.zeroSpectra(), true);
  }

  void
  test_that_true_is_returned_from_zeroSpectra_if_data_contains_empty_spectra() {
    const auto workspace = WorkspaceCreationHelper::create2DWorkspace123(1, 3);
    const DiscontinuousSpectra<std::size_t> spec("");
    const IndirectFitData data(workspace, spec);

    TS_ASSERT_EQUALS(data.zeroSpectra(), true);
  }

  void
  test_that_false_is_returned_from_zeroSpectra_if_data_contains_one_or_more_spectra() {
    for (auto i = 1u; i < 10; ++i) {
      const auto data = getIndirectFitData(i, 3);
      TS_ASSERT_EQUALS(data.zeroSpectra(), false);
    }
  }

  void
  test_that_correct_excludeRegion_is_returned_when_regions_are_in_correct_order() {
    /// When each pair of numbers in the string are in order, then the whole
    /// string is in the correct order(unordered: 10,11 9,7 ordered:10,11,7,9)
    auto data = getIndirectFitData(4, 3);

    data.setExcludeRegionString("1,8", 0);
    data.setExcludeRegionString("2,5", 1);
    data.setExcludeRegionString("1,2,5,6,3,4", 2);

    TS_ASSERT_EQUALS(data.getExcludeRegion(0), "1.0,8.0");
    TS_ASSERT_EQUALS(data.getExcludeRegion(1), "2.0,5.0");
    TS_ASSERT_EQUALS(data.getExcludeRegion(2), "1.0,2.0,5.0,6.0,3.0,4.0");
    TS_ASSERT_EQUALS(data.getExcludeRegion(3), "");
  }

  void
  test_that_correct_excludeRegionVector_is_returned_when_regions_are_in_correct_order() {
    /// When each pair of numbers in the string are in order, then the whole
    /// string is in the correct order(unordered: 10,11 9,7 ordered:10,11,7,9)
    auto data = getIndirectFitData(4, 3);

    data.setExcludeRegionString("1,8", 0);
    data.setExcludeRegionString("2,5", 1);
    std::vector<double> regionVector1{1.0, 8.0};
    std::vector<double> regionVector2{2.0, 5.0};

    TS_ASSERT_EQUALS(data.excludeRegionsVector(0), regionVector1);
    TS_ASSERT_EQUALS(data.excludeRegionsVector(1), regionVector2);
    TS_ASSERT_EQUALS(data.excludeRegionsVector(3).empty(), true);
  }

  void test_that_excludeRegion_pairs_are_stored_in_correct_order() {
    /// Example: unordered: 10,11 9,7     ordered: 10,11,7,9
    auto data = getIndirectFitData(3, 3);

    data.setExcludeRegionString("6,2", 0);
    data.setExcludeRegionString("6,2,1,2,3,4,7,6", 1);
    data.setExcludeRegionString("1,2,2,3,20,18,21,22,7,8", 2);

    std::vector<double> regionVector{2.0, 6.0};

    TS_ASSERT_EQUALS(data.getExcludeRegion(0), "2.0,6.0");
    TS_ASSERT_EQUALS(data.getExcludeRegion(1),
                     "2.0,6.0,1.0,2.0,3.0,4.0,6.0,7.0");
    TS_ASSERT_EQUALS(data.getExcludeRegion(2),
                     "1.0,2.0,2.0,3.0,18.0,20.0,21.0,22.0,7.0,8.0");
    TS_ASSERT_EQUALS(data.excludeRegionsVector(0), regionVector);
  }

  void
  test_that_excludeRegion_is_stored_correctly_when_there_are_many_spaces_in_input_string() {
    auto data = getIndirectFitData(3, 3);

    data.setExcludeRegionString("  6,     2", 0);
    data.setExcludeRegionString("6,  2,1  ,2,  3,4  ,7,6", 1);
    data.setExcludeRegionString("1,2 ,2,3,  20,  18,21,22,7, 8   ", 2);

    TS_ASSERT_EQUALS(data.getExcludeRegion(0), "2.0,6.0");
    TS_ASSERT_EQUALS(data.getExcludeRegion(1),
                     "2.0,6.0,1.0,2.0,3.0,4.0,6.0,7.0");
    TS_ASSERT_EQUALS(data.getExcludeRegion(2),
                     "1.0,2.0,2.0,3.0,18.0,20.0,21.0,22.0,7.0,8.0");
  }

  void
  test_that_setExcludeRegion_correctly_rounds_the_numbers_in_the_input_string() {
    auto data = getIndirectFitData(2, 3);

    data.setExcludeRegionString("6.29,2.93", 0);
    data.setExcludeRegionString("2.6,2.3,1.99,3.01", 1);

    TS_ASSERT_EQUALS(data.getExcludeRegion(0), "2.9,6.3");
    TS_ASSERT_EQUALS(data.getExcludeRegion(1), "2.3,2.6,2.0,3.0");
  }

  void test_throws_when_setSpectra_is_provided_an_out_of_range_spectra() {
    auto data = getIndirectFitData(10, 3);

    std::vector<Spectra> spectraPairs{std::make_pair(0u, 11u),
                                      std::make_pair(0u, 1000000000000000000u),
                                      std::make_pair(10u, 10u)};
    std::vector<std::string> spectraStrings{"-1", "10",
                                            "100000000000000000000000000000",
                                            "1,5,10", "1,2,3,4,5,6,22"};

    for (auto i = 0u; i < spectraPairs.size(); ++i)
      TS_ASSERT_THROWS(data.setSpectra(spectraPairs[i]), std::runtime_error);
    for (auto i = 0u; i < spectraStrings.size(); ++i)
      TS_ASSERT_THROWS(data.setSpectra(spectraStrings[i]), std::runtime_error);
  }

  void test_no_throw_when_setSpectra_is_provided_a_valid_spectra() {
    auto data = getIndirectFitData(10, 3);

    std::vector<Spectra> spectraPairs{
        std::make_pair(0u, 9u), std::make_pair(4u, 4u), std::make_pair(7u, 4u)};
    std::vector<std::string> spectraStrings{"0", "9", "0,9,6,4,5",
                                            "1,2,3,4,5,6"};

    for (auto i = 0u; i < spectraPairs.size(); ++i)
      TS_ASSERT_THROWS_NOTHING(data.setSpectra(spectraPairs[i]));
    for (auto i = 0u; i < spectraStrings.size(); ++i)
      TS_ASSERT_THROWS_NOTHING(data.setSpectra(spectraStrings[i]));
  }

  void
  test_no_throw_when_setStartX_is_provided_a_valid_xValue_and_spectrum_number() {
    auto data = getIndirectFitData(10, 3);

    TS_ASSERT_THROWS_NOTHING(data.setStartX(0.0, 0));
    TS_ASSERT_THROWS_NOTHING(data.setStartX(-5.0, 0));
    TS_ASSERT_THROWS_NOTHING(data.setStartX(5000000, 10));
  }

  void test_correct_startX_is_stored_in_rangeafter_using_setStartX() {
    auto data = getIndirectFitData(3, 3);

    data.setStartX(-5.0, 0);
    data.setStartX(6.53, 1);
    data.setStartX(100000000000000.0, 2);

    TS_ASSERT_EQUALS(data.getRange(0).first, -5.0);
    TS_ASSERT_EQUALS(data.getRange(1).first, 6.53);
    TS_ASSERT_EQUALS(data.getRange(2).first, 100000000000000.0);
  }

  void
  test_no_throw_when_setEndX_is_provided_a_valid_xValue_and_spectrum_number() {
    auto data = getIndirectFitData(10, 3);

    TS_ASSERT_THROWS_NOTHING(data.setStartX(0.0, 0));
    TS_ASSERT_THROWS_NOTHING(data.setStartX(-5.0, 0));
    TS_ASSERT_THROWS_NOTHING(data.setStartX(5000000, 10));
  }

  void test_correct_endX_is_stored_in_rangeafter_using_setEndX() {
    auto data = getIndirectFitData(3, 3);

    data.setEndX(-5.0, 0);
    data.setEndX(6.53, 1);
    data.setEndX(100000000000000.0, 2);

    TS_ASSERT_EQUALS(data.getRange(0).second, -5.0);
    TS_ASSERT_EQUALS(data.getRange(1).second, 6.53);
    TS_ASSERT_EQUALS(data.getRange(2).second, 100000000000000.0);
  }

  void test_that_the_startX_of_two_data_sets_are_combined_correctly() {
    auto data1 = getIndirectFitData(2, 3);
    auto data2 = getIndirectFitData(2, 3);

    data1.setStartX(6.53, 0);
    data2.setStartX(5.0, 1);
    auto combinedData = data2.combine(data1);

    TS_ASSERT_EQUALS(combinedData.getRange(0).first, 6.53);
    TS_ASSERT_EQUALS(combinedData.getRange(1).first, 5.0);
  }

  void test_that_the_endX_of_two_datasets_are_combined_correctly() {
    auto data1 = getIndirectFitData(2, 3);
    auto data2 = getIndirectFitData(2, 3);

    data1.setEndX(2.34, 0);
    data2.setEndX(5.9, 1);
    auto combinedData = data2.combine(data1);

    TS_ASSERT_EQUALS(combinedData.getRange(0).second, 2.34);
    TS_ASSERT_EQUALS(combinedData.getRange(1).second, 5.9);
  }

  void test_that_the_excludeRegion_of_two_datasets_are_combined_correctly() {
    auto data1 = getIndirectFitData(2, 3);
    auto data2 = getIndirectFitData(2, 3);

    data1.setExcludeRegionString("1,2,6,5", 0);
    data1.setExcludeRegionString("6,2", 1);
    auto combinedData = data2.combine(data1);

    TS_ASSERT_EQUALS(combinedData.getExcludeRegion(0), "1.0,2.0,5.0,6.0");
    TS_ASSERT_EQUALS(combinedData.getExcludeRegion(1), "2.0,6.0");
  }

  void
  test_that_the_spectra_pair_of_two_datasets_are_combined_correctly_when_spectra_do_not_overlap() {
    auto data1 = getIndirectFitData(10, 3);
    auto data2 = getIndirectFitData(10, 3);

    data1.setSpectra(std::make_pair(0u, 4u));
    data2.setSpectra(std::make_pair(5u, 9u));
    auto combinedData = data2.combine(data1);
    Spectra spec(std::make_pair(0u, 9u));

    TS_ASSERT(
        boost::apply_visitor(AreSpectraEqual(), combinedData.spectra(), spec));
  }

  void
  test_that_the_spectra_pair_of_two_datasets_are_combined_correctly_when_spectra_are_discontinuous() {
    auto data1 = getIndirectFitData(10, 3);
    auto data2 = getIndirectFitData(10, 3);

    data1.setSpectra(std::make_pair(0u, 4u));
    data2.setSpectra(std::make_pair(8u, 9u));
    auto combinedData = data2.combine(data1);
    Spectra spec(DiscontinuousSpectra<std::size_t>("0-4,8-9"));

    TS_ASSERT(
        boost::apply_visitor(AreSpectraEqual(), combinedData.spectra(), spec));
  }

  void
  test_that_the_spectra_pair_of_two_datasets_are_combined_correctly_when_spectra_overlap() {
    auto data1 = getIndirectFitData(10, 3);
    auto data2 = getIndirectFitData(10, 3);

    data1.setSpectra(std::make_pair(0u, 8u));
    data2.setSpectra(std::make_pair(4u, 9u));
    auto combinedData = data2.combine(data1);
    // Spectra spec(std::make_pair(0u, 9u));
    Spectra spec(DiscontinuousSpectra<std::size_t>("0-9"));

    TS_ASSERT(
        boost::apply_visitor(AreSpectraEqual(), combinedData.spectra(), spec));
  }

  void
  test_that_the_DiscontinousSpectra_of_two_datasets_are_combined_correctly_when_spectra_do_not_overlap() {
    auto data1 = getIndirectFitData(10, 3);
    auto data2 = getIndirectFitData(10, 3);

    data1.setSpectra(DiscontinuousSpectra<std::size_t>("0-4"));
    data2.setSpectra(DiscontinuousSpectra<std::size_t>("5-9"));
    auto combinedData = data2.combine(data1);
    Spectra spec(DiscontinuousSpectra<std::size_t>("0-9"));

    TS_ASSERT(
        boost::apply_visitor(AreSpectraEqual(), combinedData.spectra(), spec));
  }

  void
  test_that_the_DiscontinousSpectra_of_two_datasets_are_combined_correctly_when_spectra_overlap() {
    auto data1 = getIndirectFitData(10, 3);
    auto data2 = getIndirectFitData(10, 3);

    data1.setSpectra(DiscontinuousSpectra<std::size_t>("0-7"));
    data2.setSpectra(DiscontinuousSpectra<std::size_t>("2-9"));
    auto combinedData = data2.combine(data1);
    Spectra spec(DiscontinuousSpectra<std::size_t>("0-9"));

    TS_ASSERT(
        boost::apply_visitor(AreSpectraEqual(), combinedData.spectra(), spec));
  }

  void
  test_that_a_SpectraPair_and_DiscontinousSpectra_dataset_are_combined_correctly_when_spectra_do_not_overlap() {
    auto data1 = getIndirectFitData(10, 3);
    auto data2 = getIndirectFitData(10, 3);

    data1.setSpectra(DiscontinuousSpectra<std::size_t>("0-4"));
    data2.setSpectra(std::make_pair(5u, 9u));
    auto combinedData = data2.combine(data1);
    Spectra spec(DiscontinuousSpectra<std::size_t>("0-9"));

    TS_ASSERT(
        boost::apply_visitor(AreSpectraEqual(), combinedData.spectra(), spec));
  }

  void
  test_that_a_SpectraPair_and_DiscontinousSpectra_dataset_are_combined_correctly_when_spectra_overlap() {
    auto data1 = getIndirectFitData(10, 3);
    auto data2 = getIndirectFitData(10, 3);

    data1.setSpectra(DiscontinuousSpectra<std::size_t>("0-7"));
    data2.setSpectra(std::make_pair(4u, 9u));
    auto combinedData = data2.combine(data1);
    Spectra spec(DiscontinuousSpectra<std::size_t>("0-9"));

    TS_ASSERT(
        boost::apply_visitor(AreSpectraEqual(), combinedData.spectra(), spec));
  }
};

#endif
