// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/BoxController.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidMDAlgorithms/ConvertHFIRSCDtoMDE.h"
#include "MantidMDAlgorithms/LoadMD.h"

#include <string>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::MDAlgorithms;

class ConvertHFIRSCDtoMDETest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertHFIRSCDtoMDETest *createSuite() { return new ConvertHFIRSCDtoMDETest(); }
  static void destroySuite(ConvertHFIRSCDtoMDETest *suite) { delete suite; }

  void test_Init() {
    ConvertHFIRSCDtoMDE alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_EQUALS(alg.getPropertyValue("MergeInputs"), "0");
  }

  void test_exec() {
    // Create test input if necessary
    LoadMD loader;
    loader.initialize();
    loader.setPropertyValue("Filename", Mantid::API::FileFinder::Instance().getFullPath("HB3A_data.nxs").string());
    loader.setPropertyValue("OutputWorkspace", "ConvertHFIRSCDtoMDETest_data");
    loader.setProperty("FileBackEnd", false);
    loader.execute();
    auto inputWS = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::IMDHistoWorkspace>(
        "ConvertHFIRSCDtoMDETest_data");

    auto setGoniometer = AlgorithmManager::Instance().create("SetGoniometer");
    setGoniometer->initialize();
    setGoniometer->setProperty("Workspace", inputWS);
    setGoniometer->setPropertyValue("Axis0", "omega,0,1,0,-1");
    setGoniometer->setPropertyValue("Axis1", "chi,0,0,1,-1");
    setGoniometer->setPropertyValue("Axis2", "phi,0,1,0,-1");
    setGoniometer->setProperty("Average", false);
    setGoniometer->execute();

    ConvertHFIRSCDtoMDE alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "ConvertHFIRSCDtoMDETest_data"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Wavelength", "1.008"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from the algorithm. The type here will probably
    // need to change. It should be the type using in declareProperty for the
    // "OutputWorkspace" type. We can't use auto as it's an implicit conversion.
    Workspace_sptr output = alg.getProperty("OutputWorkspace");
    auto outWS = std::dynamic_pointer_cast<IMDEventWorkspace>(output);
    TS_ASSERT(outWS);

    // check dimensions
    TS_ASSERT_EQUALS(3, outWS->getNumDims());
    TS_ASSERT_EQUALS(Mantid::Kernel::QSample, outWS->getSpecialCoordinateSystem());
    TS_ASSERT_EQUALS("QSample", outWS->getDimension(0)->getMDFrame().name());
    TS_ASSERT_EQUALS(true, outWS->getDimension(0)->getMDUnits().isQUnit());
    TS_ASSERT_EQUALS(-10, outWS->getDimension(0)->getMinimum());
    TS_ASSERT_EQUALS(10, outWS->getDimension(0)->getMaximum());
    TS_ASSERT_EQUALS("QSample", outWS->getDimension(1)->getMDFrame().name());
    TS_ASSERT_EQUALS(true, outWS->getDimension(1)->getMDUnits().isQUnit());
    TS_ASSERT_EQUALS(-10, outWS->getDimension(1)->getMinimum());
    TS_ASSERT_EQUALS(10, outWS->getDimension(1)->getMaximum());
    TS_ASSERT_EQUALS("QSample", outWS->getDimension(2)->getMDFrame().name());
    TS_ASSERT_EQUALS(true, outWS->getDimension(2)->getMDUnits().isQUnit());
    TS_ASSERT_EQUALS(-10, outWS->getDimension(2)->getMinimum());
    TS_ASSERT_EQUALS(10, outWS->getDimension(2)->getMaximum());

    // check other things
    const Mantid::coord_t coords[3] = {-0.42f, 1.71f, 2.3f}; // roughly the location of maximum instenity
    TS_ASSERT_EQUALS(1, outWS->getNumExperimentInfo());
    TS_ASSERT_EQUALS(9038, outWS->getNEvents());
    TS_ASSERT_DELTA(outWS->getSignalAtCoord(coords, Mantid::API::NoNormalization), 568, 1e-5);

    // check coeff behavior
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "ConvertHFIRSCDtoMDETest_data"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Wavelength", "1.008"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ObliquityParallaxCoefficient", "1.5"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    output = alg.getProperty("OutputWorkspace");
    outWS = std::dynamic_pointer_cast<IMDEventWorkspace>(output);
    TS_ASSERT(outWS);

    TS_ASSERT_EQUALS(1, outWS->getNumExperimentInfo());
    TS_ASSERT_EQUALS(9038, outWS->getNEvents());
    TS_ASSERT_DELTA(outWS->getSignalAtCoord(coords, Mantid::API::NoNormalization), 453, 1e-5);
  }

  void test_wavelength_from_sample_log_without_property() {
    // HB3A_data.nxs stores the wavelength log as the string "1.008000"
    auto outWS = convert(loadData(), "");
    assertConvertedWithWavelength(outWS, 1.008);
  }

  void test_sample_log_takes_precedence_over_property() {
    auto outWS = convert(loadData(), "2.0");
    assertConvertedWithWavelength(outWS, 1.008);
  }

  void test_numeric_sample_log() {
    auto inputWS = loadData();
    inputWS->getExperimentInfo(0)->mutableRun().addProperty("wavelength", 1.008, true);
    auto outWS = convert(inputWS, "2.0");
    assertConvertedWithWavelength(outWS, 1.008);
  }

  void test_property_used_when_sample_log_missing() {
    auto inputWS = loadData();
    inputWS->getExperimentInfo(0)->mutableRun().removeProperty("wavelength");
    auto outWS = convert(inputWS, "1.008");
    assertConvertedWithWavelength(outWS, 1.008);
  }

  void test_missing_wavelength() {
    auto inputWS = loadData();
    inputWS->getExperimentInfo(0)->mutableRun().removeProperty("wavelength");
    assertWavelengthError(inputWS, "", "no 'wavelength' sample log");
  }

  void test_non_numeric_sample_log() {
    auto inputWS = loadData();
    inputWS->getExperimentInfo(0)->mutableRun().addProperty("wavelength", std::string("unknown"), true);
    assertWavelengthError(inputWS, "1.008", "cannot be converted to a number");
  }

  void test_several_wavelengths_for_single_input() { assertWavelengthError(loadData(), "1.008,1.5", "Only one value"); }

  void test_non_positive_wavelength_property() {
    ConvertHFIRSCDtoMDE alg;
    alg.initialize();
    TS_ASSERT_THROWS(alg.setProperty("Wavelength", "0.0"), const std::invalid_argument &);
    TS_ASSERT_THROWS(alg.setProperty("Wavelength", "-1.0"), const std::invalid_argument &);
  }

  void test_non_finite_wavelength_property() {
    auto inputWS = loadData();
    inputWS->getExperimentInfo(0)->mutableRun().removeProperty("wavelength");
    assertWavelengthError(inputWS, "nan", "Wavelength property must be a positive number");
  }

  void test_group_output_names_and_order() {
    // Member order differs from alphabetical order, to check that the output follows the group
    addGroup("ConvertHFIRSCDtoMDETest_group", {"ConvertHFIRSCDtoMDETest_b", "ConvertHFIRSCDtoMDETest_a"},
             {loadData(), loadData()});
    auto alg = createAlgorithm("ConvertHFIRSCDtoMDETest_group", "", "ConvertHFIRSCDtoMDETest_Q");
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    auto &ads = AnalysisDataService::Instance();
    TS_ASSERT(!alg->existsProperty("OutputWorkspace_1"));
    TS_ASSERT(!alg->existsProperty("OutputWorkspace_2"));
    auto outGroup = ads.retrieveWS<WorkspaceGroup>("ConvertHFIRSCDtoMDETest_Q");
    TS_ASSERT_EQUALS(outGroup->size(), 2);
    const std::vector<std::string> expected = {"ConvertHFIRSCDtoMDETest_Q_ConvertHFIRSCDtoMDETest_b",
                                               "ConvertHFIRSCDtoMDETest_Q_ConvertHFIRSCDtoMDETest_a"};
    TS_ASSERT_EQUALS(outGroup->getNames(), expected);
    for (const auto &name : expected)
      assertConvertedWithWavelength(ads.retrieveWS<IMDEventWorkspace>(name), 1.008);
    ads.clear();
  }

  void test_group_as_child_algorithm() {
    auto group = std::make_shared<WorkspaceGroup>();
    group->addWorkspace(loadData());
    group->addWorkspace(loadData());
    ConvertHFIRSCDtoMDE alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", std::static_pointer_cast<Workspace>(group));
    alg.setPropertyValue("OutputWorkspace", "_unused_for_child");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    Workspace_sptr output = alg.getProperty("OutputWorkspace");
    auto outGroup = std::dynamic_pointer_cast<WorkspaceGroup>(output);
    TS_ASSERT(outGroup);
    if (!outGroup)
      return;
    TS_ASSERT_EQUALS(outGroup->size(), 2);
    for (size_t i = 0; i < outGroup->size(); ++i)
      assertConvertedWithWavelength(std::dynamic_pointer_cast<IMDEventWorkspace>(outGroup->getItem(i)), 1.008);
    // A child algorithm does not store the members of its output group
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("_unused_for_child_1"));
  }

  void test_group_per_member_wavelength() {
    // The first member keeps its sample log, which takes precedence over its fallback value
    auto withoutLog = loadData();
    withoutLog->getExperimentInfo(0)->mutableRun().removeProperty("wavelength");
    addGroup("ConvertHFIRSCDtoMDETest_group", {"ConvertHFIRSCDtoMDETest_a", "ConvertHFIRSCDtoMDETest_b"},
             {loadData(), withoutLog});
    auto alg = createAlgorithm("ConvertHFIRSCDtoMDETest_group", "1.5,2.0", "ConvertHFIRSCDtoMDETest_Q");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT_DELTA(outputWavelength("ConvertHFIRSCDtoMDETest_Q_ConvertHFIRSCDtoMDETest_a"), 1.008, 1e-9);
    TS_ASSERT_DELTA(outputWavelength("ConvertHFIRSCDtoMDETest_Q_ConvertHFIRSCDtoMDETest_b"), 2.0, 1e-9);
    AnalysisDataService::Instance().clear();
  }

  void test_group_single_wavelength_for_all_members() {
    std::vector<IMDHistoWorkspace_sptr> members = {loadData(), loadData()};
    for (const auto &member : members)
      member->getExperimentInfo(0)->mutableRun().removeProperty("wavelength");
    addGroup("ConvertHFIRSCDtoMDETest_group", {"ConvertHFIRSCDtoMDETest_a", "ConvertHFIRSCDtoMDETest_b"}, members);
    auto alg = createAlgorithm("ConvertHFIRSCDtoMDETest_group", "1.008", "ConvertHFIRSCDtoMDETest_Q");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    for (const std::string name :
         {"ConvertHFIRSCDtoMDETest_Q_ConvertHFIRSCDtoMDETest_a", "ConvertHFIRSCDtoMDETest_Q_ConvertHFIRSCDtoMDETest_b"})
      assertConvertedWithWavelength(AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(name), 1.008);
    AnalysisDataService::Instance().clear();
  }

  void test_group_wavelength_count_mismatch() {
    addGroup("ConvertHFIRSCDtoMDETest_group", {"ConvertHFIRSCDtoMDETest_a", "ConvertHFIRSCDtoMDETest_b"},
             {loadData(), loadData()});
    auto alg = createAlgorithm("ConvertHFIRSCDtoMDETest_group", "1.0,1.5,2.0", "ConvertHFIRSCDtoMDETest_Q");
    assertValidationError(*alg, "Wavelength", "found 3 values for 2 members");
    AnalysisDataService::Instance().clear();
  }

  void test_group_member_missing_wavelength() {
    auto withoutLog = loadData();
    withoutLog->getExperimentInfo(0)->mutableRun().removeProperty("wavelength");
    addGroup("ConvertHFIRSCDtoMDETest_group", {"ConvertHFIRSCDtoMDETest_a", "ConvertHFIRSCDtoMDETest_b"},
             {loadData(), withoutLog});
    auto alg = createAlgorithm("ConvertHFIRSCDtoMDETest_group", "", "ConvertHFIRSCDtoMDETest_Q");
    assertValidationError(*alg, "Wavelength", "Member 'ConvertHFIRSCDtoMDETest_b': No wavelength available");
    AnalysisDataService::Instance().clear();
  }

  void test_group_member_not_an_MDHistoWorkspace() {
    auto &ads = AnalysisDataService::Instance();
    ads.addOrReplace("ConvertHFIRSCDtoMDETest_a", loadData());
    ads.addOrReplace("ConvertHFIRSCDtoMDETest_b", convert(loadData(), ""));
    auto group = std::make_shared<WorkspaceGroup>();
    group->addWorkspace(ads.retrieve("ConvertHFIRSCDtoMDETest_a"));
    group->addWorkspace(ads.retrieve("ConvertHFIRSCDtoMDETest_b"));
    ads.addOrReplace("ConvertHFIRSCDtoMDETest_group", group);
    auto alg = createAlgorithm("ConvertHFIRSCDtoMDETest_group", "", "ConvertHFIRSCDtoMDETest_Q");
    assertValidationError(*alg, "InputWorkspace", "Member 'ConvertHFIRSCDtoMDETest_b': must be an MDHistoWorkspace");
    ads.clear();
  }

  void test_group_members_from_different_instruments() {
    // Give the second member a WAND instrument with the logs that WAND requires
    auto wand = loadData();
    const auto nScans = wand->getDimension(2)->getNBins();
    auto expInfo = wand->getExperimentInfo(0);
    expInfo->setInstrument(std::make_shared<Mantid::Geometry::Instrument>("WAND"));
    expInfo->mutableRun().addProperty("duration", std::vector<double>(nScans, 1.0), true);
    expInfo->mutableRun().addProperty("monitor_count", std::vector<double>(nScans, 1.0), true);
    addGroup("ConvertHFIRSCDtoMDETest_group", {"ConvertHFIRSCDtoMDETest_a", "ConvertHFIRSCDtoMDETest_b"},
             {loadData(), wand});
    auto alg = createAlgorithm("ConvertHFIRSCDtoMDETest_group", "", "ConvertHFIRSCDtoMDETest_Q");
    assertValidationError(*alg, "InputWorkspace", "instrument WAND differs from instrument HB3A");
    AnalysisDataService::Instance().clear();
  }

  void test_empty_group() {
    AnalysisDataService::Instance().addOrReplace("ConvertHFIRSCDtoMDETest_group", std::make_shared<WorkspaceGroup>());
    auto alg = createAlgorithm("ConvertHFIRSCDtoMDETest_group", "", "ConvertHFIRSCDtoMDETest_Q");
    assertValidationError(*alg, "InputWorkspace", "The input group is empty");
    AnalysisDataService::Instance().clear();
  }

  void test_merge_inputs() {
    addGroup("ConvertHFIRSCDtoMDETest_group", {"ConvertHFIRSCDtoMDETest_a", "ConvertHFIRSCDtoMDETest_b"},
             {loadData(), loadData()});
    const auto hiddenWorkspacesBefore = hiddenWorkspaceNames();
    auto alg = createAlgorithm("ConvertHFIRSCDtoMDETest_group", "", "ConvertHFIRSCDtoMDETest_Q");
    alg->setProperty("MergeInputs", true);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    auto &ads = AnalysisDataService::Instance();
    auto outWS = ads.retrieveWS<IMDEventWorkspace>("ConvertHFIRSCDtoMDETest_Q");
    TS_ASSERT(outWS);
    if (!outWS)
      return;
    TS_ASSERT_EQUALS(outWS->getNEvents(), 2 * 9038);
    TS_ASSERT_EQUALS(outWS->getNumExperimentInfo(), 2);
    // The box controller defaults of this algorithm, not those of MergeMD
    const auto boxController = outWS->getBoxController();
    TS_ASSERT_EQUALS(boxController->getSplitInto(0), 5);
    TS_ASSERT_EQUALS(boxController->getSplitThreshold(), 1000);
    TS_ASSERT_EQUALS(boxController->getMaxDepth(), 20);
    // Neither per-member outputs nor the temporary workspaces given to MergeMD remain
    TS_ASSERT(!ads.doesExist("ConvertHFIRSCDtoMDETest_Q_ConvertHFIRSCDtoMDETest_a"));
    TS_ASSERT(!ads.doesExist("ConvertHFIRSCDtoMDETest_Q_ConvertHFIRSCDtoMDETest_b"));
    TS_ASSERT_EQUALS(hiddenWorkspaceNames(), hiddenWorkspacesBefore);
    ads.clear();
  }

  void test_merge_inputs_as_child_algorithm() {
    auto group = std::make_shared<WorkspaceGroup>();
    group->addWorkspace(loadData());
    group->addWorkspace(loadData());
    ConvertHFIRSCDtoMDE alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", std::static_pointer_cast<Workspace>(group));
    alg.setProperty("MergeInputs", true);
    alg.setPropertyValue("SplitInto", "3");
    alg.setProperty("SplitThreshold", 200);
    alg.setProperty("MaxRecursionDepth", 10);
    alg.setPropertyValue("OutputWorkspace", "_unused_for_child");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    Workspace_sptr output = alg.getProperty("OutputWorkspace");
    auto outWS = std::dynamic_pointer_cast<IMDEventWorkspace>(output);
    TS_ASSERT(outWS);
    if (outWS) {
      TS_ASSERT_EQUALS(outWS->getNEvents(), 2 * 9038);
      const auto boxController = outWS->getBoxController();
      TS_ASSERT_EQUALS(boxController->getSplitInto(0), 3);
      TS_ASSERT_EQUALS(boxController->getSplitThreshold(), 200);
      TS_ASSERT_EQUALS(boxController->getMaxDepth(), 10);
    }
    TS_ASSERT(hiddenWorkspaceNames().empty());
  }

  void test_merge_inputs_equals_convert_then_MergeMD() {
    // Different wavelengths for the two members, so that the merge combines different Q coordinates
    auto withoutLog = loadData();
    withoutLog->getExperimentInfo(0)->mutableRun().removeProperty("wavelength");
    addGroup("ConvertHFIRSCDtoMDETest_group", {"ConvertHFIRSCDtoMDETest_a", "ConvertHFIRSCDtoMDETest_b"},
             {loadData(), withoutLog});
    auto alg = createAlgorithm("ConvertHFIRSCDtoMDETest_group", "1.5,2.0", "ConvertHFIRSCDtoMDETest_Q");
    alg->setProperty("MergeInputs", true);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    auto &ads = AnalysisDataService::Instance();
    ads.addOrReplace("ConvertHFIRSCDtoMDETest_Q_a",
                     convert(ads.retrieveWS<IMDHistoWorkspace>("ConvertHFIRSCDtoMDETest_a"), ""));
    ads.addOrReplace("ConvertHFIRSCDtoMDETest_Q_b",
                     convert(ads.retrieveWS<IMDHistoWorkspace>("ConvertHFIRSCDtoMDETest_b"), "2.0"));
    auto merge = AlgorithmManager::Instance().createUnmanaged("MergeMD");
    merge->initialize();
    merge->setRethrows(true);
    merge->setPropertyValue("InputWorkspaces", "ConvertHFIRSCDtoMDETest_Q_a,ConvertHFIRSCDtoMDETest_Q_b");
    merge->setPropertyValue("OutputWorkspace", "ConvertHFIRSCDtoMDETest_reference");
    // The box controller defaults of ConvertHFIRSCDtoMDE, which it passes to MergeMD
    merge->setPropertyValue("SplitInto", "5");
    merge->setProperty("SplitThreshold", 1000);
    merge->setProperty("MaxRecursionDepth", 20);
    merge->execute();

    auto compare = AlgorithmManager::Instance().createUnmanaged("CompareMDWorkspaces");
    compare->initialize();
    compare->setRethrows(true);
    compare->setPropertyValue("Workspace1", "ConvertHFIRSCDtoMDETest_Q");
    compare->setPropertyValue("Workspace2", "ConvertHFIRSCDtoMDETest_reference");
    compare->setProperty("CheckEvents", true);
    compare->execute();
    const bool equals = compare->getProperty("Equals");
    TSM_ASSERT(compare->getPropertyValue("Result"), equals);
    ads.clear();
  }

  void test_merge_inputs_ignored_for_single_input() {
    AnalysisDataService::Instance().addOrReplace("ConvertHFIRSCDtoMDETest_a", loadData());
    auto alg = createAlgorithm("ConvertHFIRSCDtoMDETest_a", "", "ConvertHFIRSCDtoMDETest_Q");
    alg->setProperty("MergeInputs", true);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    assertConvertedWithWavelength(
        AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>("ConvertHFIRSCDtoMDETest_Q"), 1.008);
    AnalysisDataService::Instance().clear();
  }

