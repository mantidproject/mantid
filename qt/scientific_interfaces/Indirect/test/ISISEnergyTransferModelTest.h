// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "Reduction/ISISEnergyTransferData.h"
#include "Reduction/ISISEnergyTransferModel.h"

#include <chrono>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <thread>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::CustomInterfaces;

class ISISIndirectEnergyTransfer : public Algorithm {
public:
  const std::string name() const override { return "ISISIndirectEnergyTransfer"; };
  int version() const override { return 1; };
  const std::string summary() const override { return "ISISIndirectEnergyTransfer Mock algorithm"; };

private:
  void init() override {
    declareProperty("Instrument", "");
    declareProperty("Analyser", "");
    declareProperty("Reflection", "");

    declareProperty("InputFiles", "");
    declareProperty("SumFiles", false);
    declareProperty("LoadLogFiles", false);
    declareProperty("CalibrationWorkspace", "");

    declareProperty("Efixed", 0.0);
    declareProperty("SpectraRange", std::vector<long>{0, 2});
    declareProperty("BackgroundRange", std::vector<double>{0.0, 0.0});
    declareProperty("RebinString", "");

    declareProperty("DetailedBalance", 0.0);

    declareProperty("UnitX", "DeltaE");
    declareProperty("FoldMultipleFrames", false);
    declareProperty("OutputWorkspace", "");

    declareProperty("GroupingMethod", "");
    declareProperty("GroupingString", "");
    declareProperty("MapFile", "");
  };

  void exec() override {
    ITableWorkspace_sptr outputWS = WorkspaceFactory::Instance().createTable();

    outputWS->addColumn("str", "Instrument");
    outputWS->addColumn("str", "Analyser");
    outputWS->addColumn("str", "Reflection");

    outputWS->addColumn("str", "InputFiles");
    outputWS->addColumn("bool", "SumFiles");
    outputWS->addColumn("bool", "LoadLogFiles");
    outputWS->addColumn("str", "CalibrationWorkspace");

    outputWS->addColumn("double", "Efixed");
    outputWS->addColumn("str", "SpectraRange");
    outputWS->addColumn("str", "BackgroundRange");
    outputWS->addColumn("str", "RebinString");

    outputWS->addColumn("double", "DetailedBalance");

    outputWS->addColumn("str", "UnitX");
    outputWS->addColumn("bool", "FoldMultipleFrames");
    outputWS->addColumn("str", "OutputWorkspace");

    outputWS->addColumn("str", "GroupingMethod");
    outputWS->addColumn("str", "GroupingString");
    outputWS->addColumn("str", "MapFile");

    TableRow newRow = outputWS->appendRow();

    auto instrument = getPropertyValue("Instrument");
    auto analyser = getPropertyValue("Analyser");
    auto reflection = getPropertyValue("Reflection");
    auto inputFiles = getPropertyValue("InputFiles");
    auto sumFiles = getPropertyValue("SumFiles") == "1";
    auto loadLogFiles = getPropertyValue("LoadLogFiles") == "1";
    auto calibrationWorkspace = getPropertyValue("CalibrationWorkspace");
    auto eFixed = std::stod(getProperty("Efixed"));
    auto spectraRange = getPropertyValue("SpectraRange");
    auto backgroundRange = getPropertyValue("BackgroundRange");
    auto rebinString = getPropertyValue("RebinString");
    auto detailedBalance = std::stod(getProperty("DetailedBalance"));
    auto unitX = getPropertyValue("UnitX");
    auto foldMultipleFrames = getPropertyValue("FoldMultipleFrames") == "1";
    auto outputWorkspace = getPropertyValue("OutputWorkspace");
    auto groupingMethod = getPropertyValue("GroupingMethod");
    auto groupingString = getPropertyValue("GroupingString");
    auto mapFile = getPropertyValue("MapFile");

    newRow << instrument << analyser << reflection << inputFiles << sumFiles << loadLogFiles << calibrationWorkspace
           << eFixed << spectraRange << backgroundRange << rebinString << detailedBalance << unitX << foldMultipleFrames
           << outputWorkspace << groupingMethod << groupingString << mapFile;

    Mantid::API::AnalysisDataService::Instance().addOrReplace("outputWS", outputWS);
  };
};

