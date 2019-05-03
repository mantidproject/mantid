// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDIRECTFITDATATEST_H_
#define MANTID_INDIRECTFITDATATEST_H_

#include <cxxtest/TestSuite.h>

#include "IndirectFitData.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace Mantid::IndirectFitDataCreationHelper;

namespace {

std::unique_ptr<IndirectFitData>
getIndirectFitData(int const &numberOfSpectra) {
  auto const workspace = createWorkspace(numberOfSpectra);
  Spectra const spec = Spectra(0u, workspace->getNumberHistograms() - 1);
  IndirectFitData data(workspace, spec);
  return std::make_unique<IndirectFitData>(data);
}

} // namespace

class IndirectFitDataTest : public CxxTest::TestSuite {
public:
  static IndirectFitDataTest *createSuite() {
    return new IndirectFitDataTest();
  }

  static void destroySuite(IndirectFitDataTest *suite) { delete suite; }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_data_is_instantiated() {
    auto const workspace = createWorkspace(1);
    Spectra const spec =
      Spectra(0u, workspace->getNumberHistograms() - 1);

    workspace->setTitle("Test Title");
    IndirectFitData const data(workspace, spec);

    TS_ASSERT_EQUALS(data.workspace(), workspace);
    TS_ASSERT_EQUALS(data.workspace()->getTitle(), "Test Title");
    TS_ASSERT_EQUALS(data.workspace()->getNumberHistograms(), 1);
  }

  void test_that_DiscontinuousSpectra_is_set_up_correctly() {
    Spectra const spectra =
        Spectra("0-5,8,10");

    std::string const spectraString = "0-5,8,10";
    std::vector<std::size_t> const spectraVec{0, 1, 2, 3, 4, 5, 8, 10};

    TS_ASSERT_EQUALS(spectra.getString(), spectraString);
    for (auto it = spectra.begin(); it < spectra.end(); ++it)
      TS_ASSERT_EQUALS(*it, spectraVec[it - spectra.begin()]);
  }

  void
  test_that_DiscontinuousSpectra_is_sorted_before_being_stored_when_the_input_string_contains_overlapping_spectra() {
    auto data = getIndirectFitData(11);

    std::string const inputString = "8,0-7,6,10";
    Spectra const spectra = Spectra("0-8,10");
    data->setSpectra(inputString);

    TS_ASSERT_EQUALS(data->spectra(), spectra);
  }

  void
  test_that_DiscontinuousSpectra_is_sorted_before_being_stored_when_the_input_string_contains_an_invalid_spectra_range() {
    auto data = getIndirectFitData(11);

    std::string const inputString = "1,2,4-3,10";
    Spectra const spectra = Spectra("1-4,10");
    data->setSpectra(inputString);

    TS_ASSERT_EQUALS(data->spectra(), spectra);
  }

  void
  test_that_DiscontinuousSpectra_is_sorted_before_being_stored_when_the_input_string_contains_large_spaces() {
    auto data = getIndirectFitData(11);

    std::string const inputString = "  8,10,  7";
    Spectra const spectra = Spectra("7-8,10");
    data->setSpectra(inputString);

    TS_ASSERT_EQUALS(data->spectra(), spectra);
  }

  void test_data_is_stored_in_the_ADS() {
    auto const data = getIndirectFitData(1);
    SetUpADSWithWorkspace ads("WorkspaceName", data->workspace());

    TS_ASSERT(ads.doesExist("WorkspaceName"));
    auto workspace = ads.retrieveWorkspace("WorkspaceName");
    TS_ASSERT_EQUALS(workspace->getNumberHistograms(), 1);
  }

