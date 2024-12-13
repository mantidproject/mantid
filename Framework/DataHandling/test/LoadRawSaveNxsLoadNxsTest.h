// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadNexus.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/SaveNexusProcessed.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <cxxtest/TestSuite.h>
#include <fstream>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class LoadRawSaveNxsLoadNxsTest : public CxxTest::TestSuite {
public:
  void testInit() {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT(algToBeTested.isInitialized());
  }

  void testExecOnLoadraw() {
    // use SaveNexusProcessed to build a test file to load
    // for this use LoadRaw
    std::string inputFile = "CSP78173.raw";
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", inputFile);

    outputSpace = "csp78173";
    loader.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    //
    // get workspace
    //
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    if (!saveNexusP.isInitialized())
      saveNexusP.initialize();

    //
    saveNexusP.setPropertyValue("InputWorkspace", outputSpace);
    // specify name of file to save workspace to
    outputFile = "testSaveLoadrawCSP.nxs";
    remove(outputFile.c_str());
    std::string title = "Workspace from Loadraw CSP78173";
    saveNexusP.setPropertyValue("FileName", outputFile);
    outputFile = saveNexusP.getPropertyValue("Filename");
    // saveNexusP.setPropertyValue("EntryName", entryName);
    saveNexusP.setPropertyValue("Title", title);

    TS_ASSERT_THROWS_NOTHING(saveNexusP.execute());
    TS_ASSERT(saveNexusP.isExecuted());
  }

  void testExecRaw() {
    // test LoadNexusProcessed reading the data from SNP on Loadraw CSP78173

    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    // specify name of workspace
    myOutputSpace = "testLNP3";
    TS_ASSERT_THROWS_NOTHING(algToBeTested.setPropertyValue("OutputWorkspace", myOutputSpace));
    // file name to load
    inputFile = outputFile;
    entryNumber = 1;
    TS_ASSERT_THROWS_NOTHING(algToBeTested.setPropertyValue("FileName", inputFile));
    algToBeTested.setProperty("EntryNumber", entryNumber);

    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = algToBeTested.getPropertyValue("FileName"));
    TS_ASSERT(!result.compare(inputFile));
    TS_ASSERT_THROWS_NOTHING(result = algToBeTested.getPropertyValue("OutputWorkspace"));
    TS_ASSERT(!result.compare(myOutputSpace));
    int res = -1;
    TS_ASSERT_THROWS_NOTHING(res = algToBeTested.getProperty("EntryNumber"));
    TS_ASSERT(res == entryNumber);

    // Test that nexus precessed file is successfully loaded
    // The loading of the current version of nexus processed is tested here.
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(myOutputSpace));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    // set to 4 for CSP78173
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 4);
    // Check two X vectors are the same
    TS_ASSERT((output2D->dataX(1)) == (output2D->dataX(3)));
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->dataY(1).size(), output2D->dataY(2).size());
    // Check one particular value
    TS_ASSERT_EQUALS(output2D->dataY(1)[14], 9.0);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->dataE(1)[14], 3.0);
    // Check that the X data is as expected
    TS_ASSERT_EQUALS(output2D->dataX(2)[777], 15550.0);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "TOF");
    TS_ASSERT(!output->isDistribution());
    // Check units of Y axis are "Counts"
    TS_ASSERT_EQUALS(output->YUnit(), "Counts");

    // Check the proton charge has been set correctly
    TS_ASSERT_DELTA(output->run().getProtonCharge(), 0.8347, 0.0001);

    //
    // check that the instrument data has been loaded, copied from
    // LoadInstrumentTest
    //
    Instrument_const_sptr i = output->getInstrument();
    const auto &detectorInfo = output->detectorInfo();

    // std::cerr << "Count = " << i.use_count();
    std::shared_ptr<const IComponent> source = i->getSource();
    TS_ASSERT(source != nullptr);
    if (source != nullptr) {
      TS_ASSERT_EQUALS(source->getName(), "source");
      TS_ASSERT_DELTA(detectorInfo.sourcePosition().Y(), 0.0, 0.01);

      std::shared_ptr<const IComponent> samplepos = i->getSample();
      TS_ASSERT_EQUALS(samplepos->getName(), "some-surface-holder");
      TS_ASSERT_DELTA(detectorInfo.samplePosition().X(), 0.0, 0.01);

      size_t detectorIndex;
      auto hasDetector(true);
      try {
        detectorIndex = detectorInfo.indexOf(103);
      } catch (std::out_of_range &) {
        hasDetector = false;
      }

      if (hasDetector) {
        const auto &detector103 = detectorInfo.detector(detectorIndex);
        TS_ASSERT_EQUALS(detector103.getID(), 103);
        TS_ASSERT_EQUALS(detector103.getName(), "linear-detector-pixel");
        TS_ASSERT_DELTA(detectorInfo.position(detectorIndex).Z(), 12.403, 0.01);
        TS_ASSERT_DELTA(detectorInfo.position(detectorIndex).Y(), 0.1164, 0.01);
        const auto d = detectorInfo.l2(detectorIndex);
        TS_ASSERT_DELTA(d, 2.1477, 0.0001);
      }
    }

    //----------------------------------------------------------------------
    // Tests to check that spectra-detector mapping is done correctly
    //----------------------------------------------------------------------
    TS_ASSERT_EQUALS(output2D->getSpectrum(0).getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS(output2D->getSpectrum(0).getSpectrumNo(), 1);
    TS_ASSERT(output2D->getSpectrum(0).hasDetectorID(1));

    // obtain the expected log data which was read from the Nexus file (NXlog)

    Property *l_property = output->run().getLogData(std::string("height"));
    TimeSeriesProperty<double> *l_timeSeriesDouble1 = dynamic_cast<TimeSeriesProperty<double> *>(l_property);
    std::string timeSeriesString = l_timeSeriesDouble1->value();

    //
    // Testing log data - this was failing at one time as internal format of log
    // data changed, but now OK again
    //
    TS_ASSERT_EQUALS(timeSeriesString.substr(0, 30), "2008-Jun-17 11:10:44  -0.86526");

    l_property = output->run().getLogData(std::string("ICPevent"));
    TimeSeriesProperty<std::string> *l_timeSeriesString = dynamic_cast<TimeSeriesProperty<std::string> *>(l_property);
    timeSeriesString = l_timeSeriesString->value();

    //
    // Testing log data - this was failing at one time as internal format of log
    // data changed, but now OK again
    // It was disabled, with a TODO comment, with this string: "2008-Jun-17
    // 11:11:13  CHANGE PERIOD 12",
    // now enabled after changing 12=> 1 (and added one more space character
    // before CHANGE).
    TS_ASSERT_EQUALS(timeSeriesString.substr(0, 38), "2008-Jun-17 11:11:13   CHANGE PERIOD 1");

    remove(outputFile.c_str());
  }

  void testSaveLoadSingleValuedWs() {
    // Tests LoadNexusProcessed saving and loading a single valued workspace
    MatrixWorkspace_sptr singleValuedWs = WorkspaceCreationHelper::createWorkspaceSingleValue(2.2);
    // step 1: save the single valued workspace
    saveNexusP.setProperty("InputWorkspace", singleValuedWs);
    outputFile = "testSaveLoadSingleValuedWs.nxs";
    remove(outputFile.c_str());
    saveNexusP.setPropertyValue("FileName", outputFile);
    outputFile = saveNexusP.getPropertyValue("Filename");

    TS_ASSERT_THROWS_NOTHING(saveNexusP.execute());
    TS_ASSERT(saveNexusP.isExecuted());
    TS_ASSERT(Poco::File(outputFile).exists());

    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    // specify name of workspace
    myOutputSpace = "singleValuedWs";
    TS_ASSERT_THROWS_NOTHING(algToBeTested.setPropertyValue("OutputWorkspace", myOutputSpace));
    // file name to load
    inputFile = outputFile;
    TS_ASSERT_THROWS_NOTHING(algToBeTested.setPropertyValue("FileName", inputFile));

    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = algToBeTested.getPropertyValue("FileName"));
    TS_ASSERT(!result.compare(inputFile));
    TS_ASSERT_THROWS_NOTHING(result = algToBeTested.getPropertyValue("OutputWorkspace"));
    TS_ASSERT(!result.compare(myOutputSpace));

    // Test that nexus precessed file is successfully loaded
    // The loading of the current version of nexus processed is tested here.
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    // get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(myOutputSpace));
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 1);
    // Check two X vectors are the same
    TS_ASSERT((output->dataX(0)) == (singleValuedWs->dataX(0)));
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output->dataY(0).size(), singleValuedWs->dataY(0).size());
  }

private:
  LoadNexus algToBeTested;
  std::string inputFile;
  int entryNumber;

  std::string myOutputSpace;

  SaveNexusProcessed saveNexusP;
  Mantid::DataHandling::LoadRaw3 loader;
  std::string outputSpace;
  std::string outputFile;
};
