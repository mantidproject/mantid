// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidQtWidgets/Spectroscopy/FitData.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitConversion.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace MantidQt::CustomInterfaces::Inelastic;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;

namespace {

std::unique_ptr<FitData> getFitData(int const &numberOfSpectra) {
  auto const workspace = createWorkspace(numberOfSpectra);
  FunctionModelSpectra const spec =
      FunctionModelSpectra(WorkspaceIndex{0}, WorkspaceIndex{workspace->getNumberHistograms() - 1});
  FitData data(workspace, spec);
  return std::make_unique<FitData>(data);
}

} // namespace

class FitDataTest : public CxxTest::TestSuite {
public:
  static FitDataTest *createSuite() { return new FitDataTest(); }

  static void destroySuite(FitDataTest *suite) { delete suite; }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_data_is_instantiated() {
    auto const workspace = createWorkspace(1);
    FunctionModelSpectra const spec =
        FunctionModelSpectra(WorkspaceIndex{0}, WorkspaceIndex{workspace->getNumberHistograms() - 1});

    workspace->setTitle("Test Title");
    FitData const data(workspace, spec);

    TS_ASSERT_EQUALS(data.workspace(), workspace);
    TS_ASSERT_EQUALS(data.workspace()->getTitle(), "Test Title");
    TS_ASSERT_EQUALS(data.workspace()->getNumberHistograms(), 1);
  }

  void test_that_DiscontinuousSpectra_is_set_up_correctly() {
    FunctionModelSpectra const spectra = FunctionModelSpectra("0-5,8,10");

    std::string const spectraString = "0-5,8,10";
    std::vector<WorkspaceIndex> const spectraVec{WorkspaceIndex(0), WorkspaceIndex(1), WorkspaceIndex(2),
                                                 WorkspaceIndex(3), WorkspaceIndex(4), WorkspaceIndex(5),
                                                 WorkspaceIndex(8), WorkspaceIndex(10)};

    TS_ASSERT_EQUALS(spectra.getString(), spectraString);
    for (auto it = spectra.begin(); it < spectra.end(); ++it)
      TS_ASSERT_EQUALS(*it, spectraVec[it - spectra.begin()]);
  }

  void
  test_that_DiscontinuousSpectra_is_sorted_before_being_stored_when_the_input_string_contains_overlapping_spectra() {
    auto data = getFitData(11);

    std::string const inputString = "8,0-7,6,10";
    FunctionModelSpectra const spectra = FunctionModelSpectra("0-8,10");
    data->setSpectra(inputString);

    TS_ASSERT_EQUALS(data->spectra(), spectra);
  }

  void
  test_that_DiscontinuousSpectra_is_sorted_before_being_stored_when_the_input_string_contains_an_invalid_spectra_range() {
    auto data = getFitData(11);

    std::string const inputString = "1,2,4-3,10";
    FunctionModelSpectra const spectra = FunctionModelSpectra("1-4,10");
    data->setSpectra(inputString);

    TS_ASSERT_EQUALS(data->spectra(), spectra);
  }

  void test_that_DiscontinuousSpectra_is_sorted_before_being_stored_when_the_input_string_contains_large_spaces() {
    auto data = getFitData(11);

    std::string const inputString = "  8,10,  7";
    FunctionModelSpectra const spectra = FunctionModelSpectra("7-8,10");
    data->setSpectra(inputString);

    TS_ASSERT_EQUALS(data->spectra(), spectra);
  }

  void test_erasing_non_existent_spectra_handled_gracefully() {
    FunctionModelSpectra spectra = FunctionModelSpectra("7-8,10");
    spectra.erase(WorkspaceIndex{9});
    TS_ASSERT_EQUALS(FunctionModelSpectra("7-8,10"), spectra);
  }

  void test_data_is_stored_in_the_ADS() {
    auto const data = getFitData(1);
    SetUpADSWithWorkspace ads("WorkspaceName", data->workspace());

    TS_ASSERT(ads.doesExist("WorkspaceName"));
    auto workspace = ads.retrieveWorkspace("WorkspaceName");
    TS_ASSERT_EQUALS(workspace->getNumberHistograms(), 1);
  }