  void
  test_displayName_returns_a_valid_name_when_provided_a_rangeDelimiter_and_spectrum_number() {
    auto const data = getIndirectFitData(1);

    std::vector<std::string> const formatStrings{
        "%1%_s%2%_Result", "%1%_f%2%,s%2%_Parameter", "%1%_s%2%_Parameter"};
    std::string const rangeDelimiter = "_to_";
    std::size_t const spectrum = 1;

    TS_ASSERT_EQUALS(data->displayName(formatStrings[0], rangeDelimiter),
                     "_s0_Result");
    TS_ASSERT_EQUALS(data->displayName(formatStrings[1], rangeDelimiter),
                     "_f0+s0_Parameter");
    TS_ASSERT_EQUALS(data->displayName(formatStrings[2], spectrum),
                     "_s1_Parameter");
  }

  void test_displayName_removes_red_part_of_a_workspace_name() {
    auto const data = getIndirectFitData(1);

    SetUpADSWithWorkspace ads("Workspace_3456_red", data->workspace());
    std::string const formatString = "%1%_s%2%_Result";
    std::string const rangeDelimiter = "_to_";

    TS_ASSERT_EQUALS(data->displayName(formatString, rangeDelimiter),
                     "Workspace_3456_s0_Result");
  }

  void
  test_that_the_number_of_spectra_returned_matches_the_instantiated_value() {
    auto const data = getIndirectFitData(10);
    TS_ASSERT_EQUALS(data->numberOfSpectra(), 10);
  }

  void test_that_getSpectrum_returns_the_expected_spectrum_numbers() {
    auto const data = getIndirectFitData(4);

    for (auto i = 0u; i < data->numberOfSpectra(); ++i) {
      std::size_t const spectrumNum = data->getSpectrum(i);
      TS_ASSERT_EQUALS(spectrumNum, i);
    }
  }

  void
  test_that_true_is_returned_from_zeroSpectra_if_data_contains_empty_workspace() {
    auto workspace = boost::make_shared<Workspace2D>();
    Spectra const spec = Spectra(0u, 0u);
    IndirectFitData const data(workspace, spec);

    TS_ASSERT_EQUALS(data.zeroSpectra(), true);
  }

  void
  test_that_true_is_returned_from_zeroSpectra_if_data_contains_empty_spectra() {
    auto const workspace = createWorkspace(1);
    Spectra const spec("");
    IndirectFitData const data(workspace, spec);

    TS_ASSERT_EQUALS(data.zeroSpectra(), true);
  }

  void
  test_that_false_is_returned_from_zeroSpectra_if_data_contains_one_or_more_spectra() {
    for (auto i = 1u; i < 10; ++i) {
      auto const data = getIndirectFitData(i);
      TS_ASSERT_EQUALS(data->zeroSpectra(), false);
    }
  }

  void
  test_that_correct_excludeRegion_is_returned_when_regions_are_in_correct_order() {
    /// When each pair of numbers in the string are in order, then the whole
    /// string is in the correct order(unordered: 10,11 9,7 ordered:10,11,7,9)
    auto data = getIndirectFitData(4);

    data->setExcludeRegionString("1,8", 0);
    data->setExcludeRegionString("2,5", 1);
    data->setExcludeRegionString("1,2,5,6,3,4", 2);

    TS_ASSERT_EQUALS(data->getExcludeRegion(0), "1.000,8.000");
    TS_ASSERT_EQUALS(data->getExcludeRegion(1), "2.000,5.000");
    TS_ASSERT_EQUALS(data->getExcludeRegion(2),
                     "1.000,2.000,5.000,6.000,3.000,4.000");
    TS_ASSERT_EQUALS(data->getExcludeRegion(3), "");
  }

  void
  test_that_correct_excludeRegionVector_is_returned_when_regions_are_in_correct_order() {
    /// When each pair of numbers in the string are in order, then the whole
    /// string is in the correct order(unordered: 10,11 9,7 ordered:10,11,7,9)
    auto data = getIndirectFitData(4);

    data->setExcludeRegionString("1,8", 0);
    data->setExcludeRegionString("2,5", 1);
    std::vector<double> const regionVector1{1.0, 8.0};
    std::vector<double> const regionVector2{2.0, 5.0};

    TS_ASSERT_EQUALS(data->excludeRegionsVector(0), regionVector1);
    TS_ASSERT_EQUALS(data->excludeRegionsVector(1), regionVector2);
    TS_ASSERT_EQUALS(data->excludeRegionsVector(3).empty(), true);
  }

