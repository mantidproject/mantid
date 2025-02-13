// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperties.h"
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

namespace {

std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> defaultGroupingProps() {
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  Mantid::API::AlgorithmProperties::update("GroupingMethod", std::string("IPF"), *properties);
  return properties;
}

} // namespace

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
    declareProperty("SpectraRange", std::vector<int>{0, 2});
    declareProperty("BackgroundRange", std::vector<double>{0.0, 0.0});
    declareProperty("RebinString", "");

    declareProperty("DetailedBalance", 0.0);

    declareProperty("UnitX", "DeltaE");
    declareProperty("FoldMultipleFrames", false);
    declareProperty("OutputWorkspace", "");

    declareProperty("GroupingMethod", "");
    declareProperty("GroupingString", "");
    declareProperty("GroupingFile", "");
    declareProperty("NGroups", 1);
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
    outputWS->addColumn("str", "GroupingFile");

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
    auto groupingFile = getPropertyValue("GroupingFile");

    newRow << instrument << analyser << reflection << inputFiles << sumFiles << loadLogFiles << calibrationWorkspace
           << eFixed << spectraRange << backgroundRange << rebinString << detailedBalance << unitX << foldMultipleFrames
           << outputWorkspace << groupingMethod << groupingString << groupingFile;

    Mantid::API::AnalysisDataService::Instance().addOrReplace("outputWS", outputWS);
  };
};

DECLARE_ALGORITHM(ISISIndirectEnergyTransfer)

class ISISEnergyTransferModelTest : public CxxTest::TestSuite {
public:
  static ISISEnergyTransferModelTest *createSuite() { return new ISISEnergyTransferModelTest(); }
  static void destroySuite(ISISEnergyTransferModelTest *suite) { delete suite; }

  void setUp() override {
    m_model = std::make_unique<IETModel>();
    AnalysisDataService::Instance().clear();
  }

  void tearDown() override {
    m_model.reset();
    AnalysisDataService::Instance().clear();
  }

  void testSetInstrumentProperties() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    InstrumentData instData("instrument", "analyser", "reflection");
    m_model->setInstrumentProperties(*properties, instData);

