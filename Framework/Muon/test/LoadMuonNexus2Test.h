// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this
// test case.
#include "MantidDataObjects/WorkspaceSingleValue.h"

#include <cxxtest/TestSuite.h>
#include <fstream>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidMuon/LoadMuonNexus2.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using Mantid::detid_t;
using Mantid::Types::Core::DateAndTime;

class LoadMuonNexus2Test : public CxxTest::TestSuite {
public:
  void check_spectra_and_detectors(const MatrixWorkspace_sptr &output) {

    //----------------------------------------------------------------------
    // Tests to check that spectra-detector mapping is done correctly
    //----------------------------------------------------------------------
    // Check the total number of elements in the map for HET
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 192);

    // Test one to one mapping, for example spectra 6 has only 1 pixel
    TS_ASSERT_EQUALS(output->getSpectrum(6).getDetectorIDs().size(), 1);

    auto detectorgroup = output->getSpectrum(99).getDetectorIDs();
    TS_ASSERT_EQUALS(detectorgroup.size(), 1);
    TS_ASSERT_EQUALS(*detectorgroup.begin(), 100);
  }

  void testExec() {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    std::string inputFile = "argus0026287.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);

    std::string outputSpace = "outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());

    //
    // Test additional output parameters
    //
    std::string field = nxLoad.getProperty("MainFieldDirection");
    TS_ASSERT(field == "Transverse");
    double timeZero = nxLoad.getProperty("TimeZero");
    TS_ASSERT_DELTA(timeZero, 0.224, 0.001);
    double firstGood = nxLoad.getProperty("FirstGoodData");
    TS_ASSERT_DELTA(firstGood, 0.384, 0.001);
    double lastGood = nxLoad.getProperty("LastGoodData");
    TS_ASSERT_DELTA(lastGood, 32.0, 0.001);
    //

    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 192 for file inputFile = "argus0026287.nxs";
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 192);
    TS_ASSERT_EQUALS(output2D->blocksize(), 2000);
    // Check two X vectors are the same
    TS_ASSERT((output2D->x(3)) == (output2D->x(31)));
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->y(5).size(), output2D->y(17).size());
    // Check one particular value
    TS_ASSERT_EQUALS(output2D->y(11)[686], 9);
    TS_ASSERT_EQUALS(output2D->y(12)[686], 7);
    TS_ASSERT_EQUALS(output2D->y(13)[686], 7);

    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->e(11)[686], 3);
    TS_ASSERT_DELTA(output2D->e(12)[686], 2.646, 0.001);
    TS_ASSERT_DELTA(output2D->e(13)[686], 2.646, 0.001);
    // Check that the time is as expected from bin boundary update
    TS_ASSERT_DELTA(output2D->x(11)[687], 10.992, 0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Label");
    TS_ASSERT(!output->isDistribution());

    Property *l_property = output->run().getLogData(std::string("temperature_1_log"));
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double> *>(l_property);
    std::map<DateAndTime, double> asMap = l_timeSeriesDouble->valueAsMap();
    TS_ASSERT_EQUALS(l_timeSeriesDouble->size(), 37);
    TS_ASSERT_EQUALS(l_timeSeriesDouble->nthValue(10), 180.0);
    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS(timeSeriesString.substr(0, 25), "2008-Sep-11 14:17:41  180");
    // check that sample name has been set correctly
    TS_ASSERT_EQUALS(output->sample().getName(), "GaAs");

    check_spectra_and_detectors(output);

    AnalysisDataService::Instance().remove(outputSpace);
  }

  void testMinMax() {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    nxLoad.setPropertyValue("FileName", "argus0026287.nxs");
    std::string outputSpace = "outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);
    nxLoad.setPropertyValue("SpectrumMin", "10");
    nxLoad.setPropertyValue("SpectrumMax", "20");

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());
    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 11);
    TS_ASSERT_EQUALS(output2D->blocksize(), 2000);
    // Check two X vectors are the same
    TS_ASSERT((output2D->x(3)) == (output2D->x(7)));
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->y(5).size(), output2D->y(10).size());

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Label");
    TS_ASSERT(!output->isDistribution());

    AnalysisDataService::Instance().remove(outputSpace);
  }

  void testList() {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    nxLoad.setPropertyValue("FileName", "argus0026287.nxs");
    std::string outputSpace = "outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);
    nxLoad.setPropertyValue("SpectrumList", "1,10,20");

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());
    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 3);
    TS_ASSERT_EQUALS(output2D->blocksize(), 2000);
    // Check two X vectors are the same
    TS_ASSERT((output2D->x(0)) == (output2D->x(2)));
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->y(0).size(), output2D->y(1).size());

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Label");
    TS_ASSERT(!output->isDistribution());

    AnalysisDataService::Instance().remove(outputSpace);
  }

  void testMinMax_List() {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    nxLoad.setPropertyValue("FileName", "argus0026287.nxs");
    std::string outputSpace = "outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);
    nxLoad.setPropertyValue("SpectrumMin", "10");
    nxLoad.setPropertyValue("SpectrumMax", "20");
    nxLoad.setPropertyValue("SpectrumList", "30,40,50");

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());
    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 14);
    TS_ASSERT_EQUALS(output2D->blocksize(), 2000);
    // Check two X vectors are the same
    TS_ASSERT((output2D->x(3)) == (output2D->x(7)));
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->y(5).size(), output2D->y(10).size());

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Label");
    TS_ASSERT(!output->isDistribution());

    AnalysisDataService::Instance().remove(outputSpace);
  }

  /// Test that spectrum numbers and detector IDs are set correctly
  void testList_spectrumNumber_detectorID() {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();
    nxLoad.setChild(true);
    nxLoad.setPropertyValue("FileName", "argus0026287.nxs");
    nxLoad.setPropertyValue("OutputWorkspace", "__NotUsed");
    nxLoad.setPropertyValue("SpectrumMin", "5");
    nxLoad.setPropertyValue("SpectrumMax", "10");
    nxLoad.setPropertyValue("SpectrumList", "29, 31");
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());

    Workspace_sptr outWS = nxLoad.getProperty("OutputWorkspace");
    const auto loadedWS = std::dynamic_pointer_cast<Workspace2D>(outWS);
    TS_ASSERT(loadedWS);

    // Check the right spectra have been loaded
    const std::vector<Mantid::specnum_t> expectedSpectra{5, 6, 7, 8, 9, 10, 29, 31};
    TS_ASSERT_EQUALS(loadedWS->getNumberHistograms(), expectedSpectra.size());
    for (size_t i = 0; i < loadedWS->getNumberHistograms(); ++i) {
      const auto spec = loadedWS->getSpectrum(i);
      TS_ASSERT_EQUALS(spec.getSpectrumNo(), expectedSpectra[i]);
      // detector ID = spectrum number for this muon Nexus v2 file
      const auto detIDs = spec.getDetectorIDs();
      TS_ASSERT_EQUALS(detIDs.size(), 1);
      TS_ASSERT_EQUALS(*detIDs.begin(), static_cast<int>(spec.getSpectrumNo()));
    }
  }

  void testExec1() {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    std::string inputFile = "argus0026577.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);

    std::string outputSpace = "outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());
    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace + "_1");
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 192 for file inputFile = "argus0026287.nxs";
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 192);
    TS_ASSERT_EQUALS(output2D->blocksize(), 2000);
    // Check two X vectors are the same
    TS_ASSERT((output2D->x(3)) == (output2D->x(31)));
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->y(5).size(), output2D->y(17).size());
    // Check one particular value
    TS_ASSERT_EQUALS(output2D->y(11)[686], 7);
    TS_ASSERT_EQUALS(output2D->y(12)[686], 2);
    TS_ASSERT_EQUALS(output2D->y(13)[686], 6);

    // Check that the error on that value is correct
    TS_ASSERT_DELTA(output2D->e(11)[686], 2.646, 0.001);
    TS_ASSERT_DELTA(output2D->e(12)[686], 1.414, 0.001);
    TS_ASSERT_DELTA(output2D->e(13)[686], 2.449, 0.001);
    // Check that the time is as expected from bin boundary update
    TS_ASSERT_DELTA(output2D->x(11)[687], 10.992, 0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Label");
    TS_ASSERT(!output->isDistribution());

    //----------------------------------------------------------------------
    // Test code copied from LoadLogTest to check Child Algorithm is running
    // properly
    //----------------------------------------------------------------------
    Property *l_property = output->run().getLogData(std::string("temperature_1_log"));
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double> *>(l_property);
    std::map<DateAndTime, double> asMap = l_timeSeriesDouble->valueAsMap();
    TS_ASSERT_EQUALS(l_timeSeriesDouble->size(), 42);
    TS_ASSERT_DELTA(l_timeSeriesDouble->nthValue(10), 7.3146, 0.0001);
    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS(timeSeriesString.substr(0, 25), "2008-Sep-18 00:57:19  7.3");
    // check that sample name has been set correctly
    TS_ASSERT_EQUALS(output->sample().getName(), "GaAs");

    check_spectra_and_detectors(output);

    AnalysisDataService::Instance().clear();
  }

  void testExec2() {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    std::string inputFile = "argus0031800.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);

    std::string outputSpace = "outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());
    //
    // Test workspace data (copied from LoadRawTest.h)
    //
    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace + "_2");
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 192 for file inputFile = "argus0026287.nxs";
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 192);
    TS_ASSERT_EQUALS(output2D->blocksize(), 2000);
    // Check two X vectors are the same
    TS_ASSERT((output2D->x(3)) == (output2D->x(31)));
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->y(5).size(), output2D->y(17).size());
    // Check one particular value
    TS_ASSERT_EQUALS(output2D->y(11)[686], 4);
    TS_ASSERT_EQUALS(output2D->y(12)[686], 6);
    TS_ASSERT_EQUALS(output2D->y(13)[686], 0);

    // Check that the error on that value is correct
    TS_ASSERT_DELTA(output2D->e(11)[686], 2, 0.001);
    TS_ASSERT_DELTA(output2D->e(12)[686], 2.449, 0.001);
    TS_ASSERT_DELTA(output2D->e(13)[686], 0, 0.001);
    // Check that the time is as expected from bin boundary update
    TS_ASSERT_DELTA(output2D->x(11)[687], 10.992, 0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Label");
    TS_ASSERT(!output->isDistribution());

    Property *l_property = output->run().getLogData(std::string("temperature_1_log"));
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double> *>(l_property);
    std::map<DateAndTime, double> asMap = l_timeSeriesDouble->valueAsMap();
    TS_ASSERT_EQUALS(l_timeSeriesDouble->size(), 31);
    TS_ASSERT_DELTA(l_timeSeriesDouble->nthValue(10), 10.644, 0.0001);
    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS(timeSeriesString.substr(0, 25), "2009-Jul-08 10:23:50  10.");
    // check that sample name has been set correctly
    TS_ASSERT_EQUALS(output->sample().getName(), "GaAs");

    check_spectra_and_detectors(output);

    AnalysisDataService::Instance().clear();
  }

  void test_gpd_file() {
    LoadMuonNexus2 nxLoad;
    nxLoad.initialize();

    // Now set required filename and output workspace name
    std::string inputFile = "deltat_tdc_gpd_0900.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);

    std::string outputSpace = "outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);

    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());
    TS_ASSERT(nxLoad.isExecuted());

    //
    // Test additional output parameters
    //
    std::string field = nxLoad.getProperty("MainFieldDirection");
    TS_ASSERT(field == "Transverse");
    //  TimeZero, FirstGoodData and LastGoodData are not read yet so they are 0
    double timeZero = nxLoad.getProperty("TimeZero");
    TS_ASSERT_DELTA(timeZero, 0.0, 0.001);
    double firstGood = nxLoad.getProperty("FirstGoodData");
    TS_ASSERT_DELTA(firstGood, 0.0, 0.001);
    double lastGood = nxLoad.getProperty("LastGoodData");
    TS_ASSERT_DELTA(lastGood, 0.0, 0.001);

    //
    // Test workspace data
    //
    MatrixWorkspace_sptr output;
    output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 192 for file inputFile = "argus0026287.nxs";
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(output2D->blocksize(), 8192);
    // Check two X vectors are the same
    TS_ASSERT((output2D->x(0)) == (output2D->x(1)));
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->y(0).size(), output2D->y(1).size());
    // Check one particular value
    TS_ASSERT_EQUALS(output2D->y(0)[686], 516);
    TS_ASSERT_EQUALS(output2D->y(0)[687], 413);
    TS_ASSERT_EQUALS(output2D->y(1)[686], 381);

    // Check that the error on that value is correct
    TS_ASSERT_DELTA(output2D->e(0)[686], 22.7156, 0.001);
    TS_ASSERT_DELTA(output2D->e(0)[687], 20.3224, 0.001);
    TS_ASSERT_DELTA(output2D->e(1)[686], 19.5192, 0.001);
    // Check that the time is as expected from bin boundary update
    TS_ASSERT_DELTA(output2D->x(1)[687], 0.8050, 0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Label");
    TS_ASSERT(!output->isDistribution());

    AnalysisDataService::Instance().remove(outputSpace);
  }
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadMuonNexus2TestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00006475.nxs");
    loader.setPropertyValue("OutputWorkspace", "ws");
  }

  void tearDown() override { AnalysisDataService::Instance().remove("ws"); }

  void testDefaultLoad() { loader.execute(); }

private:
  LoadMuonNexus2 loader;
};