  void test_that_excludeRegion_pairs_are_stored_in_an_order_of_low_to_high() {
    /// Example: unordered: 10,11 9,7     ordered: 10,11,7,9
    auto data = getIndirectFitData(3);

    data->setExcludeRegionString("6,2", 0);
    data->setExcludeRegionString("6,2,1,2,3,4,7,6", 1);
    data->setExcludeRegionString("1,2,2,3,20,18,21,22,7,8", 2);

    std::vector<double> const regionVector{2.0, 6.0};

    TS_ASSERT_EQUALS(data->getExcludeRegion(0), "2.000,6.000");
    TS_ASSERT_EQUALS(data->getExcludeRegion(1),
                     "2.000,6.000,1.000,2.000,3.000,4.000,6.000,7.000");
    TS_ASSERT_EQUALS(
        data->getExcludeRegion(2),
        "1.000,2.000,2.000,3.000,18.000,20.000,21.000,22.000,7.000,8.000");
    TS_ASSERT_EQUALS(data->excludeRegionsVector(0), regionVector);
  }

  void
  test_that_excludeRegion_is_stored_without_spaces_when_there_are_many_spaces_in_input_string() {
    auto data = getIndirectFitData(3);

    data->setExcludeRegionString("  6,     2", 0);
    data->setExcludeRegionString("6,  2,1  ,2,  3,4  ,7,6", 1);
    data->setExcludeRegionString("1,2 ,2,3,  20,  18,21,22,7, 8   ", 2);

    TS_ASSERT_EQUALS(data->getExcludeRegion(0), "2.000,6.000");
    TS_ASSERT_EQUALS(data->getExcludeRegion(1),
                     "2.000,6.000,1.000,2.000,3.000,4.000,6.000,7.000");
    TS_ASSERT_EQUALS(
        data->getExcludeRegion(2),
        "1.000,2.000,2.000,3.000,18.000,20.000,21.000,22.000,7.000,8.000");
  }

  void
  test_that_setExcludeRegion_rounds_the_numbers_in_the_input_string_to_the_appropriate_decimal_place() {
    auto data = getIndirectFitData(2);

    data->setExcludeRegionString("6.29445,2.93343", 0);
    data->setExcludeRegionString("2.6,2.3,1.9999,3.0125", 1);

    TS_ASSERT_EQUALS(data->getExcludeRegion(0), "2.933,6.294");
    TS_ASSERT_EQUALS(data->getExcludeRegion(1), "2.300,2.600,2.000,3.013");
  }

  void test_throws_when_setSpectra_is_provided_an_out_of_range_spectra() {
    auto data = getIndirectFitData(10);

    std::vector<Spectra> const spectraPairs{
      Spectra(0u, 11u), Spectra(0u, 1000u),
      Spectra(10u, 10u)};
    std::vector<std::string> const spectraStrings{
        "10", "1000", "1,5,10", "1,2,3,4,5,6,22"};

    for (auto i = 0u; i < spectraPairs.size(); ++i)
      TS_ASSERT_THROWS(data->setSpectra(spectraPairs[i]), std::runtime_error);
    for (auto i = 0u; i < spectraStrings.size(); ++i)
      TS_ASSERT_THROWS(data->setSpectra(spectraStrings[i]), std::runtime_error);
  }