  void test_displayName_returns_a_valid_name_when_provided_a_rangeDelimiter_and_spectrum_number() {
    auto const data = getFitData(1);

    std::vector<std::string> const formatStrings{"%1%_s%2%_Result", "%1%_f%2%,s%2%_Parameter", "%1%_s%2%_Parameter"};
    std::string const rangeDelimiter = "_to_";
    WorkspaceIndex const spectrum = WorkspaceIndex{1};

    TS_ASSERT_EQUALS(data->displayName(formatStrings[0], rangeDelimiter), "_s0_Result");
    TS_ASSERT_EQUALS(data->displayName(formatStrings[1], rangeDelimiter), "_f0+s0_Parameter");
    TS_ASSERT_EQUALS(data->displayName(formatStrings[2], spectrum), "_s1_Parameter");
  }

  void test_displayName_removes_red_part_of_a_workspace_name() {
    auto const data = getFitData(1);

    SetUpADSWithWorkspace ads("Workspace_3456_red", data->workspace());
    std::string const formatString = "%1%_s%2%_Result";
    std::string const rangeDelimiter = "_to_";

    TS_ASSERT_EQUALS(data->displayName(formatString, rangeDelimiter), "Workspace_3456_s0_Result");
  }

  void test_that_the_number_of_spectra_returned_matches_the_instantiated_value() {
    auto const data = getFitData(10);
    TS_ASSERT_EQUALS(data->numberOfSpectra(), FitDomainIndex{10});
  }

  void test_that_getSpectrum_returns_the_expected_spectrum_numbers() {
    auto const data = getFitData(4);

    for (auto i = FitDomainIndex{0}; i < data->numberOfSpectra(); ++i) {
      WorkspaceIndex const spectrumNum = data->getSpectrum(i);
      TS_ASSERT_EQUALS(spectrumNum.value, i.value);
    }
  }

  void test_that_true_is_returned_from_zeroSpectra_if_data_contains_empty_workspace() {
    auto workspace = std::make_shared<Workspace2D>();
    FunctionModelSpectra const spec = FunctionModelSpectra("");
    FitData const data(workspace, spec);

    TS_ASSERT_EQUALS(data.zeroSpectra(), true);
  }

  void test_that_true_is_returned_from_zeroSpectra_if_data_contains_empty_spectra() {
    auto const workspace = createWorkspace(1);
    FunctionModelSpectra const spec("");
    FitData const data(workspace, spec);

    TS_ASSERT_EQUALS(data.zeroSpectra(), true);
  }

  void test_that_false_is_returned_from_zeroSpectra_if_data_contains_one_or_more_spectra() {
    for (auto i = 1u; i < 10; ++i) {
      auto const data = getFitData(i);
      TS_ASSERT_EQUALS(data->zeroSpectra(), false);
    }
  }

  void test_that_correct_excludeRegion_is_returned_when_regions_are_in_correct_order() {
    /// When each pair of numbers in the string are in order, then the whole
    /// string is in the correct order(unordered: 10,11 9,7
    /// ordered:10,11,7,9)
    auto data = getFitData(4);

    data->setExcludeRegionString("1,8", WorkspaceIndex{0});
    data->setExcludeRegionString("2,5", WorkspaceIndex{1});
    data->setExcludeRegionString("1,2,5,6,3,4", WorkspaceIndex{2});

    TS_ASSERT_EQUALS(data->getExcludeRegion(WorkspaceIndex{0}), "1.000,8.000");
    TS_ASSERT_EQUALS(data->getExcludeRegion(WorkspaceIndex{1}), "2.000,5.000");
    TS_ASSERT_EQUALS(data->getExcludeRegion(WorkspaceIndex{2}), "1.000,2.000,5.000,6.000,3.000,4.000");
    TS_ASSERT_EQUALS(data->getExcludeRegion(WorkspaceIndex{3}), "");
  }