private:
  IMDHistoWorkspace_sptr m_data;

  /// Names of hidden workspaces currently in the ADS
  std::vector<std::string> hiddenWorkspaceNames() {
    std::vector<std::string> names;
    for (const auto &name : AnalysisDataService::Instance().getObjectNames(Mantid::Kernel::DataServiceSort::Unsorted,
                                                                           Mantid::Kernel::DataServiceHidden::Include))
      if (name.starts_with("__"))
        names.emplace_back(name);
    return names;
  }

  /// Add workspaces to the AnalysisDataService under the given names, and group them
  void addGroup(const std::string &groupName, const std::vector<std::string> &names,
                const std::vector<IMDHistoWorkspace_sptr> &members) {
    auto &ads = AnalysisDataService::Instance();
    auto group = std::make_shared<WorkspaceGroup>();
    for (size_t i = 0; i < names.size(); ++i) {
      ads.addOrReplace(names[i], members[i]);
      group->addWorkspace(members[i]);
    }
    ads.addOrReplace(groupName, group);
  }

  /// Create a non-child algorithm working with names in the AnalysisDataService; an empty wavelength is left unset
  IAlgorithm_sptr createAlgorithm(const std::string &input, const std::string &wavelength, const std::string &output) {
    auto alg = AlgorithmManager::Instance().createUnmanaged("ConvertHFIRSCDtoMDE");
    alg->initialize();
    alg->setRethrows(true);
    alg->setPropertyValue("InputWorkspace", input);
    if (!wavelength.empty())
      alg->setPropertyValue("Wavelength", wavelength);
    alg->setPropertyValue("OutputWorkspace", output);
    return alg;
  }

  double outputWavelength(const std::string &name) {
    auto ws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(name);
    return ws->getExperimentInfo(0)->run().getPropertyValueAsType<double>("wavelength");
  }

  /// Check that validation reports an error for the property, containing the given text
  void assertValidationError(IAlgorithm &alg, const std::string &property, const std::string &expected) {
    const auto errors = alg.validateInputs();
    TS_ASSERT_EQUALS(errors.count(property), 1);
    if (errors.count(property) == 1)
      TS_ASSERT(errors.at(property).find(expected) != std::string::npos);
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  /// Load HB3A_data.nxs once, and return a copy that a test may modify
  IMDHistoWorkspace_sptr loadData() {
    if (!m_data) {
      LoadMD loader;
      loader.initialize();
      loader.setChild(true);
      loader.setPropertyValue("Filename", "HB3A_data.nxs");
      loader.setPropertyValue("OutputWorkspace", "_unused_for_child");
      loader.setProperty("FileBackEnd", false);
      loader.execute();
      Mantid::API::IMDWorkspace_sptr loaded = loader.getProperty("OutputWorkspace");
      m_data = std::dynamic_pointer_cast<IMDHistoWorkspace>(loaded);

      auto setGoniometer = AlgorithmManager::Instance().createUnmanaged("SetGoniometer");
      setGoniometer->initialize();
      setGoniometer->setChild(true);
      setGoniometer->setProperty("Workspace", m_data);
      setGoniometer->setPropertyValue("Axis0", "omega,0,1,0,-1");
      setGoniometer->setPropertyValue("Axis1", "chi,0,0,1,-1");
      setGoniometer->setPropertyValue("Axis2", "phi,0,1,0,-1");
      setGoniometer->setProperty("Average", false);
      setGoniometer->execute();
    }
    return m_data->clone();
  }

  /// Run the algorithm; an empty wavelength leaves the Wavelength property unset
  IMDEventWorkspace_sptr convert(const IMDHistoWorkspace_sptr &inputWS, const std::string &wavelength) {
    ConvertHFIRSCDtoMDE alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWS);
    if (!wavelength.empty())
      alg.setProperty("Wavelength", wavelength);
    alg.setPropertyValue("OutputWorkspace", "_unused_for_child");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    Workspace_sptr output = alg.getProperty("OutputWorkspace");
    return std::dynamic_pointer_cast<IMDEventWorkspace>(output);
  }

  /// Check the output against the reference values of test_exec, obtained with the given wavelength
  void assertConvertedWithWavelength(const IMDEventWorkspace_sptr &outWS, const double wavelength) {
    TS_ASSERT(outWS);
    if (!outWS)
      return;
    const auto &run = outWS->getExperimentInfo(0)->run();
    TS_ASSERT_DELTA(run.getPropertyValueAsType<double>("wavelength"), wavelength, 1e-9);
    const Mantid::coord_t coords[3] = {-0.42f, 1.71f, 2.3f};
    TS_ASSERT_EQUALS(9038, outWS->getNEvents());
    TS_ASSERT_DELTA(outWS->getSignalAtCoord(coords, Mantid::API::NoNormalization), 568, 1e-5);
  }

  /// Check that validation reports a Wavelength error containing the given text
  void assertWavelengthError(const IMDHistoWorkspace_sptr &inputWS, const std::string &wavelength,
                             const std::string &expected) {
    ConvertHFIRSCDtoMDE alg;
    alg.setChild(true);
    alg.initialize();
    alg.setProperty("InputWorkspace", inputWS);
    if (!wavelength.empty())
      alg.setProperty("Wavelength", wavelength);
    alg.setPropertyValue("OutputWorkspace", "_unused_for_child");
    const auto errors = alg.validateInputs();
    TS_ASSERT_EQUALS(errors.count("Wavelength"), 1);
    if (errors.count("Wavelength") == 1)
      TS_ASSERT(errors.at("Wavelength").find(expected) != std::string::npos);
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }
};