  void test_no_throw_when_setSpectra_is_provided_a_valid_spectra() {
    auto data = getIndirectFitData(10);

    std::vector<Spectra> const spectraPairs{
      Spectra(0u, 9u), Spectra(4u, 4u), Spectra(7u, 4u)};
    std::vector<std::string> const spectraStrings{"0", "9", "0,9,6,4,5",
                                                  "1,2,3,4,5,6"};

    for (auto i = 0u; i < spectraPairs.size(); ++i)
      TS_ASSERT_THROWS_NOTHING(data->setSpectra(spectraPairs[i]));
    for (auto i = 0u; i < spectraStrings.size(); ++i)
      TS_ASSERT_THROWS_NOTHING(data->setSpectra(spectraStrings[i]));
  }

  void
  test_no_throw_when_setStartX_is_provided_a_valid_xValue_and_spectrum_number() {
    auto data = getIndirectFitData(10);

    TS_ASSERT_THROWS_NOTHING(data->setStartX(0.0, 0));
    TS_ASSERT_THROWS_NOTHING(data->setStartX(-5.0, 0));
    TS_ASSERT_THROWS_NOTHING(data->setStartX(5000000, 10));
  }

  void test_the_provided_startX_is_stored_in_range_after_using_setStartX() {
    auto data = getIndirectFitData(3);

    data->setStartX(-5.0, 0);
    data->setStartX(6.53, 1);
    data->setStartX(100000000000000.0, 2);

    TS_ASSERT_EQUALS(data->getRange(0).first, -5.0);
    TS_ASSERT_EQUALS(data->getRange(1).first, 6.53);
    TS_ASSERT_EQUALS(data->getRange(2).first, 100000000000000.0);
  }

  void
  test_no_throw_when_setEndX_is_provided_a_valid_xValue_and_spectrum_number() {
    auto data = getIndirectFitData(10);

    TS_ASSERT_THROWS_NOTHING(data->setStartX(0.0, 0));
    TS_ASSERT_THROWS_NOTHING(data->setStartX(-5.0, 0));
    TS_ASSERT_THROWS_NOTHING(data->setStartX(5000000, 10));
  }

  void test_the_provided_endX_is_stored_in_range_after_using_setEndX() {
    auto data = getIndirectFitData(3);

    data->setEndX(-5.0, 0);
    data->setEndX(6.53, 1);
    data->setEndX(100000000000000.0, 2);

    TS_ASSERT_EQUALS(data->getRange(0).second, -5.0);
    TS_ASSERT_EQUALS(data->getRange(1).second, 6.53);
    TS_ASSERT_EQUALS(data->getRange(2).second, 100000000000000.0);
  }

  void
  test_that_the_startX_of_two_data_sets_are_combined_when_relating_to_two_seperate_spectra() {
    auto data1 = getIndirectFitData(2);
    auto data2 = getIndirectFitData(2);

    data1->setStartX(6.53, 0);
    data2->setStartX(5.0, 1);
    auto const combinedData = data2->combine(*data1);

    TS_ASSERT_EQUALS(combinedData.getRange(0).first, 6.53);
    TS_ASSERT_EQUALS(combinedData.getRange(1).first, 5.0);
  }

  void
  test_that_the_endX_of_two_datasets_are_combined_when_relating_to_two_seperate_spectra() {
    auto data1 = getIndirectFitData(2);
    auto data2 = getIndirectFitData(2);

    data1->setEndX(2.34, 0);
    data2->setEndX(5.9, 1);
    auto const combinedData = data2->combine(*data1);

    TS_ASSERT_EQUALS(combinedData.getRange(0).second, 2.34);
    TS_ASSERT_EQUALS(combinedData.getRange(1).second, 5.9);
  }

  void
  test_that_the_excludeRegion_of_two_datasets_are_combined_when_relating_to_two_seperate_spectra() {
    auto data1 = getIndirectFitData(2);
    auto data2 = getIndirectFitData(2);

    data1->setExcludeRegionString("1,2,6,5", 0);
    data1->setExcludeRegionString("6,2", 1);
    auto const combinedData = data2->combine(*data1);

    TS_ASSERT_EQUALS(combinedData.getExcludeRegion(0),
                     "1.000,2.000,5.000,6.000");
    TS_ASSERT_EQUALS(combinedData.getExcludeRegion(1), "2.000,6.000");
  }