  void test_that_correct_excludeRegionVector_is_returned_when_regions_are_in_correct_order() {
    /// When each pair of numbers in the string are in order, then the whole
    /// string is in the correct order(unordered: 10,11 9,7
    /// ordered:10,11,7,9)
    auto data = getFitData(4);

    data->setExcludeRegionString("1,8", WorkspaceIndex{0});
    data->setExcludeRegionString("2,5", WorkspaceIndex{1});
    std::vector<double> const regionVector1{1.0, 8.0};
    std::vector<double> const regionVector2{2.0, 5.0};

    TS_ASSERT_EQUALS(data->excludeRegionsVector(WorkspaceIndex{0}), regionVector1);
    TS_ASSERT_EQUALS(data->excludeRegionsVector(WorkspaceIndex{1}), regionVector2);
    TS_ASSERT_EQUALS(data->excludeRegionsVector(WorkspaceIndex{3}).empty(), true);
  }

  void test_that_excludeRegion_pairs_are_stored_in_an_order_of_low_to_high() {
    /// Example: unordered: 10,11 9,7     ordered: 10,11,7,9
    auto data = getFitData(3);

    data->setExcludeRegionString("6,2", WorkspaceIndex{0});
    data->setExcludeRegionString("6,2,1,2,3,4,7,6", WorkspaceIndex{1});
    data->setExcludeRegionString("1,2,2,3,20,18,21,22,7,8", WorkspaceIndex{2});

    std::vector<double> const regionVector{2.0, 6.0};

    TS_ASSERT_EQUALS(data->getExcludeRegion(WorkspaceIndex{0}), "2.000,6.000");
    TS_ASSERT_EQUALS(data->getExcludeRegion(WorkspaceIndex{1}), "2.000,6.000,1.000,2.000,3.000,4.000,6.000,7.000");
    TS_ASSERT_EQUALS(data->getExcludeRegion(WorkspaceIndex{2}),
                     "1.000,2.000,2.000,3.000,18.000,20.000,21.000,22.000,7.000,8.000");
    TS_ASSERT_EQUALS(data->excludeRegionsVector(WorkspaceIndex{0}), regionVector);
  }

  void test_that_excludeRegion_is_stored_without_spaces_when_there_are_many_spaces_in_input_string() {
    auto data = getFitData(3);

    data->setExcludeRegionString("  6,     2", WorkspaceIndex{0});
    data->setExcludeRegionString("6,  2,1  ,2,  3,4  ,7,6", WorkspaceIndex{1});
    data->setExcludeRegionString("1,2 ,2,3,  20,  18,21,22,7, 8   ", WorkspaceIndex{2});

    TS_ASSERT_EQUALS(data->getExcludeRegion(WorkspaceIndex{0}), "2.000,6.000");
    TS_ASSERT_EQUALS(data->getExcludeRegion(WorkspaceIndex{1}), "2.000,6.000,1.000,2.000,3.000,4.000,6.000,7.000");
    TS_ASSERT_EQUALS(data->getExcludeRegion(WorkspaceIndex{2}),
                     "1.000,2.000,2.000,3.000,18.000,20.000,21.000,22.000,7.000,8.000");
  }

  void test_that_setExcludeRegion_rounds_the_numbers_in_the_input_string_to_the_appropriate_decimal_place() {
    auto data = getFitData(2);

    data->setExcludeRegionString("6.29445,2.93343", WorkspaceIndex{0});
    data->setExcludeRegionString("2.6,2.3,1.9999,3.0125", WorkspaceIndex{1});

    TS_ASSERT_EQUALS(data->getExcludeRegion(WorkspaceIndex{0}), "2.933,6.294");
    TS_ASSERT_EQUALS(data->getExcludeRegion(WorkspaceIndex{1}), "2.300,2.600,2.000,3.013");
  }

