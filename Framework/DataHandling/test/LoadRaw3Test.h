// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::Types::Core::DateAndTime;

class LoadRaw3Test : public CxxTest::TestSuite {
public:
  static LoadRaw3Test *createSuite() { return new LoadRaw3Test(); }
  static void destroySuite(LoadRaw3Test *suite) { delete suite; }

  LoadRaw3Test() {
    // Path to test input file assumes Test directory checked out from SVN
    inputFile = "HET15869.raw";
  }

  /// <summary>
  /// Test that the log file paths are correctly found on Windows when using the alternate data stream.
  /// </summary>
  void testAlternateDataStream() {
#ifdef _WIN32
    Poco::Path rawFilePath("./fakeRawFile.raw");
    Poco::File rawFile(rawFilePath);

    std::ofstream file(rawFile.path());
    file << "data goes here";

    std::string adsFileName = rawFile.path() + ":checksum";
    std::ofstream adsFile(adsFileName);
    adsFile << "ad0bc56c4c556fa368565000f01e77f7 *fakeRawFile.log" << std::endl;
    adsFile << "d5ace6dc7ac6c4365d48ee1f2906c6f4 *fakeRawFile.nxs" << std::endl;
    adsFile << "9c70ad392023515f775af3d3984882f3 *fakeRawFile.raw" << std::endl;
    adsFile << "66f74b6c0cc3eb497b92d4956ed8d6b5 *fakeRawFile_ICPdebug.txt" << std::endl;
    adsFile << "e200aa65186b61e487175d5263b315aa *fakeRawFile_ICPevent.txt" << std::endl;
    adsFile << "91be40aa4f54d050a9eb4abea394720e *fakeRawFile_ICPstatus.txt" << std::endl;
    adsFile << "50aa2872110a9b862b01c6c83f8ce9a8 *fakeRawFile_Status.txt" << std::endl;

    file.close();
    adsFile.close();

    // Create the log files, otherwise the searchForLogFiles function won't include them in the
    // list of log files.
    Poco::File logFile("./fakeRawFile.log");
    logFile.createFile();
    Poco::File icpDebugFile("./fakeRawFile_ICPdebug.txt");
    icpDebugFile.createFile();
    Poco::File icpEventFile("./fakeRawFile_ICPevent.txt");
    icpEventFile.createFile();
    Poco::File icpStatusFile("./fakeRawFile_ICPstatus.txt");
    icpStatusFile.createFile();
    Poco::File statusFile("./fakeRawFile_Status.txt");
    statusFile.createFile();

    std::list<std::string> logFiles = LoadRawHelper::searchForLogFiles(rawFilePath);

    // One .log and four .txt files are listed in the alternate data stream.
    TS_ASSERT_EQUALS(5, logFiles.size());
    TS_ASSERT(std::find(logFiles.begin(), logFiles.end(), "fakeRawFile.log") != logFiles.end());
    TS_ASSERT(std::find(logFiles.begin(), logFiles.end(), "fakeRawFile_ICPdebug.txt") != logFiles.end());
    TS_ASSERT(std::find(logFiles.begin(), logFiles.end(), "fakeRawFile_ICPevent.txt") != logFiles.end());
    TS_ASSERT(std::find(logFiles.begin(), logFiles.end(), "fakeRawFile_ICPstatus.txt") != logFiles.end());
    TS_ASSERT(std::find(logFiles.begin(), logFiles.end(), "fakeRawFile_Status.txt") != logFiles.end());

    rawFile.remove();
    logFile.remove();
    icpDebugFile.remove();
    icpEventFile.remove();
    icpStatusFile.remove();
    statusFile.remove();
#endif
  }

  /// <summary>
  /// Check that the .log file is added to the list of log files when loading a raw file.
  /// </summary>
  void testLogFileSearch() {
    std::string rawFileName = FileFinder::Instance().getFullPath("NIMROD00001097.raw");
    Poco::Path rawFilePath(rawFileName);

    std::list<std::string> logFiles = LoadRawHelper::searchForLogFiles(rawFilePath);

    // Count number of log files ending in ".log" - should be exactly one.
    int logCount = 0;
    for (const auto &logFile : logFiles) {
      if (boost::algorithm::iends_with(logFile, ".log")) {
        ++logCount;
      }
    }

    TS_ASSERT_EQUALS(1, logCount);
  }

  void testConfidence() {
    using Mantid::Kernel::FileDescriptor;
    LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", inputFile);

    FileDescriptor descriptor(loader.getPropertyValue("Filename"));
    TS_ASSERT_EQUALS(80, loader.confidence(descriptor));
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
  }