  void
  test_that_the_spectra_pair_of_two_datasets_are_combined_correctly_when_spectra_do_not_overlap() {
    auto data1 = getIndirectFitData(10);
    auto data2 = getIndirectFitData(10);

    data1->setSpectra(Spectra(0u, 4u));
    data2->setSpectra(Spectra(5u, 9u));
    auto const combinedData = data2->combine(*data1);
    Spectra const spec(Spectra(0u, 9u));

    TS_ASSERT_EQUALS(combinedData.spectra(), spec);
  }

  void
  test_that_the_spectra_pair_of_two_datasets_are_combined_correctly_when_spectra_are_discontinuous() {
    auto data1 = getIndirectFitData(10);
    auto data2 = getIndirectFitData(10);

    data1->setSpectra(Spectra(0u, 4u));
    data2->setSpectra(Spectra(8u, 9u));
    auto const combinedData = data2->combine(*data1);
    Spectra const spec(Spectra("0-4,8-9"));

    TS_ASSERT_EQUALS(combinedData.spectra(), spec);
  }

  void
  test_that_the_spectra_pair_of_two_datasets_are_combined_correctly_when_spectra_overlap() {
    auto data1 = getIndirectFitData(10);
    auto data2 = getIndirectFitData(10);

    data1->setSpectra(Spectra(0u, 8u));
    data2->setSpectra(Spectra(4u, 9u));
    auto const combinedData = data2->combine(*data1);
    Spectra const spec(Spectra("0-9"));

    TS_ASSERT_EQUALS(combinedData.spectra(), spec);
  }

  void
  test_that_the_DiscontinuousSpectra_of_two_datasets_are_combined_correctly_when_spectra_do_not_overlap() {
    auto data1 = getIndirectFitData(10);
    auto data2 = getIndirectFitData(10);

    data1->setSpectra(Spectra("0-4"));
    data2->setSpectra(Spectra("5-9"));
    auto const combinedData = data2->combine(*data1);
    Spectra const spec(Spectra("0-9"));

    TS_ASSERT_EQUALS(combinedData.spectra(), spec);
  }

  void
  test_that_the_DiscontinuousSpectra_of_two_datasets_are_combined_correctly_when_spectra_overlap() {
    auto data1 = getIndirectFitData(10);
    auto data2 = getIndirectFitData(10);

    data1->setSpectra(Spectra("0-7"));
    data2->setSpectra(Spectra("2-9"));
    auto const combinedData = data2->combine(*data1);
    Spectra const spec(Spectra("0-9"));

    TS_ASSERT_EQUALS(combinedData.spectra(), spec);
  }

  void
  test_that_a_Spectra_pair_and_DiscontinuousSpectra_dataset_are_combined_correctly_when_spectra_do_not_overlap() {
    auto data1 = getIndirectFitData(10);
    auto data2 = getIndirectFitData(10);

    data1->setSpectra(Spectra("0-4"));
    data2->setSpectra(Spectra(5u, 9u));
    auto const combinedData = data2->combine(*data1);
    Spectra const spec(Spectra("0-9"));

    TS_ASSERT_EQUALS(combinedData.spectra(), spec);
  }

  void
  test_that_a_Spectra_pair_and_DiscontinuousSpectra_dataset_are_combined_correctly_when_spectra_overlap() {
    auto data1 = getIndirectFitData(10);
    auto data2 = getIndirectFitData(10);

    data1->setSpectra(Spectra("0-7"));
    data2->setSpectra(Spectra(4u, 9u));
    auto const combinedData = data2->combine(*data1);
    Spectra const spec(Spectra("0-9"));

    TS_ASSERT_EQUALS(combinedData.spectra(), spec);
  }
};

#endif