  void test_throws_when_setSpectra_is_provided_an_out_of_range_spectra() {
    auto data = getFitData(10);

    std::vector<FunctionModelSpectra> const spectraPairs{FunctionModelSpectra(WorkspaceIndex{0}, WorkspaceIndex{11}),
                                                         FunctionModelSpectra(WorkspaceIndex{10}, WorkspaceIndex{10})};
    std::vector<std::string> const spectraStrings{"10", "1,5,10", "1,2,3,4,5,6,22"};

    for (auto i = 0u; i < spectraPairs.size(); ++i)
      TS_ASSERT_THROWS(data->setSpectra(spectraPairs[i]), const std::runtime_error &);
    for (auto i = 0u; i < spectraStrings.size(); ++i)
      TS_ASSERT_THROWS(data->setSpectra(spectraStrings[i]), const std::runtime_error &);
  }

  void test_no_throw_when_setSpectra_is_provided_a_valid_spectra() {
    auto data = getFitData(10);

    std::vector<FunctionModelSpectra> const spectraPairs{FunctionModelSpectra(WorkspaceIndex{0}, WorkspaceIndex{9}),
                                                         FunctionModelSpectra(WorkspaceIndex{4}, WorkspaceIndex{4}),
                                                         FunctionModelSpectra(WorkspaceIndex{7}, WorkspaceIndex{4})};
    std::vector<std::string> const spectraStrings{"0", "9", "0,9,6,4,5", "1,2,3,4,5,6"};

    for (auto i = 0u; i < spectraPairs.size(); ++i)
      TS_ASSERT_THROWS_NOTHING(data->setSpectra(spectraPairs[i]));
    for (auto i = 0u; i < spectraStrings.size(); ++i)
      TS_ASSERT_THROWS_NOTHING(data->setSpectra(spectraStrings[i]));
  }

  void test_no_throw_when_setStartX_is_provided_a_valid_xValue_and_spectrum_number() {
    auto data = getFitData(10);

    TS_ASSERT_THROWS_NOTHING(data->setStartX(0.0, WorkspaceIndex{0}));
    TS_ASSERT_THROWS_NOTHING(data->setStartX(-5.0, WorkspaceIndex{0}));
    TS_ASSERT_THROWS_NOTHING(data->setStartX(5000000, WorkspaceIndex{10}));
  }

  void test_the_provided_startX_is_stored_in_range_after_using_setStartX() {
    auto data = getFitData(3);

    data->setStartX(-5.0, WorkspaceIndex{0});
    data->setStartX(6.53, WorkspaceIndex{1});
    data->setStartX(10.0, WorkspaceIndex{2});

    TS_ASSERT_EQUALS(data->getRange(WorkspaceIndex{0}).first, -5.0);
    TS_ASSERT_EQUALS(data->getRange(WorkspaceIndex{1}).first, 6.53);
    TS_ASSERT_EQUALS(data->getRange(WorkspaceIndex{2}).first, 10.0);
  }

  void test_no_throw_when_setEndX_is_provided_a_valid_xValue_and_spectrum_number() {
    auto data = getFitData(10);

    TS_ASSERT_THROWS_NOTHING(data->setStartX(0.0, WorkspaceIndex{0}));
    TS_ASSERT_THROWS_NOTHING(data->setStartX(-5.0, WorkspaceIndex{0}));
    TS_ASSERT_THROWS_NOTHING(data->setStartX(5000000, WorkspaceIndex{10}));
  }

  void test_the_provided_endX_is_stored_in_range_after_using_setEndX() {
    auto data = getFitData(3);

    data->setEndX(0.0, WorkspaceIndex{0});
    data->setEndX(6.53, WorkspaceIndex{1});
    data->setEndX(100000000000000.0, WorkspaceIndex{2});

    TS_ASSERT_EQUALS(data->getRange(WorkspaceIndex{0}).second, 0.0);
    TS_ASSERT_EQUALS(data->getRange(WorkspaceIndex{1}).second, 6.53);
    TS_ASSERT_EQUALS(data->getRange(WorkspaceIndex{2}).second, 100000000000000.0);
  }