  void testExec() {

    if (!loader.isInitialized())
      loader.initialize();

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(loader.execute(), const std::runtime_error &);

    // Now set it...
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("LoadMonitors", "Include");

    outputSpace = "outer";
    loader.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 2584 for file HET15869.RAW
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 2584);
    // Check two X vectors are the same
    TS_ASSERT((output2D->dataX(99)) == (output2D->dataX(1734)));
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->dataY(673).size(), output2D->dataY(2111).size());
    // Check one particular value
    TS_ASSERT_EQUALS(output2D->dataY(999)[777], 9);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->dataE(999)[777], 3);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->dataX(999)[777], 554.1875);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS(output2D->getAxis(0)->unit()->unitID(), "TOF")
    TS_ASSERT(!output2D->isDistribution())

    // Check the proton charge has been set correctly
    TS_ASSERT_DELTA(output2D->run().getProtonCharge(), 171.0353, 0.0001)

    //----------------------------------------------------------------------
    // Tests taken from LoadInstrumentTest to check Child Algorithm is running
    // properly
    //  ----------------------------------------------------------------------
    std::shared_ptr<const Instrument> i = output2D->getInstrument();
    std::shared_ptr<const Mantid::Geometry::IComponent> source = i->getSource();

    TS_ASSERT_EQUALS(source->getName(), "undulator");
    TS_ASSERT_DELTA(source->getPos().Y(), 0.0, 0.01);

    std::shared_ptr<const Mantid::Geometry::IComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS(samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA(samplepos->getPos().Z(), 0.0, 0.01);

    const auto &detectorInfo = output2D->detectorInfo();
    const auto detIndex = detectorInfo.indexOf(103);
    const auto &det103 = detectorInfo.detector(detIndex);
    TS_ASSERT_EQUALS(det103.getID(), 103);
    TS_ASSERT_EQUALS(det103.getName(), "HET_non_PSDtube");
    TS_ASSERT_DELTA(detectorInfo.position(detIndex).X(), 0.4013, 0.01);
    TS_ASSERT_DELTA(detectorInfo.position(detIndex).Z(), 2.4470, 0.01);

    //----------------------------------------------------------------------
    // Test code copied from LoadLogTest to check Child Algorithm is running
    // properly
    //----------------------------------------------------------------------
    // std::shared_ptr<Sample> sample = output2D->getSample();
    Property *l_property = output2D->run().getLogData(std::string("TEMP1"));
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double> *>(l_property);

    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS(timeSeriesString.substr(0, 23), "2007-Nov-13 15:16:20  0");

    l_property = output2D->run().getLogData("run_number");
    TS_ASSERT_EQUALS(l_property->value(), "15869");

    //----------------------------------------------------------------------
    // Tests to check that spectra-detector mapping is done correctly
    //----------------------------------------------------------------------
    // Test one to one mapping, for example spectra 6 has only 1 pixel
    TS_ASSERT_EQUALS(output2D->getSpectrum(6).getDetectorIDs().size(),
                     1); // rummap.ndet(6),1);

    // Test one to many mapping, for example 10 pixels contribute to spectra
    // 2084 (workspace index 2083)
    TS_ASSERT_EQUALS(output2D->getSpectrum(2083).getDetectorIDs().size(),
                     10); // map.ndet(2084),10);

    // Check the id number of all pixels contributing
    std::set<detid_t> detectorgroup;
    detectorgroup = output2D->getSpectrum(2083).getDetectorIDs();
    std::set<detid_t>::const_iterator it;
    int pixnum = 101191;
    for (it = detectorgroup.begin(); it != detectorgroup.end(); ++it)
      TS_ASSERT_EQUALS(*it, pixnum++);

    AnalysisDataService::Instance().remove(outputSpace);
  }

  void testMixedLimits() {
    if (!loader2.isInitialized())
      loader2.initialize();

    loader2.setPropertyValue("Filename", inputFile);
    loader2.setPropertyValue("OutputWorkspace", "outWS");
    loader2.setPropertyValue("SpectrumList", "998,999,1000");
    loader2.setPropertyValue("SpectrumMin", "5");
    loader2.setPropertyValue("SpectrumMax", "10");

    TS_ASSERT_THROWS_NOTHING(loader2.execute());
    TS_ASSERT(loader2.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("outWS"));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    // Should be 6 for selected input
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 9);

    // Check two X vectors are the same
    TS_ASSERT((output2D->dataX(1)) == (output2D->dataX(5)));

    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->dataY(2).size(), output2D->dataY(7).size());

    // Check one particular value
    TS_ASSERT_EQUALS(output2D->dataY(8)[777], 9);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->dataE(8)[777], 3);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->dataX(8)[777], 554.1875);
  }

  void testMinlimit() {
    LoadRaw3 alg;
    std::string outWS = "outWSLimitTest";
    if (!alg.isInitialized())
      alg.initialize();

    alg.setPropertyValue("Filename", inputFile);
    alg.setPropertyValue("OutputWorkspace", outWS);
    alg.setPropertyValue("SpectrumMin", "2580");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outWS));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 5);
    AnalysisDataService::Instance().remove(outWS);
  }

  void testMaxlimit() {
    LoadRaw3 alg;
    std::string outWS = "outWSLimitTest";
    if (!alg.isInitialized())
      alg.initialize();

    alg.setPropertyValue("Filename", inputFile);
    alg.setPropertyValue("OutputWorkspace", outWS);
    alg.setPropertyValue("SpectrumMax", "5");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outWS));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 5);
    AnalysisDataService::Instance().remove(outWS);
  }

  void testMinMaxlimit() {
    LoadRaw3 alg;
    std::string outWS = "outWSLimitTest";
    if (!alg.isInitialized())
      alg.initialize();

    alg.setPropertyValue("Filename", inputFile);
    alg.setPropertyValue("OutputWorkspace", outWS);
    alg.setPropertyValue("SpectrumMin", "5");
    alg.setPropertyValue("SpectrumMax", "10");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outWS));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 6);
    TS_ASSERT_EQUALS(output2D->getSpectrum(0).getSpectrumNo(), 5);
    TS_ASSERT_EQUALS(output2D->getSpectrum(1).getSpectrumNo(), 6);
    TS_ASSERT(output2D->getSpectrum(1).hasDetectorID(4103));
    TS_ASSERT_EQUALS(output2D->getSpectrum(5).getSpectrumNo(), 10);
    TS_ASSERT(output2D->getSpectrum(5).hasDetectorID(4107));
    AnalysisDataService::Instance().remove(outWS);
  }

  void testListlimit() {
    LoadRaw3 alg;
    std::string outWS = "outWSLimitTest";
    if (!alg.isInitialized())
      alg.initialize();

    alg.setPropertyValue("Filename", inputFile);
    alg.setPropertyValue("OutputWorkspace", outWS);
    alg.setPropertyValue("SpectrumList", "998,999,1000");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outWS));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 3);
    AnalysisDataService::Instance().remove(outWS);
  }

  void testfail() {
    LoadRaw3 loader3;
    if (!loader3.isInitialized())
      loader3.initialize();
    std::string outWS = "LoadRaw3-out2";
    loader3.setPropertyValue("Filename", inputFile);
    loader3.setPropertyValue("OutputWorkspace", outWS);
    loader3.setPropertyValue("SpectrumList", "0,999,1000");
    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "10");
    loader3.execute();
    Workspace_sptr output;
    // test that there is no workspace as it should have failed
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve(outWS), const std::runtime_error &);

    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "1");
    loader3.execute();
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve(outWS), const std::runtime_error &);

    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "3");
    loader3.execute();
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve(outWS), const std::runtime_error &);

    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "5");
    loader3.execute();
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve(outWS), const std::runtime_error &);

    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "3000");
    loader3.execute();
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve(outWS), const std::runtime_error &);

    loader3.setPropertyValue("SpectrumMin", "5");
    loader3.setPropertyValue("SpectrumMax", "10");
    loader3.setPropertyValue("SpectrumList", "999,3000");
    loader3.execute();
    TS_ASSERT_THROWS(output = AnalysisDataService::Instance().retrieve(outWS), const std::runtime_error &);

    loader3.setPropertyValue("SpectrumList", "999,2000");
    loader3.execute();
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outWS));
    AnalysisDataService::Instance().remove(outWS);
  }

  void testMultiPeriod() {
    LoadRaw3 loader5;
    loader5.initialize();
    loader5.setPropertyValue("Filename", "CSP78173.raw");
    loader5.setPropertyValue("OutputWorkspace", "multiperiod");
    // loader5.setPropertyValue("SpectrumList", "0,1,2,3");

    TS_ASSERT_THROWS_NOTHING(loader5.execute())
    TS_ASSERT(loader5.isExecuted())

    WorkspaceGroup_sptr work_out;
    TS_ASSERT_THROWS_NOTHING(work_out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("multiperiod"));

    Workspace_sptr wsSptr = AnalysisDataService::Instance().retrieve("multiperiod");
    WorkspaceGroup_sptr sptrWSGrp = std::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
    std::vector<std::string> wsNamevec;
    wsNamevec = sptrWSGrp->getNames();
    int period = 1;
    std::vector<std::string>::const_iterator it = wsNamevec.begin();
    for (; it != wsNamevec.end(); ++it) {
      std::stringstream count;
      count << period;
      std::string wsName = "multiperiod_" + count.str();
      TS_ASSERT_EQUALS(*it, wsName)
      period++;
    }
    std::vector<std::string>::const_iterator itr1 = wsNamevec.begin();
    int periodNumber = 0;
    const int nHistograms = 4;
    for (; itr1 != wsNamevec.end(); ++itr1) {
      MatrixWorkspace_sptr outsptr;
      TS_ASSERT_THROWS_NOTHING(outsptr = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>((*itr1)));
      doTestMultiPeriodWorkspace(outsptr, nHistograms, ++periodNumber);
    }
    std::vector<std::string>::const_iterator itr = wsNamevec.begin();
    MatrixWorkspace_sptr outsptr1;
    TS_ASSERT_THROWS_NOTHING(outsptr1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>((*itr)));
    MatrixWorkspace_sptr outsptr2;
    TS_ASSERT_THROWS_NOTHING(outsptr2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>((*++itr)));

    TS_ASSERT_EQUALS(outsptr1->dataX(0), outsptr2->dataX(0))

    // But the data should be different
    TS_ASSERT_DIFFERS(outsptr1->dataY(1)[8], outsptr2->dataY(1)[8])

    TS_ASSERT_EQUALS(outsptr1->getInstrument()->baseInstrument(), outsptr2->getInstrument()->baseInstrument())
    TS_ASSERT_EQUALS(&(outsptr1->sample()), &(outsptr2->sample()))
    TS_ASSERT_DIFFERS(&(outsptr1->run()), &(outsptr2->run()))

    itr1 = wsNamevec.begin();
    for (; itr1 != wsNamevec.end(); ++itr1) {
      AnalysisDataService::Instance().remove(*itr);
    }
  }

  // test if parameters set in instrument definition file are loaded properly
  void testIfParameterFromIDFLoaded() {
    LoadRaw3 loader4;
    loader4.initialize();
    loader4.setPropertyValue("Filename", "TSC10076.raw");
    loader4.setPropertyValue("OutputWorkspace", "parameterIDF");
    TS_ASSERT_THROWS_NOTHING(loader4.execute())
    TS_ASSERT(loader4.isExecuted())

    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("parameterIDF"));

    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    const auto &detectorInfo = output2D->detectorInfo();
    const auto &detector = detectorInfo.detector(detectorInfo.indexOf(60));
    TS_ASSERT_EQUALS(detector.getID(), 60);

    const auto &pmap = output2D->constInstrumentParameters();
    TS_ASSERT_EQUALS(static_cast<int>(pmap.size()), 161);
    AnalysisDataService::Instance().remove("parameterIDF");
  }

  void testTwoTimeRegimes() {
    LoadRaw3 loader5;
    loader5.initialize();
    loader5.setPropertyValue("Filename", "IRS38633.raw");
    loader5.setPropertyValue("OutputWorkspace", "twoRegimes");
    loader5.setPropertyValue("SpectrumList", "2,3");
    loader5.setPropertyValue("LoadMonitors", "Include");
    TS_ASSERT_THROWS_NOTHING(loader5.execute())
    TS_ASSERT(loader5.isExecuted())

    MatrixWorkspace_const_sptr output;
    TS_ASSERT(output =
                  std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("twoRegimes")))
    // Shift should be 3300 - check a couple of values
    TS_ASSERT_EQUALS(output->readX(0).front() + 3300, output->readX(1).front())
    TS_ASSERT_EQUALS(output->readX(0).back() + 3300, output->readX(1).back())

    AnalysisDataService::Instance().remove("twoRegimes");
  }
  void testSeparateMonitors() {
    doTestSeparateMonitors("Separate");
    doTestSeparateMonitors("1");
  }

  void doTestSeparateMonitors(const std::string &option) {
    LoadRaw3 loader6;
    if (!loader6.isInitialized())
      loader6.initialize();

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(loader6.execute(), const std::runtime_error &);

    // Now set it...
    loader6.setPropertyValue("Filename", inputFile);
    loader6.setPropertyValue("LoadMonitors", option);

    outputSpace = "outer1";
    loader6.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(loader6.execute());
    TS_ASSERT(loader6.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    Workspace_sptr monitoroutput;
    TS_ASSERT_THROWS_NOTHING(monitoroutput = AnalysisDataService::Instance().retrieve(outputSpace + "_monitors"));
    Workspace2D_sptr monitoroutput2D = std::dynamic_pointer_cast<Workspace2D>(monitoroutput);
    // Should be 2584 for file HET15869.RAW
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 2580);

    TS_ASSERT_EQUALS(monitoroutput2D->getNumberHistograms(), 4);

    TS_ASSERT(monitoroutput2D->getSpectrum(0).hasDetectorID(601));
    TS_ASSERT(monitoroutput2D->getSpectrum(1).hasDetectorID(602));

    // Check two X vectors are the same
    TS_ASSERT((output2D->dataX(95)) == (output2D->dataX(1730)));
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->dataY(669).size(), output2D->dataY(2107).size());
    // Check one particular value
    TS_ASSERT_EQUALS(output2D->dataY(995)[0], 1);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->dataE(995)[777], 3);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->dataX(995)[777], 554.1875);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS(output2D->getAxis(0)->unit()->unitID(), "TOF")
    TS_ASSERT(!output2D->isDistribution())

    // Check the proton charge has been set correctly
    TS_ASSERT_DELTA(output2D->run().getProtonCharge(), 171.0353, 0.0001)

    // Test monitors attached:
    auto monitorsAttached = output2D->monitorWorkspace();
    TS_ASSERT(monitorsAttached);
    auto realMonWs = std::dynamic_pointer_cast<Workspace2D>(monitorsAttached);
    TS_ASSERT_EQUALS(realMonWs.get(), monitoroutput2D.get())
    //----------------------------------------------------------------------
    // Tests taken from LoadInstrumentTest to check Child Algorithm is running
    // properly
    //----------------------------------------------------------------------
    std::shared_ptr<const Instrument> i = output2D->getInstrument();
    std::shared_ptr<const Mantid::Geometry::IComponent> source = i->getSource();

    TS_ASSERT_EQUALS(source->getName(), "undulator");
    TS_ASSERT_DELTA(source->getPos().Y(), 0.0, 0.01);

    std::shared_ptr<const Mantid::Geometry::IComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS(samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA(samplepos->getPos().Z(), 0.0, 0.01);

    const auto &detectorInfo = output2D->detectorInfo();
    const auto detectorIndex = detectorInfo.indexOf(103);
    const auto &detector103 = detectorInfo.detector(detectorIndex);

    TS_ASSERT_EQUALS(detector103.getID(), 103);
    TS_ASSERT_EQUALS(detector103.getName(), "HET_non_PSDtube");
    TS_ASSERT_DELTA(detectorInfo.position(detectorIndex).X(), 0.4013, 0.01);
    TS_ASSERT_DELTA(detectorInfo.position(detectorIndex).Z(), 2.4470, 0.01);

    ////----------------------------------------------------------------------
    // Test code copied from LoadLogTest to check Child Algorithm is running
    // properly
    //----------------------------------------------------------------------
    Property *l_property = output2D->run().getLogData(std::string("TEMP1"));
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double> *>(l_property);
    std::string timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS(timeSeriesString.substr(0, 23), "2007-Nov-13 15:16:20  0");

    //----------------------------------------------------------------------
    // Tests to check that spectra-detector mapping is done correctly
    //----------------------------------------------------------------------
    // Test one to one mapping, for example spectra 6 has only 1 pixel
    TS_ASSERT_EQUALS(output2D->getSpectrum(6).getDetectorIDs().size(),
                     1); // rummap.ndet(6),1);

    // Test one to many mapping, for example 10 pixels contribute to spectra
    // 2084 (workspace index 2083)
    TS_ASSERT_EQUALS(output2D->getSpectrum(2079).getDetectorIDs().size(),
                     10); // map.ndet(2084),10);

    // Check the id number of all pixels contributing
    std::set<detid_t> detectorgroup;
    detectorgroup = output2D->getSpectrum(2079).getDetectorIDs();
    std::set<detid_t>::const_iterator it;
    int pixnum = 101191;
    for (it = detectorgroup.begin(); it != detectorgroup.end(); ++it)
      TS_ASSERT_EQUALS(*it, pixnum++);

    // Test if filename log is found in both monitor and sata workspace
    TS_ASSERT(output2D->run().hasProperty("raw_filename"));
    TS_ASSERT(monitoroutput2D->run().hasProperty("raw_filename"));
    TS_ASSERT_EQUALS(loader6.getPropertyValue("Filename"), output2D->run().getProperty("raw_filename")->value());
    TS_ASSERT_EQUALS(loader6.getPropertyValue("Filename"), monitoroutput2D->run().getProperty("raw_filename")->value());
    AnalysisDataService::Instance().remove(outputSpace);
    AnalysisDataService::Instance().remove(outputSpace + "_monitors");
  }

  void testSeparateMonitorsMultiPeriod() {
    LoadRaw3 loader7;
    loader7.initialize();
    loader7.setPropertyValue("Filename", "CSP79590.raw");
    loader7.setPropertyValue("OutputWorkspace", "multiperiod");
    loader7.setPropertyValue("LoadMonitors", "Separate");

    TS_ASSERT_THROWS_NOTHING(loader7.execute())
    TS_ASSERT(loader7.isExecuted())

    WorkspaceGroup_sptr work_out;
    TS_ASSERT_THROWS_NOTHING(work_out = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("multiperiod"));

    WorkspaceGroup_sptr monitor_work_out;
    TS_ASSERT_THROWS_NOTHING(monitor_work_out =
                                 AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("multiperiod_monitors"));

    Workspace_sptr monitorwsSptr = AnalysisDataService::Instance().retrieve("multiperiod_monitors");
    WorkspaceGroup_sptr monitorsptrWSGrp = std::dynamic_pointer_cast<WorkspaceGroup>(monitorwsSptr);

    const std::vector<std::string> monitorwsNamevec = monitorsptrWSGrp->getNames();
    int period = 1;
    std::vector<std::string>::const_iterator it = monitorwsNamevec.begin();
    for (; it != monitorwsNamevec.end(); ++it) {
      std::stringstream count;
      count << period;
      std::string wsName = "multiperiod_monitors_" + count.str();
      TS_ASSERT_EQUALS(*it, wsName)
      period++;
    }
    std::vector<std::string>::const_iterator itr1 = monitorwsNamevec.begin();
    for (; itr1 != monitorwsNamevec.end(); ++itr1) {
      MatrixWorkspace_sptr outsptr;
      TS_ASSERT_THROWS_NOTHING(outsptr = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>((*itr1)));
      TS_ASSERT_EQUALS(outsptr->getNumberHistograms(), 2)
    }
    std::vector<std::string>::const_iterator monitr = monitorwsNamevec.begin();
    MatrixWorkspace_sptr monoutsptr1;
    TS_ASSERT_THROWS_NOTHING(monoutsptr1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>((*monitr)));
    MatrixWorkspace_sptr monoutsptr2;
    TS_ASSERT_THROWS_NOTHING(monoutsptr2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>((*++monitr)));

    TS_ASSERT_EQUALS(monoutsptr1->dataX(0), monoutsptr2->dataX(0))

    // But the data should be different
    TS_ASSERT_DIFFERS(monoutsptr1->dataY(1)[555], monoutsptr2->dataY(1)[555])

    // Same number of logs
    const auto &monPeriod1Run = monoutsptr1->run();
    const auto &monPeriod2Run = monoutsptr2->run();
    TS_ASSERT_EQUALS(monPeriod1Run.getLogData().size(), monPeriod2Run.getLogData().size());
    TS_ASSERT(monPeriod1Run.hasProperty("period 1"))
    TS_ASSERT(monPeriod2Run.hasProperty("period 2"))

    Workspace_sptr wsSptr = AnalysisDataService::Instance().retrieve("multiperiod");
    WorkspaceGroup_sptr sptrWSGrp = std::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);

    const std::vector<std::string> wsNamevec = sptrWSGrp->getNames();
    period = 1;
    it = wsNamevec.begin();
    for (; it != wsNamevec.end(); ++it) {
      std::stringstream count;
      count << period;
      std::string wsName = "multiperiod_" + count.str();
      TS_ASSERT_EQUALS(*it, wsName)
      period++;
    }
    itr1 = wsNamevec.begin();
    int periodNumber = 0;
    const int nHistograms = 2;
    for (; itr1 != wsNamevec.end(); ++itr1) {
      MatrixWorkspace_sptr outsptr;
      TS_ASSERT_THROWS_NOTHING(outsptr = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>((*itr1)));
      doTestMultiPeriodWorkspace(outsptr, nHistograms, ++periodNumber);
    }
    std::vector<std::string>::const_iterator itr = wsNamevec.begin();
    MatrixWorkspace_sptr outsptr1;
    TS_ASSERT_THROWS_NOTHING(outsptr1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>((*itr)));
    MatrixWorkspace_sptr outsptr2;
    TS_ASSERT_THROWS_NOTHING(outsptr2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>((*++itr)));

    TS_ASSERT_EQUALS(outsptr1->dataX(0), outsptr2->dataX(0))
    TS_ASSERT_EQUALS(outsptr1->dataY(1)[555], outsptr2->dataY(1)[555])

    // But the data should be different
    TS_ASSERT_DIFFERS(&(outsptr1->run()), &(outsptr2->run()))

    it = monitorwsNamevec.begin();
    for (; it != monitorwsNamevec.end(); ++it) {
      AnalysisDataService::Instance().remove(*it);
    }
    it = wsNamevec.begin();
    for (; it != wsNamevec.end(); ++it) {
      AnalysisDataService::Instance().remove(*it);
    }
  }

  void testSeparateMonitorsFromMultiPeriodFileLimitingSpectraToOnlyMonitors() {
    LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "CSP79590.raw");
    std::string outputWSName = "outputname";
    loader.setPropertyValue("OutputWorkspace", outputWSName);
    loader.setPropertyValue("LoadMonitors", "Separate");
    loader.setPropertyValue("SpectrumList", "2");

    TS_ASSERT_THROWS_NOTHING(loader.execute())
    TS_ASSERT(loader.isExecuted())

    /// ADS should only contain single group with given name as the spectrum
    /// list contains only monitors
    AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
    TSM_ASSERT("Expected workspace is not in the ADS", ads.doesExist(outputWSName));
    TSM_ASSERT("A separate monitor workspace has been found when it should not be",
               !ads.doesExist(outputWSName + "_monitors"))

    // Check group is correct
    const size_t nperiods(2);
    WorkspaceGroup_sptr outputGroup = ads.retrieveWS<WorkspaceGroup>(outputWSName);
    TSM_ASSERT("Expected main workspace to be a group", outputGroup);
    TS_ASSERT_EQUALS(nperiods, outputGroup->size());

    for (size_t i = 1; i <= nperiods; ++i) {
      std::ostringstream wsname;
      wsname << outputWSName << "_" << i;
      std::ostringstream msg;
      msg << "Expected to find workspace '" << wsname.str() << "' in the ADS.";
      TSM_ASSERT(msg.str(), ads.doesExist(wsname.str()));
      msg.str("");
      msg << "Expected to find workspace '" << wsname.str() << "' as member of output group.";
      TSM_ASSERT(msg.str(), outputGroup->contains(wsname.str()));

      wsname.str("");
      wsname << outputWSName << "_monitors_" << i;
      msg.str("");
      msg << "Expected NOT to find workspace '" << wsname.str() << "' in the ADS.";
      TSM_ASSERT(msg.str(), !ads.doesExist(wsname.str()));
    }

    MatrixWorkspace_sptr output1 = ads.retrieveWS<MatrixWorkspace>(outputWSName + "_1");
    TS_ASSERT_EQUALS(1, output1->getNumberHistograms());

    TS_ASSERT_THROWS_NOTHING(output1->getSpectrum(0));
    auto &spectrum2 = output1->getSpectrum(0);
    TS_ASSERT_EQUALS(2, spectrum2.getSpectrumNo());

    const auto &detIDs = spectrum2.getDetectorIDs();
    TS_ASSERT_EQUALS(1, detIDs.size());
    TS_ASSERT(spectrum2.hasDetectorID(2));

    ads.remove(outputWSName);
  }

  // no monitors in the selected range
  void testSeparateMonitorswithMixedLimits() {
    LoadRaw3 loader9;
    if (!loader9.isInitialized())
      loader9.initialize();

    loader9.setPropertyValue("Filename", inputFile);
    loader9.setPropertyValue("OutputWorkspace", "outWS");
    loader9.setPropertyValue("SpectrumList", "998,999,1000");
    loader9.setPropertyValue("SpectrumMin", "5");
    loader9.setPropertyValue("SpectrumMax", "10");
    loader9.setPropertyValue("LoadMonitors", "Separate");

    TS_ASSERT_THROWS_NOTHING(loader9.execute());
    TS_ASSERT(loader9.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("outWS"));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    // Should be 6 for selected input
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 9);

    // Check two X vectors are the same
    TS_ASSERT((output2D->dataX(1)) == (output2D->dataX(5)));

    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->dataY(2).size(), output2D->dataY(7).size());

    // Check one particular value
    TS_ASSERT_EQUALS(output2D->dataY(8)[777], 9);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->dataE(8)[777], 3);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->dataX(8)[777], 554.1875);
    AnalysisDataService::Instance().remove("outWS");
  }

  // start and end spectra contains  monitors only
  void testSeparateMonitorswithMaxMinLimits1() {
    LoadRaw3 loader9;
    if (!loader9.isInitialized())
      loader9.initialize();

    loader9.setPropertyValue("Filename", inputFile);
    loader9.setPropertyValue("OutputWorkspace", "outWS");
    // loader9.setPropertyValue("SpectrumList", "998,999,1000");
    loader9.setPropertyValue("SpectrumMin", "2");
    loader9.setPropertyValue("SpectrumMax", "4");
    loader9.setPropertyValue("LoadMonitors", "Separate");

    TS_ASSERT_THROWS_NOTHING(loader9.execute());
    TS_ASSERT(loader9.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("outWS"));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    TS_ASSERT(output2D);
    if (!output2D)
      return;

    // Should be 3 for selected input
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 3);

    // Check two X vectors are the same
    // TS_ASSERT( (output2D->dataX(1)) == (output2D->dataX(5)) );

    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->dataY(1).size(), output2D->dataY(2).size());

    // Check one particular value
    TS_ASSERT_EQUALS(output2D->dataY(1)[1], 192);
    AnalysisDataService::Instance().remove("outWS");
  }

  // select start and end spectra a mix of monitors and normal workspace
  void testSeparateMonitorswithMaxMinimits2() {
    LoadRaw3 loader10;
    if (!loader10.isInitialized())
      loader10.initialize();

    loader10.setPropertyValue("Filename", inputFile);
    loader10.setPropertyValue("OutputWorkspace", "outWS");
    loader10.setPropertyValue("SpectrumMin", "2");
    loader10.setPropertyValue("SpectrumMax", "100");
    loader10.setPropertyValue("LoadMonitors", "Separate");

    TS_ASSERT_THROWS_NOTHING(loader10.execute());
    TS_ASSERT(loader10.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("outWS"));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    Workspace_sptr monitoroutput;
    TS_ASSERT_THROWS_NOTHING(monitoroutput = AnalysisDataService::Instance().retrieve("outWS_monitors"));
    Workspace2D_sptr monitoroutput2D = std::dynamic_pointer_cast<Workspace2D>(monitoroutput);

    // Should be 6 for selected input
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 96);

    TS_ASSERT_EQUALS(monitoroutput2D->getNumberHistograms(), 3);

    // Check two X vectors are the same
    TS_ASSERT((monitoroutput2D->dataX(1)) == (output2D->dataX(1)));

    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS(output2D->dataY(2).size(), output2D->dataY(3).size());
    AnalysisDataService::Instance().remove("outWS_monitors");
    AnalysisDataService::Instance().remove("outWS");

    // Check one particular value
  }
  // no monitors in the selected range
  void testSeparateMonitorswithMixedLimits3() {
    LoadRaw3 loader11;
    if (!loader11.isInitialized())
      loader11.initialize();

    loader11.setPropertyValue("Filename", inputFile);
    loader11.setPropertyValue("OutputWorkspace", "outWS");
    loader11.setPropertyValue("SpectrumList", "2,3,1000,1001,1002");
    loader11.setPropertyValue("SpectrumMin", "2");
    loader11.setPropertyValue("SpectrumMax", "100");
    loader11.setPropertyValue("LoadMonitors", "Separate");

    TS_ASSERT_THROWS_NOTHING(loader11.execute());
    TS_ASSERT(loader11.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("outWS"));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    Workspace_sptr monitoroutput;
    TS_ASSERT_THROWS_NOTHING(monitoroutput = AnalysisDataService::Instance().retrieve("outWS_monitors"));
    Workspace2D_sptr monitoroutput2D = std::dynamic_pointer_cast<Workspace2D>(monitoroutput);

    // Should be 6 for selected input
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 99);

    TS_ASSERT_EQUALS(monitoroutput2D->getNumberHistograms(), 3);

    AnalysisDataService::Instance().remove("outWS_monitors");
    AnalysisDataService::Instance().remove("outWS");
  }
  void testExcludeMonitors() {
    doTestExcludeMonitors("Exclude");
    doTestExcludeMonitors("0");
  }

  // no monitors in the selected range
  void doTestExcludeMonitors(const std::string &option) {
    LoadRaw3 loader11;
    if (!loader11.isInitialized())
      loader11.initialize();

    loader11.setPropertyValue("Filename", inputFile);
    loader11.setPropertyValue("OutputWorkspace", "outWS");
    loader11.setPropertyValue("LoadMonitors", option);

    TS_ASSERT_THROWS_NOTHING(loader11.execute());
    TS_ASSERT(loader11.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("outWS"));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 6 for selected input
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 2580);
    // Check one particular value
    TS_ASSERT_EQUALS(output2D->dataY(995)[777], 9);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->dataE(995)[777], 3);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS(output2D->dataX(995)[777], 554.1875);
    AnalysisDataService::Instance().remove("outWS");
  }

  void testExcludeMonitorswithMaxMinLimits() {
    LoadRaw3 loader11;
    if (!loader11.isInitialized())
      loader11.initialize();

    loader11.setPropertyValue("Filename", inputFile);
    loader11.setPropertyValue("OutputWorkspace", "outWS");
    loader11.setPropertyValue("SpectrumList", "2,3,1000,1001,1002");
    loader11.setPropertyValue("SpectrumMin", "2");
    loader11.setPropertyValue("SpectrumMax", "100");
    loader11.setPropertyValue("LoadMonitors", "Exclude");

    TS_ASSERT_THROWS_NOTHING(loader11.execute());
    TS_ASSERT(loader11.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve("outWS"));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 6 for selected input
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 99);
    AnalysisDataService::Instance().remove("outWS");
  }

  void testExecWithRawDatafile_s_type() {
    LoadRaw3 loader12;
    if (!loader12.isInitialized())
      loader12.initialize();

    // Now set it...
    loader12.setPropertyValue("Filename", "CSP74683.s02");
    loader12.setPropertyValue("LoadMonitors", "Include");

    outputSpace = "LoadLogTest-rawdatafile_so_type";
    loader12.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(loader12.execute());
    TS_ASSERT(loader12.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    // Obtain the expected log files which should be in the same directory as
    // the raw datafile
    Property *l_property = output2D->run().getLogData(std::string("ICPevent"));
    TimeSeriesProperty<std::string> *l_timeSeriesString = dynamic_cast<TimeSeriesProperty<std::string> *>(l_property);
    std::string timeSeriesString = l_timeSeriesString->value();
    TS_ASSERT_EQUALS(timeSeriesString.substr(0, 26), "2007-Oct-02 17:16:04   END");

    AnalysisDataService::Instance().remove(outputSpace);
  }

  void testLoadFromCombinedLogFile() {
    LoadRaw3 loader13;
    if (!loader13.isInitialized())
      loader13.initialize();

    // Now set it...
    loader13.setPropertyValue("Filename", "OFFSPEC00004622.raw");
    loader13.setPropertyValue("LoadMonitors", "Include");

    outputSpace = "ads_datafile";
    loader13.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(loader13.execute());
    TS_ASSERT(loader13.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    Property *l_property = output2D->run().getLogData(std::string("ICPevent"));
    TimeSeriesProperty<std::string> *l_timeSeriesString = dynamic_cast<TimeSeriesProperty<std::string> *>(l_property);
    std::string timeSeriesString = l_timeSeriesString->value();
    TS_ASSERT_EQUALS(timeSeriesString.substr(0, 36), "2009-Nov-11 11:25:57   CHANGE_PERIOD");

    Property *string_property = output2D->run().getLogData(std::string("RF1Ampon"));
    TimeSeriesProperty<std::string> *l_timeSeriesString1 =
        dynamic_cast<TimeSeriesProperty<std::string> *>(string_property);
    std::map<DateAndTime, std::string> vmap = l_timeSeriesString1->valueAsMap();
    std::map<DateAndTime, std::string>::const_iterator itr;
    for (itr = vmap.begin(); itr != vmap.end(); ++itr) {
      TS_ASSERT_EQUALS(itr->second, "False");
    }

    string_property = output2D->run().getLogData(std::string("ShutterStatus"));
    l_timeSeriesString1 = dynamic_cast<TimeSeriesProperty<std::string> *>(string_property);
    std::map<DateAndTime, std::string> vmap1 = l_timeSeriesString1->valueAsMap();
    for (itr = vmap1.begin(); itr != vmap1.end(); ++itr) {
      TS_ASSERT_EQUALS(itr->second, "OPEN");
    }

    Property *double_property = output2D->run().getLogData(std::string("b2v2"));
    TimeSeriesProperty<double> *l_timeSeriesDouble1 = dynamic_cast<TimeSeriesProperty<double> *>(double_property);
    std::map<DateAndTime, double> vmapb2v2 = l_timeSeriesDouble1->valueAsMap();
    std::map<DateAndTime, double>::const_iterator vmapb2v2itr;
    for (vmapb2v2itr = vmapb2v2.begin(); vmapb2v2itr != vmapb2v2.end(); ++vmapb2v2itr) {
      TS_ASSERT_EQUALS(vmapb2v2itr->second, -0.004);
    }

    AnalysisDataService::Instance().remove(outputSpace);
  }

  void test_loading_selected_periods() {
    LoadRaw3 loadAllPeriods;
    loadAllPeriods.initialize();
    loadAllPeriods.setProperty("Filename", "CSP78173.raw");
    loadAllPeriods.setProperty("OutputWorkspace", "allPeriods");
    loadAllPeriods.execute();
    auto allPeriods = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("allPeriods");
    TS_ASSERT_EQUALS(allPeriods->getNumberOfEntries(), 12);

    LoadRaw3 loadSelectedPeriods;
    loadSelectedPeriods.initialize();
    loadSelectedPeriods.setProperty("Filename", "CSP78173.raw");
    loadSelectedPeriods.setProperty("OutputWorkspace", "selectedPeriods");
    loadSelectedPeriods.setProperty("PeriodList", "1,3-5");
    loadSelectedPeriods.execute();
    auto selectedPeriods = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("selectedPeriods");
    TS_ASSERT_EQUALS(selectedPeriods->getNumberOfEntries(), 4);
    TS_ASSERT(AnalysisDataService::Instance().doesExist("selectedPeriods_1"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("selectedPeriods_3"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("selectedPeriods_4"));
    TS_ASSERT(AnalysisDataService::Instance().doesExist("selectedPeriods_5"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("selectedPeriods_2"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("selectedPeriods_6"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("selectedPeriods_7"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("selectedPeriods_8"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("selectedPeriods_9"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("selectedPeriods_10"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("selectedPeriods_11"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("selectedPeriods_12"));

    TS_ASSERT_EQUALS(checkWorkspacesMatch(allPeriods->getItem(0), selectedPeriods->getItem(0)), "");
    TS_ASSERT_EQUALS(checkWorkspacesMatch(allPeriods->getItem(2), selectedPeriods->getItem(1)), "");
    TS_ASSERT_EQUALS(checkWorkspacesMatch(allPeriods->getItem(3), selectedPeriods->getItem(2)), "");
    TS_ASSERT_EQUALS(checkWorkspacesMatch(allPeriods->getItem(4), selectedPeriods->getItem(3)), "");

    AnalysisDataService::Instance().clear();
  }

private:
  /// Helper method to run common set of tests on a workspace in a multi-period
  /// group.
  void doTestMultiPeriodWorkspace(const MatrixWorkspace_sptr &workspace, const size_t &nHistograms,
                                  int expected_period) {
    // Check the number of histograms.
    TS_ASSERT_EQUALS(workspace->getNumberHistograms(), nHistograms);
    // Check the current period property.
    const Mantid::API::Run &run = workspace->run();
    Property *prop = run.getLogData("current_period");
    PropertyWithValue<int> *current_period_property = dynamic_cast<PropertyWithValue<int> *>(prop);
    TS_ASSERT(current_period_property != nullptr);
    int actual_period = boost::lexical_cast<int>(current_period_property->value());
    TS_ASSERT_EQUALS(expected_period, actual_period);
    // Check the period n property.
    std::stringstream stream;
    stream << "period " << actual_period;
    TSM_ASSERT_THROWS_NOTHING("period number series could not be found.", run.getLogData(stream.str()));
  }

  /// Check that two matrix workspaces match
  std::string checkWorkspacesMatch(const Workspace_sptr &workspace1, const Workspace_sptr &workspace2) {
    auto ws1 = std::dynamic_pointer_cast<MatrixWorkspace>(workspace1);
    auto ws2 = std::dynamic_pointer_cast<MatrixWorkspace>(workspace2);
    if (!ws1 || !ws2) {
      return "At least one of the workspaces is not a MatrixWorkspace.";
    }
    if (ws1->getNumberHistograms() != ws2->getNumberHistograms()) {
      return "Workspaces have different numbers of histograms.";
    }
    if (ws1->blocksize() != ws2->blocksize()) {
      return "Workspaces have different numbers of bins.";
    }
    for (size_t i = 0; i < ws1->getNumberHistograms(); ++i) {
      auto &x1 = ws1->readX(i);
      auto &x2 = ws2->readX(i);
      if (!std::equal(x1.begin(), x1.end(), x2.begin())) {
        return "Mismatch in x-values.";
      }
      auto &y1 = ws1->readY(i);
      auto &y2 = ws2->readY(i);
      if (!std::equal(y1.begin(), y1.end(), y2.begin())) {
        return "Mismatch in y-values.";
      }
      auto &e1 = ws1->readE(i);
      auto &e2 = ws2->readE(i);
      if (!std::equal(e1.begin(), e1.end(), e2.begin())) {
        return "Mismatch in error values.";
      }
    }
    auto &ws1logs = ws1->run().getLogData();
    auto &ws2logs = ws2->run().getLogData();
    if (ws1logs.size() != ws2logs.size()) {
      return "Different numbers of logs";
    }
    return "";
  }

  LoadRaw3 loader, loader2, loader3;
  std::string inputFile;
  std::string outputSpace;
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadRaw3TestPerformance : public CxxTest::TestSuite {
public:
  void testDefaultLoad() {
    LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "HET15869.raw");
    loader.setPropertyValue("OutputWorkspace", "ws");
    TS_ASSERT(loader.execute());
  }
};