DECLARE_ALGORITHM(ISISIndirectEnergyTransfer)

class ISISEnergyTransferModelTest : public CxxTest::TestSuite {
public:
  ISISEnergyTransferModelTest() = default;
  void setUp() override { AnalysisDataService::Instance().clear(); }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testSetInstrumentProperties() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    InstrumentData instData("instrument", "analyser", "reflection");
    model->setInstrumentProperties(reductionAlgorithm, instData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("Instrument"), "instrument");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("Analyser"), "analyser");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("Reflection"), "reflection");
  }

  void testSetInputPropertiesWithAllEnabled() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETInputData inputData("input_workspace", "input_workspace", true, true, true, "calibration_workspace");
    model->setInputProperties(reductionAlgorithm, inputData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("InputFiles"), "input_workspace");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("SumFiles"), "1");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("LoadLogFiles"), "1");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("CalibrationWorkspace"), "calibration_workspace");
  }

  void testSetInputPropertiesWithAllDisabled() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETInputData inputData("input_workspace", "input_workspace", false, false, false, "");
    model->setInputProperties(reductionAlgorithm, inputData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("InputFiles"), "input_workspace");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("SumFiles"), "0");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("LoadLogFiles"), "0");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("CalibrationWorkspace"), "");
  }

  void testSetConversionPropertiesWithoutEfixed() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETConversionData conversionData(1.0, 1, 2);
    model->setConversionProperties(reductionAlgorithm, conversionData, "instrument");

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("Efixed"), "0");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("SpectraRange"), "1,2");
  }

  void testSetConversionPropertiesWithEfixed() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETConversionData conversionData(1.0, 1, 2);
    model->setConversionProperties(reductionAlgorithm, conversionData, "IRIS");

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("Efixed"), "1");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("SpectraRange"), "1,2");
  }

  void testSetBackgroundPropertiesWithBackgroundEnabled() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETBackgroundData backgroundData(true, 1.0, 2.0);
    model->setBackgroundProperties(reductionAlgorithm, backgroundData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("BackgroundRange"), "1,2");
  }

  void testSetBackgroundPropertiesWithBackgroundDisabled() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETBackgroundData backgroundData(false, 1.0, 2.0);
    model->setBackgroundProperties(reductionAlgorithm, backgroundData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("BackgroundRange"), "0,0");
  }

  void testSetRebinPropertiesWithMultipleRebin() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETRebinData rebinData(true, "Multiple", 1.0, 2.0, 3.0, "1,2,10");
    model->setRebinProperties(reductionAlgorithm, rebinData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("RebinString"), "1,2,10");
  }

  void testSetRebinPropertiesWithMultipleLogRebin() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETRebinData rebinData(true, "Multiple", 1.0, 2.0, 3.0, "2,-0.035,10");
    model->setRebinProperties(reductionAlgorithm, rebinData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("RebinString"), "2,-0.035,10");
  }

  void testSetRebinPropertiesWithMultipleVariableRangeRebin() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETRebinData rebinData(true, "Multiple", 1.0, 2.0, 3.0, "0,2,10,4,20");
    model->setRebinProperties(reductionAlgorithm, rebinData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("RebinString"), "0,2,10,4,20");
  }

  void testSetRebinPropertiesWithSingleRebin() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETRebinData rebinData(true, "Single", 0.0, 2.0, 6.0, "");
    model->setRebinProperties(reductionAlgorithm, rebinData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("RebinString"), "0.000000,6.000000,2.000000");
  }

  void testSetRebinPropertiesWithNoRebin() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETRebinData rebinData(false, "Single", 0.0, 0.0, 0.0, "1.0, 3.0, 5.0");
    model->setRebinProperties(reductionAlgorithm, rebinData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("RebinString"), "");
  }

  void testSetAnalysisPropertiesWithPropsEnabled() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETAnalysisData analysisData(true, 2.5);
    model->setAnalysisProperties(reductionAlgorithm, analysisData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("DetailedBalance"), "2.5");
  }

  void testSetAnalysisPropertiesWithPropsDisabled() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETAnalysisData analysisData(false, 2.5);
    model->setAnalysisProperties(reductionAlgorithm, analysisData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("DetailedBalance"), "0");
  }

  void testSetOutputPropertiesWithPropsEnabled() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETOutputData outputData(true, true);
    model->setOutputProperties(reductionAlgorithm, outputData, "output");

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("UnitX"), "DeltaE_inWavenumber");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("FoldMultipleFrames"), "1");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("OutputWorkspace"), "output");
  }

  void testSetOutputPropertiesWithPropsDisabled() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETOutputData outputData(false, false);
    model->setOutputProperties(reductionAlgorithm, outputData, "output");

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("UnitX"), "DeltaE");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("FoldMultipleFrames"), "0");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("OutputWorkspace"), "output");
  }

  void testSetGroupingPropertiesWithFileGrouping() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETGroupingData groupingData(IETGroupingType::FILE, 2, "map_file", "1,2,3");
    IETConversionData conversionData(1.0, 1, 5);
    model->setGroupingProperties(reductionAlgorithm, groupingData, conversionData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("GroupingString"), "");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("MapFile"), "map_file");
  }

  void testSetGroupingPropertiesWithCustomGrouping() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETGroupingData groupingData(IETGroupingType::CUSTOM, 2, "map_file", "1,2,3");
    IETConversionData conversionData(1.0, 1, 5);
    model->setGroupingProperties(reductionAlgorithm, groupingData, conversionData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("GroupingString"), "1,2,3");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("MapFile"), "");
  }

  void testSetGroupingPropertiesWithDefaultGrouping() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETGroupingData groupingData(IETGroupingType::DEFAULT, 2, "map_file", "1,2,3");
    IETConversionData conversionData(1.0, 1, 5);
    model->setGroupingProperties(reductionAlgorithm, groupingData, conversionData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("GroupingString"), "");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("MapFile"), "");
  }

  void testSetGroupingPropertiesWithGroupsGrouping() {
    auto model = makeModel();
    auto reductionAlgorithm = makeReductionAlgorithm();

    IETGroupingData groupingData(IETGroupingType::GROUPS, 2, "map_file", "1,2,3");
    IETConversionData conversionData(1.0, 1, 5);
    model->setGroupingProperties(reductionAlgorithm, groupingData, conversionData);

    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("GroupingString"), "1-2,3-4,5-5");
    TS_ASSERT_EQUALS(reductionAlgorithm->getPropertyValue("MapFile"), "");
  }

  void testGetOutputGroupName() {
    auto model = makeModel();

    InstrumentData instData("instrument", "analyser", "reflection");
    std::string inputFiles("1234, 1235");
    std::string outputName = model->getOuputGroupName(instData, inputFiles);

    TS_ASSERT_EQUALS(outputName, "instrument1234, 1235_analyser_reflection_Reduced");
  }

  void testRunIETAlgorithm() {
    MantidQt::API::BatchAlgorithmRunner *batch = new MantidQt::API::BatchAlgorithmRunner(nullptr);

    IETInputData inputData("input_workspace1, input_workspace2", "input_workspace1, input_workspace2", true, false,
                           true, "calibration_workspace");
    IETConversionData conversionData(1.0, 1, 2);
    IETGroupingData groupingData(IETGroupingType::DEFAULT, 2, "map_file");
    IETBackgroundData backgroundData(true, 0, 1);
    IETAnalysisData analysisData(true, 2.5);
    IETRebinData rebinData(true, "Multiple", 0.0, 0.0, 0.0, "1,2");
    IETOutputData outputData(false, false);

    IETRunData runData(inputData, conversionData, groupingData, backgroundData, analysisData, rebinData, outputData);

    InstrumentData instData("instrument", "analyser", "reflection");

    m_model->runIETAlgorithm(batch, instData, runData);

    // Wait for the algorithm to finish
    std::this_thread::sleep_for(std::chrono::seconds(1));

    TS_ASSERT_EQUALS(AnalysisDataService::Instance().doesExist("outputWS"), true);
    if (AnalysisDataService::Instance().doesExist("outputWS")) {
      ITableWorkspace_sptr outputWS =
          Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::TableWorkspace>("outputWS");

      TS_ASSERT_EQUALS(outputWS->rowCount(), 1);
      TS_ASSERT_EQUALS(outputWS->columnCount(), 18);

      TS_ASSERT_EQUALS(outputWS->getColumn(0)->name(), "Instrument");
      TS_ASSERT_EQUALS(outputWS->getColumn(1)->name(), "Analyser");
      TS_ASSERT_EQUALS(outputWS->getColumn(2)->name(), "Reflection");

      TS_ASSERT_EQUALS(outputWS->getColumn(3)->name(), "InputFiles");
      TS_ASSERT_EQUALS(outputWS->getColumn(4)->name(), "SumFiles");
      TS_ASSERT_EQUALS(outputWS->getColumn(5)->name(), "LoadLogFiles");
      TS_ASSERT_EQUALS(outputWS->getColumn(6)->name(), "CalibrationWorkspace");

      TS_ASSERT_EQUALS(outputWS->getColumn(7)->name(), "Efixed");
      TS_ASSERT_EQUALS(outputWS->getColumn(8)->name(), "SpectraRange");
      TS_ASSERT_EQUALS(outputWS->getColumn(9)->name(), "BackgroundRange");
      TS_ASSERT_EQUALS(outputWS->getColumn(10)->name(), "RebinString");

      TS_ASSERT_EQUALS(outputWS->getColumn(11)->name(), "DetailedBalance");

      TS_ASSERT_EQUALS(outputWS->getColumn(12)->name(), "UnitX");
      TS_ASSERT_EQUALS(outputWS->getColumn(13)->name(), "FoldMultipleFrames");
      TS_ASSERT_EQUALS(outputWS->getColumn(14)->name(), "OutputWorkspace");

      TS_ASSERT_EQUALS(outputWS->getColumn(15)->name(), "GroupingMethod");
      TS_ASSERT_EQUALS(outputWS->getColumn(16)->name(), "GroupingString");
      TS_ASSERT_EQUALS(outputWS->getColumn(17)->name(), "MapFile");
    }
  }

  void testCreateGroupingWithFileGrouping() {
    IETConversionData conversionData;
    IETGroupingData fileData(IETGroupingType::FILE, 2, "map_file");

    std::pair<std::string, std::string> fileGrouping = m_model->createGrouping(fileData, conversionData);
    TS_ASSERT_EQUALS(fileGrouping.first, IETGroupingType::FILE);
    TS_ASSERT_EQUALS(fileGrouping.second, "map_file");
  }

  void testCreateGroupingWithGroupsGrouping() {
    IETGroupingData groupsData(IETGroupingType::GROUPS, 2, "map_file");
    IETConversionData groupsConversion(1.0, 1, 5);

    std::pair<std::string, std::string> groupsGrouping = m_model->createGrouping(groupsData, groupsConversion);
    TS_ASSERT_EQUALS(groupsGrouping.first, IETGroupingType::CUSTOM);
    TS_ASSERT_EQUALS(groupsGrouping.second, "1-2,3-4,5-5");
  }

  void testCreateGroupingWithDefaultGrouping() {
    IETConversionData conversionData;
    IETGroupingData defaultData(IETGroupingType::DEFAULT, 2, "map_file");

    std::pair<std::string, std::string> defaultGrouping = m_model->createGrouping(defaultData, conversionData);
    TS_ASSERT_EQUALS(defaultGrouping.first, IETGroupingType::IPF);
    TS_ASSERT_EQUALS(defaultGrouping.second, "");
  }

  void testCreateGroupingWithCustomGrouping() {
    IETConversionData conversionData;
    IETGroupingData customData(IETGroupingType::CUSTOM, 2, "map_file", "1,2-4,5");

    std::pair<std::string, std::string> customGrouping = m_model->createGrouping(customData, conversionData);
    TS_ASSERT_EQUALS(customGrouping.first, IETGroupingType::CUSTOM);
    TS_ASSERT_EQUALS(customGrouping.second, "1,2-4,5");
  }

  void testCreateGroupingWithAllGrouping() {
    IETConversionData conversionData;
    IETGroupingData allData(IETGroupingType::ALL, 2, "map_file");

    std::pair<std::string, std::string> allGrouping = m_model->createGrouping(allData, conversionData);
    TS_ASSERT_EQUALS(allGrouping.first, IETGroupingType::ALL);
    TS_ASSERT_EQUALS(allGrouping.second, "");
  }

  void testCreateGroupingWithIndividualGrouping() {
    IETConversionData conversionData;
    IETGroupingData individualData(IETGroupingType::INDIVIDUAL, 2, "map_file");

    std::pair<std::string, std::string> individualGrouping = m_model->createGrouping(individualData, conversionData);
    TS_ASSERT_EQUALS(individualGrouping.first, IETGroupingType::INDIVIDUAL);
    TS_ASSERT_EQUALS(individualGrouping.second, "");
  }

  void testGetDetectorGroupingString() {
    std::string groupingString = m_model->getDetectorGroupingString(1, 10, 2);
    TS_ASSERT_EQUALS(groupingString, "1-5,6-10");
  }

  void testValidateRunDataGroupingInvalid() {
    IETInputData inputData("iris26184_multi_graphite002_red");
    IETConversionData conversionData(0.5, 1, 2);
    IETGroupingData groupingData(IETGroupingType::CUSTOM);
    IETBackgroundData backgroundData(false);
    IETAnalysisData analysisData;
    IETRebinData rebinData;
    IETOutputData outputData;

    IETRunData runData(inputData, conversionData, groupingData, backgroundData, analysisData, rebinData, outputData);

    std::vector<std::string> errors = m_model->validateRunData(runData, 1, 10);
    TS_ASSERT_EQUALS(errors.size(), 1);
    if (errors.size() == 1)
      TS_ASSERT_EQUALS(errors[0], "Please supply a custom grouping for detectors.");
  }

  void testValidateRunDataAnalysisInvalid() {
    IETInputData inputData("iris26184_multi_graphite002_red");
    IETConversionData conversionData(0.5, 1, 2);
    IETGroupingData groupingData(IETGroupingType::DEFAULT);
    IETBackgroundData backgroundData(false);
    IETAnalysisData analysisData(true, 0.0);
    IETRebinData rebinData;
    IETOutputData outputData;

    IETRunData runData(inputData, conversionData, groupingData, backgroundData, analysisData, rebinData, outputData);

    std::vector<std::string> errors = m_model->validateRunData(runData, 1, 10);
    TS_ASSERT_EQUALS(errors.size(), 1);
    if (errors.size() == 1)
      TS_ASSERT_EQUALS(errors[0], "Detailed Balance must be more than 0 K");
  }

  void testValidateRunDataSpectraInvalid() {
    IETInputData inputData("iris26184_multi_graphite002_red");
    IETConversionData conversionData(0.5, 4, 2);
    IETGroupingData groupingData(IETGroupingType::DEFAULT);
    IETBackgroundData backgroundData(false);
    IETAnalysisData analysisData;
    IETRebinData rebinData;
    IETOutputData outputData;

    IETRunData runData(inputData, conversionData, groupingData, backgroundData, analysisData, rebinData, outputData);

    std::vector<std::string> errors = m_model->validateRunData(runData, 1, 10);
    TS_ASSERT_EQUALS(errors.size(), 1);
    if (errors.size() == 1)
      TS_ASSERT_EQUALS(errors[0], "Minimum spectra must be less than maximum spectra.");
  }

  void testValidateRunDataBackgroundInvalid() {
    IETInputData inputData("iris26184_multi_graphite002_red");
    IETConversionData conversionData(0.5, 1, 2);
    IETGroupingData groupingData(IETGroupingType::DEFAULT);
    IETBackgroundData backgroundData(true, -1, 1);
    IETAnalysisData analysisData;
    IETRebinData rebinData;
    IETOutputData outputData;

    IETRunData runData(inputData, conversionData, groupingData, backgroundData, analysisData, rebinData, outputData);

    std::vector<std::string> errors = m_model->validateRunData(runData, 1, 10);
    TS_ASSERT_EQUALS(errors.size(), 2);
    if (errors.size() == 2) {
      TS_ASSERT_EQUALS(errors[0], "The Start of Background Removal is less than the minimum of the data range");
      TS_ASSERT_EQUALS(errors[1], "The End of Background Removal is more than the maximum of the data range");
    }
  }

  void testValidateRunDataAllValid() {
    IETInputData inputData("iris26184_multi_graphite002_red");
    IETConversionData conversionData(0.5, 1, 2);
    IETGroupingData groupingData(IETGroupingType::DEFAULT);
    IETBackgroundData backgroundData(false);
    IETAnalysisData analysisData;
    IETRebinData rebinData;
    IETOutputData outputData;

    IETRunData runData(inputData, conversionData, groupingData, backgroundData, analysisData, rebinData, outputData);

    std::vector<std::string> errors = m_model->validateRunData(runData, 1, 10);
    TS_ASSERT_EQUALS(errors.size(), 0);
  }

  void testValidatePlotDataInputInvalid() {
    IETInputData inputData;
    IETConversionData conversionData;
    IETBackgroundData backgroundData;

    IETPlotData plotData(inputData, conversionData, backgroundData);

    std::vector<std::string> errors = m_model->validatePlotData(plotData);
    TS_ASSERT_EQUALS(errors.size(), 1);
    if (errors.size() == 1)
      TS_ASSERT_EQUALS(errors[0], "You must select a run file.");
  }

  void testValidatePlotDataSpectraInvalid() {
    IETInputData inputData("iris26184_multi_graphite002_red");
    IETConversionData conversionData(0.5, 4, 2);
    IETBackgroundData backgroundData(false);

    IETPlotData plotData(inputData, conversionData, backgroundData);

    std::vector<std::string> errors = m_model->validatePlotData(plotData);
    TS_ASSERT_EQUALS(errors.size(), 1);
    if (errors.size() == 1)
      TS_ASSERT_EQUALS(errors[0], "Minimum spectra must be less than maximum spectra.");
  }

  void testValidatePlotDataBackgroundInvalid() {
    IETInputData inputData("iris26184_multi_graphite002_red");
    IETConversionData conversionData(0.5, 1, 2);
    IETBackgroundData backgroundData(true, -1, 1);

    IETPlotData plotData(inputData, conversionData, backgroundData);

    std::vector<std::string> errors = m_model->validatePlotData(plotData);
    TS_ASSERT_EQUALS(errors.size(), 2);
    if (errors.size() == 2) {
      TS_ASSERT_EQUALS(errors[0], "The Start of Background Removal is less than the minimum of the data range");
      TS_ASSERT_EQUALS(errors[1], "The End of Background Removal is more than the maximum of the data range");
    }
  }

  void testValidatePlotDataAllValid() {
    IETInputData inputData("iris26184_multi_graphite002_red");
    IETConversionData conversionData(0.5, 1, 2);
    IETBackgroundData backgroundData(false);

    IETPlotData plotData(inputData, conversionData, backgroundData);

    std::vector<std::string> errors = m_model->validatePlotData(plotData);
    TS_ASSERT_EQUALS(errors.size(), 0);
  }

  void testPlotRawAlgorithmQueueReturnsTwoAlgorithmsIfRemoveBackgroundIsFalse() {
    IETInputData inputData;
    IETConversionData conversionData;
    IETBackgroundData backgroundData(false);

    IETPlotData plotData(inputData, conversionData, backgroundData);
    InstrumentData instData("TFXA", "graphite", "004");

    auto const algorithmQueue = m_model->plotRawAlgorithmQueue(instData, plotData);
    TS_ASSERT_EQUALS(2, algorithmQueue.size());
  }

  void testPlotRawAlgorithmQueueReturnsFourAlgorithmsIfRemoveBackgroundIsTrue() {
    IETInputData inputData;
    IETConversionData conversionData;
    IETBackgroundData backgroundData(true, 1, 4);

    IETPlotData plotData(inputData, conversionData, backgroundData);
    InstrumentData instData("TFXA", "graphite", "004");

    auto const algorithmQueue = m_model->plotRawAlgorithmQueue(instData, plotData);
    TS_ASSERT_EQUALS(4, algorithmQueue.size());
  }

private:
  std::unique_ptr<IETModel> makeModel() { return std::make_unique<IETModel>(); }
  IAlgorithm_sptr makeReductionAlgorithm() { return AlgorithmManager::Instance().create("ISISIndirectEnergyTransfer"); }

  std::unique_ptr<IETModel> m_model;
  IAlgorithm_sptr m_reductionAlg;
};