  void test_that_endX_can_not_set_to_value_less_than_startX() {
    auto data = getFitData(1);
    auto startX = data->getRange(WorkspaceIndex{0}).first;
    data->setEndX(startX - 1.0, WorkspaceIndex{0});

    TS_ASSERT_EQUALS(data->getRange(WorkspaceIndex{0}).second, startX);
  }

  void test_that_startX_can_not_set_to_value_greater_than_startX() {
    auto data = getFitData(1);
    auto endX = data->getRange(WorkspaceIndex{0}).second;
    data->setStartX(endX + 1.0, WorkspaceIndex{0});

    TS_ASSERT_EQUALS(data->getRange(WorkspaceIndex{0}).first, endX);
  }

  void test_that_the_startX_of_two_data_sets_are_combined_when_relating_to_two_seperate_spectra() {
    auto data1 = getFitData(2);
    auto data2 = getFitData(2);

    data1->setStartX(6.53, WorkspaceIndex{0});
    data2->setStartX(5.0, WorkspaceIndex{1});
    auto const combinedData = data2->combine(*data1);

    TS_ASSERT_EQUALS(combinedData.getRange(WorkspaceIndex{0}).first, 6.53);
    TS_ASSERT_EQUALS(combinedData.getRange(WorkspaceIndex{1}).first, 5.0);
  }

  void test_that_the_endX_of_two_datasets_are_combined_when_relating_to_two_seperate_spectra() {
    auto data1 = getFitData(2);
    auto data2 = getFitData(2);

    data1->setEndX(2.34, WorkspaceIndex{0});
    data2->setEndX(5.9, WorkspaceIndex{1});
    auto const combinedData = data2->combine(*data1);

    TS_ASSERT_EQUALS(combinedData.getRange(WorkspaceIndex{0}).second, 2.34);
    TS_ASSERT_EQUALS(combinedData.getRange(WorkspaceIndex{1}).second, 5.9);
  }

  void test_that_the_excludeRegion_of_two_datasets_are_combined_when_relating_to_two_seperate_spectra() {
    auto data1 = getFitData(2);
    auto data2 = getFitData(2);

    data1->setExcludeRegionString("1,2,6,5", WorkspaceIndex{0});
    data1->setExcludeRegionString("6,2", WorkspaceIndex{1});
    auto const combinedData = data2->combine(*data1);

    TS_ASSERT_EQUALS(combinedData.getExcludeRegion(WorkspaceIndex{0}), std::string("1.000,2.000,5.000,6.000"));
    TS_ASSERT_EQUALS(combinedData.getExcludeRegion(WorkspaceIndex{1}), std::string("2.000,6.000"));
  }

  void test_that_the_spectra_pair_of_two_datasets_are_combined_correctly_when_spectra_do_not_overlap() {
    auto data1 = getFitData(10);
    auto data2 = getFitData(10);

    data1->setSpectra(FunctionModelSpectra(WorkspaceIndex{0}, WorkspaceIndex{4}));
    data2->setSpectra(FunctionModelSpectra(WorkspaceIndex{5}, WorkspaceIndex{9}));
    auto const combinedData = data2->combine(*data1);
    FunctionModelSpectra const spec(FunctionModelSpectra(WorkspaceIndex{0}, WorkspaceIndex{9}));

    TS_ASSERT_EQUALS(combinedData.spectra(), spec);
  }

  void test_that_the_spectra_pair_of_two_datasets_are_combined_correctly_when_spectra_are_discontinuous() {
    auto data1 = getFitData(10);
    auto data2 = getFitData(10);

    data1->setSpectra(FunctionModelSpectra(WorkspaceIndex{0}, WorkspaceIndex{4}));
    data2->setSpectra(FunctionModelSpectra(WorkspaceIndex{8}, WorkspaceIndex{9}));
    auto const combinedData = data2->combine(*data1);
    FunctionModelSpectra const spec(FunctionModelSpectra("0-4,8-9"));

    TS_ASSERT_EQUALS(combinedData.spectra(), spec);
  }