    TS_ASSERT_EQUALS(properties->getPropertyValue("Instrument"), "instrument");
    TS_ASSERT_EQUALS(properties->getPropertyValue("Analyser"), "analyser");
    TS_ASSERT_EQUALS(properties->getPropertyValue("Reflection"), "reflection");
  }

  void testSetInputPropertiesWithAllEnabled() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    IETInputData inputData("input_workspace", "input_workspace", true, true, true, "calibration_workspace");
    m_model->setInputProperties(*properties, inputData);

    TS_ASSERT_EQUALS(properties->getPropertyValue("InputFiles"), "input_workspace");
    TS_ASSERT_EQUALS(properties->getPropertyValue("SumFiles"), "1");
    TS_ASSERT_EQUALS(properties->getPropertyValue("LoadLogFiles"), "1");
    TS_ASSERT_EQUALS(properties->getPropertyValue("CalibrationWorkspace"), "calibration_workspace");
  }

  void testSetInputPropertiesWithAllDisabled() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    IETInputData inputData("input_workspace", "input_workspace", false, false, false, "");
    m_model->setInputProperties(*properties, inputData);

    TS_ASSERT_EQUALS(properties->getPropertyValue("InputFiles"), "input_workspace");
    TS_ASSERT_EQUALS(properties->getPropertyValue("SumFiles"), "0");
    TS_ASSERT_EQUALS(properties->getPropertyValue("LoadLogFiles"), "0");
    TS_ASSERT(!properties->existsProperty("CalibrationWorkspace"));
  }

  void testSetConversionPropertiesWithoutEfixed() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    IETConversionData conversionData(1.0, 1, 2);
    m_model->setConversionProperties(*properties, conversionData, "instrument");

    TS_ASSERT(!properties->existsProperty("Efixed"));
    TS_ASSERT_EQUALS(properties->getPropertyValue("SpectraRange"), "1, 2");
  }

  void testSetConversionPropertiesWithEfixed() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    IETConversionData conversionData(1.0, 1, 2);
    m_model->setConversionProperties(*properties, conversionData, "IRIS");

    TS_ASSERT_EQUALS(properties->getPropertyValue("Efixed"), "1");
    TS_ASSERT_EQUALS(properties->getPropertyValue("SpectraRange"), "1, 2");
  }

  void testSetBackgroundPropertiesWithBackgroundEnabled() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    IETBackgroundData backgroundData(true, 1.0, 2.0);
    m_model->setBackgroundProperties(*properties, backgroundData);

    TS_ASSERT_EQUALS(properties->getPropertyValue("BackgroundRange"), "1, 2");
  }

  void testSetBackgroundPropertiesWithBackgroundDisabled() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    IETBackgroundData backgroundData(false, 1.0, 2.0);
    m_model->setBackgroundProperties(*properties, backgroundData);

    TS_ASSERT(!properties->existsProperty("BackgroundRange"));
  }

  void testSetRebinPropertiesWithMultipleRebin() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    IETRebinData rebinData(true, "Multiple", 1.0, 2.0, 3.0, "1,2,10");
    m_model->setRebinProperties(*properties, rebinData);

    TS_ASSERT_EQUALS(properties->getPropertyValue("RebinString"), "1,2,10");
  }

  void testSetRebinPropertiesWithMultipleLogRebin() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    IETRebinData rebinData(true, "Multiple", 1.0, 2.0, 3.0, "2,-0.035,10");
    m_model->setRebinProperties(*properties, rebinData);

    TS_ASSERT_EQUALS(properties->getPropertyValue("RebinString"), "2,-0.035,10");
  }

  void testSetRebinPropertiesWithMultipleVariableRangeRebin() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    IETRebinData rebinData(true, "Multiple", 1.0, 2.0, 3.0, "0,2,10,4,20");
    m_model->setRebinProperties(*properties, rebinData);

    TS_ASSERT_EQUALS(properties->getPropertyValue("RebinString"), "0,2,10,4,20");
  }

  void testSetRebinPropertiesWithSingleRebin() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    IETRebinData rebinData(true, "Single", 0.0, 2.0, 6.0, "");
    m_model->setRebinProperties(*properties, rebinData);

    TS_ASSERT_EQUALS(properties->getPropertyValue("RebinString"), "0.000000,6.000000,2.000000");
  }

  void testSetRebinPropertiesWithNoRebin() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    IETRebinData rebinData(false, "Single", 0.0, 0.0, 0.0, "1.0, 3.0, 5.0");
    m_model->setRebinProperties(*properties, rebinData);

    TS_ASSERT(!properties->existsProperty("RebinString"));
  }

  void testSetAnalysisPropertiesWithPropsEnabled() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    IETAnalysisData analysisData(true, 2.5);
    m_model->setAnalysisProperties(*properties, analysisData);

    TS_ASSERT_EQUALS(properties->getPropertyValue("DetailedBalance"), "2.5");
  }

  void testSetAnalysisPropertiesWithPropsDisabled() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    IETAnalysisData analysisData(false, 2.5);
    m_model->setAnalysisProperties(*properties, analysisData);

    TS_ASSERT(!properties->existsProperty("DetailedBalance"));
  }

  void testSetOutputPropertiesWithPropsEnabled() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    IETOutputData outputData(true, true);
    m_model->setOutputProperties(*properties, outputData, "output", "label");

    TS_ASSERT_EQUALS(properties->getPropertyValue("UnitX"), "DeltaE_inWavenumber");
    TS_ASSERT_EQUALS(properties->getPropertyValue("FoldMultipleFrames"), "1");
    TS_ASSERT_EQUALS(properties->getPropertyValue("OutputWorkspace"), "output");
  }

  void testSetOutputPropertiesWithPropsDisabled() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    IETOutputData outputData(false, false);
    m_model->setOutputProperties(*properties, outputData, "output", "label");

    TS_ASSERT(!properties->existsProperty("UnitX"));
    TS_ASSERT_EQUALS(properties->getPropertyValue("FoldMultipleFrames"), "0");
    TS_ASSERT_EQUALS(properties->getPropertyValue("OutputWorkspace"), "output");
  }

  void testGetOutputGroupName() {

    InstrumentData instData("instrument", "analyser", "reflection");
    std::string inputFiles("1234, 1235");
    std::string outputName = m_model->getOutputGroupName(instData, inputFiles);

    TS_ASSERT_EQUALS(outputName, "instrument1234, 1235_analyser_reflection_Reduced");
  }

  void test_energyTransferAlgorithm() {
    IETInputData inputData("input_workspace1, input_workspace2", "input_workspace1, input_workspace2", true, false,
                           true, "calibration_workspace");
    IETConversionData conversionData(1.0, 1, 2);
    auto groupingProperties = defaultGroupingProps();
    IETBackgroundData backgroundData(true, 0, 1);
    IETAnalysisData analysisData(true, 2.5);
    IETRebinData rebinData(true, "Multiple", 0.0, 0.0, 0.0, "1,2");
    IETOutputData outputData(false, false);

    IETRunData runData(inputData, conversionData, std::move(groupingProperties), backgroundData, analysisData,
                       rebinData, outputData);

    InstrumentData instData("instrument", "analyser", "reflection");

    auto configuredAlg = m_model->energyTransferAlgorithm(instData, runData, "outputGroupName", "label");
    auto &runtimeProps = configuredAlg->getAlgorithmRuntimeProps();
    TS_ASSERT_EQUALS("instrument", runtimeProps.getPropertyValue("Instrument"));
    TS_ASSERT_EQUALS("analyser", runtimeProps.getPropertyValue("Analyser"));
    TS_ASSERT_EQUALS("reflection", runtimeProps.getPropertyValue("Reflection"));

    TS_ASSERT_EQUALS("input_workspace1, input_workspace2", runtimeProps.getPropertyValue("InputFiles"));
    TS_ASSERT_EQUALS("1", runtimeProps.getPropertyValue("SumFiles"));
    TS_ASSERT_EQUALS("0", runtimeProps.getPropertyValue("LoadLogFiles"));

    TS_ASSERT_EQUALS("1, 2", runtimeProps.getPropertyValue("SpectraRange"));
    TS_ASSERT_EQUALS("0, 1", runtimeProps.getPropertyValue("BackgroundRange"));
  }

  void testValidateRunDetailedBalanceInvalid() {
    IETInputData inputData("iris26184_multi_graphite002_red");
    IETConversionData conversionData(0.5, 1, 2);
    IETBackgroundData backgroundData(false);
    IETAnalysisData analysisData(true, 0.0);
    IETRebinData rebinData;
    IETOutputData outputData;

    IETRunData runData(inputData, conversionData, defaultGroupingProps(), backgroundData, analysisData, rebinData,
                       outputData);

    std::vector<std::string> errors = m_model->validateRunData(runData);
    TS_ASSERT_EQUALS(errors.size(), 1);
    if (errors.size() == 1)
      TS_ASSERT_EQUALS(errors[0], "Detailed Balance must be more than 0 K");
  }

  void testValidateRunDataSpectraInvalid() {
    IETInputData inputData("iris26184_multi_graphite002_red");
    IETConversionData conversionData(0.5, 4, 2);
    IETBackgroundData backgroundData(false);
    IETAnalysisData analysisData;
    IETRebinData rebinData;
    IETOutputData outputData;

    IETRunData runData(inputData, conversionData, defaultGroupingProps(), backgroundData, analysisData, rebinData,
                       outputData);

    std::vector<std::string> errors = m_model->validateRunData(runData);
    TS_ASSERT_EQUALS(errors.size(), 1);
    if (errors.size() == 1)
      TS_ASSERT_EQUALS(errors[0], "Minimum spectra must be less than maximum spectra.");
  }

  void testValidateRunDataBackgroundInvalid() {
    IETInputData inputData("iris26184_multi_graphite002_red");
    IETConversionData conversionData(0.5, 1, 2);
    IETBackgroundData backgroundData(true, -1, 1);
    IETAnalysisData analysisData;
    IETRebinData rebinData;
    IETOutputData outputData;

    IETRunData runData(inputData, conversionData, defaultGroupingProps(), backgroundData, analysisData, rebinData,
                       outputData);

    std::vector<std::string> errors = m_model->validateRunData(runData);
    TS_ASSERT_EQUALS(errors.size(), 2);
    if (errors.size() == 2) {
      TS_ASSERT_EQUALS(errors[0], "The Start of Background Removal is less than the minimum of the data range");
      TS_ASSERT_EQUALS(errors[1], "The End of Background Removal is more than the maximum of the data range");
    }
  }

  void testValidateRunDataAllValid() {
    IETInputData inputData("iris26184_multi_graphite002_red");
    IETConversionData conversionData(0.5, 1, 2);
    IETBackgroundData backgroundData(false);
    IETAnalysisData analysisData;
    IETRebinData rebinData;
    IETOutputData outputData;

    IETRunData runData(inputData, conversionData, defaultGroupingProps(), backgroundData, analysisData, rebinData,
                       outputData);

    std::vector<std::string> errors = m_model->validateRunData(runData);
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
  IAlgorithm_sptr makeReductionAlgorithm() {
    auto alg = AlgorithmManager::Instance().create("ISISIndirectEnergyTransfer");
    alg->setLogging(false);
    return alg;
  }

  std::unique_ptr<IETModel> m_model;
  IAlgorithm_sptr m_reductionAlg;
};