  void test_that_the_spectra_pair_of_two_datasets_are_combined_correctly_when_spectra_overlap() {
    auto data1 = getFitData(10);
    auto data2 = getFitData(10);

    data1->setSpectra(FunctionModelSpectra(WorkspaceIndex{0}, WorkspaceIndex{8}));
    data2->setSpectra(FunctionModelSpectra(WorkspaceIndex{4}, WorkspaceIndex{9}));
    auto const combinedData = data2->combine(*data1);
    FunctionModelSpectra const spec(FunctionModelSpectra("0-9"));

    TS_ASSERT_EQUALS(combinedData.spectra(), spec);
  }

  void test_that_the_DiscontinuousSpectra_of_two_datasets_are_combined_correctly_when_spectra_do_not_overlap() {
    auto data1 = getFitData(10);
    auto data2 = getFitData(10);

    data1->setSpectra(FunctionModelSpectra("0-4"));
    data2->setSpectra(FunctionModelSpectra("5-9"));
    auto const combinedData = data2->combine(*data1);
    FunctionModelSpectra const spec(FunctionModelSpectra("0-9"));

    TS_ASSERT_EQUALS(combinedData.spectra(), spec);
  }

  void test_that_the_DiscontinuousSpectra_of_two_datasets_are_combined_correctly_when_spectra_overlap() {
    auto data1 = getFitData(10);
    auto data2 = getFitData(10);

    data1->setSpectra(FunctionModelSpectra("0-7"));
    data2->setSpectra(FunctionModelSpectra("2-9"));
    auto const combinedData = data2->combine(*data1);
    FunctionModelSpectra const spec(FunctionModelSpectra("0-9"));

    TS_ASSERT_EQUALS(combinedData.spectra(), spec);
  }

  void test_that_a_Spectra_pair_and_DiscontinuousSpectra_dataset_are_combined_correctly_when_spectra_do_not_overlap() {
    auto data1 = getFitData(10);
    auto data2 = getFitData(10);

    data1->setSpectra(FunctionModelSpectra("0-4"));
    data2->setSpectra(FunctionModelSpectra(WorkspaceIndex{5}, WorkspaceIndex{9}));
    auto const combinedData = data2->combine(*data1);
    FunctionModelSpectra const spec(FunctionModelSpectra("0-9"));

    TS_ASSERT_EQUALS(combinedData.spectra(), spec);
  }

  void test_that_a_Spectra_pair_and_DiscontinuousSpectra_dataset_are_combined_correctly_when_spectra_overlap() {
    auto data1 = getFitData(10);
    auto data2 = getFitData(10);

    data1->setSpectra(FunctionModelSpectra("0-7"));
    data2->setSpectra(FunctionModelSpectra(WorkspaceIndex{4}, WorkspaceIndex{9}));
    auto const combinedData = data2->combine(*data1);
    FunctionModelSpectra const spec(FunctionModelSpectra("0-9"));

    TS_ASSERT_EQUALS(combinedData.spectra(), spec);
  }
  void test_that_getQValues() {
    auto const workspace = createWorkspaceWithInelasticInstrument(10);
    FunctionModelSpectra const spec =
        FunctionModelSpectra(WorkspaceIndex{0}, WorkspaceIndex{workspace->getNumberHistograms() - 1});
    auto data = std::make_unique<FitData>(FitData(workspace, spec));
    auto spectrumInfo = workspace->spectrumInfo();
    auto detID = spectrumInfo.detector(0).getID();
    double efixed = workspace->getEFixed(detID);
    double usignTheta = 0.5 * spectrumInfo.twoTheta(0);
    double q = Mantid::Kernel::UnitConversion::convertToElasticQ(usignTheta, efixed);
    TS_ASSERT_EQUALS(data->getQValues()[0], q);
  }
};
