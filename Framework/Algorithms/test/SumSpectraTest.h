#ifndef SUMSPECTRATEST_H_
#define SUMSPECTRATEST_H_

#include "MantidAlgorithms/SumSpectra.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <boost/lexical_cast.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class SumSpectraTest : public CxxTest::TestSuite {
public:
  static SumSpectraTest *createSuite() { return new SumSpectraTest(); }
  static void destroySuite(SumSpectraTest *suite) { delete suite; }

  SumSpectraTest() {
    this->nTestHist = 10;
    this->inputSpace =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nTestHist,
                                                                     102, true);
    this->inputSpace->instrumentParameters().addBool(
        inputSpace->getDetector(1).get(), "masked", true);
  }

  ~SumSpectraTest() { AnalysisDataService::Instance().clear(); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void testExecWithLimits() {
    if (!alg.isInitialized())
      alg.initialize();

    // Set the properties
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputSpace));
    const std::string outputSpace1 = "SumSpectraOut1";
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outputSpace1));

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("StartWorkspaceIndex", "1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("EndWorkspaceIndex", "3"));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve(outputSpace1));

    Workspace2D_const_sptr output2D =
        boost::dynamic_pointer_cast<const Workspace2D>(output);
    size_t max = 0;
    TS_ASSERT_EQUALS(max = inputSpace->blocksize(), output2D->blocksize());
    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 1);

    const Mantid::MantidVec &x = output2D->readX(0);
    const Mantid::MantidVec &y = output2D->readY(0);
    const Mantid::MantidVec &e = output2D->readE(0);
    TS_ASSERT_EQUALS(x.size(), 103);
    TS_ASSERT_EQUALS(y.size(), 102);
    TS_ASSERT_EQUALS(e.size(), 102);

    for (size_t i = 0; i < max; ++i) {
      TS_ASSERT_EQUALS(x[i], inputSpace->readX(0)[i]);
      TS_ASSERT_EQUALS(y[i], inputSpace->readY(2)[i] + inputSpace->readY(3)[i]);
      TS_ASSERT_DELTA(
          e[i], std::sqrt(inputSpace->readY(2)[i] + inputSpace->readY(3)[i]),
          1.0e-10);
    }

    // Check the detectors mapped to the single spectra
    const ISpectrum *spec = output2D->getSpectrum(0);
    TS_ASSERT_EQUALS(spec->getSpectrumNo(), 2);
    TS_ASSERT_EQUALS(spec->getDetectorIDs().size(), 2);
    TS_ASSERT(spec->hasDetectorID(3));
    TS_ASSERT(spec->hasDetectorID(4));

    TS_ASSERT(output2D->run().hasProperty("NumAllSpectra"))
    TS_ASSERT(output2D->run().hasProperty("NumMaskSpectra"))
    TS_ASSERT(output2D->run().hasProperty("NumZeroSpectra"))

    TS_ASSERT_EQUALS(boost::lexical_cast<std::string>(3 - 1),
                     output2D->run().getLogData("NumAllSpectra")->value())
    TS_ASSERT_EQUALS(boost::lexical_cast<std::string>(1),
                     output2D->run().getLogData("NumMaskSpectra")->value())
    TS_ASSERT_EQUALS(boost::lexical_cast<std::string>(0),
                     output2D->run().getLogData("NumZeroSpectra")->value())
  }

  void testExecWithoutLimits() {
    Mantid::Algorithms::SumSpectra alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    // Set the properties
    alg2.setProperty("InputWorkspace", inputSpace);
    const std::string outputSpace2 = "SumSpectraOut2";
    alg2.setPropertyValue("OutputWorkspace", outputSpace2);
    alg2.setProperty("IncludeMonitors", false);

    // Check setting of invalid property value causes failure
    TS_ASSERT_THROWS(alg2.setPropertyValue("StartWorkspaceIndex", "-1"),
                     std::invalid_argument);

    TS_ASSERT_THROWS_NOTHING(alg2.execute());
    TS_ASSERT(alg2.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve(outputSpace2));
    Workspace2D_const_sptr output2D =
        boost::dynamic_pointer_cast<const Workspace2D>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 1);

    const Mantid::MantidVec &x = output2D->readX(0);
    const Mantid::MantidVec &y = output2D->readY(0);
    const Mantid::MantidVec &e = output2D->readE(0);
    TS_ASSERT_EQUALS(x.size(), 103);
    TS_ASSERT_EQUALS(y.size(), 102);
    TS_ASSERT_EQUALS(e.size(), 102);

    // Check a few bins
    TS_ASSERT_EQUALS(x[0], inputSpace->readX(0)[0]);
    TS_ASSERT_EQUALS(x[50], inputSpace->readX(0)[50]);
    TS_ASSERT_EQUALS(x[100], inputSpace->readX(0)[100]);
    TS_ASSERT_EQUALS(y[7], 14);
    TS_ASSERT_EQUALS(y[38], 14);
    TS_ASSERT_EQUALS(y[72], 14);
    TS_ASSERT_DELTA(e[28], std::sqrt(y[28]), 0.00001);
    TS_ASSERT_DELTA(e[47], std::sqrt(y[47]), 0.00001);
    TS_ASSERT_DELTA(e[99], std::sqrt(y[99]), 0.00001);

    // Check the detectors mapped to the single spectra
    const ISpectrum *spec = output2D->getSpectrum(0);
    TS_ASSERT_EQUALS(spec->getSpectrumNo(), 1);
    // Spectra at workspace index 1 is masked, 8 & 9 are monitors
    TS_ASSERT_EQUALS(spec->getDetectorIDs().size(), 7);
    TS_ASSERT(spec->hasDetectorID(1));
    TS_ASSERT(spec->hasDetectorID(3));
    TS_ASSERT(spec->hasDetectorID(4));
    TS_ASSERT(spec->hasDetectorID(5));
    TS_ASSERT(spec->hasDetectorID(6));
    TS_ASSERT(spec->hasDetectorID(7));

    TS_ASSERT(output2D->run().hasProperty("NumAllSpectra"))
    TS_ASSERT(output2D->run().hasProperty("NumMaskSpectra"))
    TS_ASSERT(output2D->run().hasProperty("NumZeroSpectra"))

    // Spectra at workspace index 1 is masked, 8 & 9 are monitors
    TS_ASSERT_EQUALS(boost::lexical_cast<std::string>(nTestHist - 3),
                     output2D->run().getLogData("NumAllSpectra")->value())
    TS_ASSERT_EQUALS(boost::lexical_cast<std::string>(1),
                     output2D->run().getLogData("NumMaskSpectra")->value())
    TS_ASSERT_EQUALS(boost::lexical_cast<std::string>(0),
                     output2D->run().getLogData("NumZeroSpectra")->value())
  }

  void testExecEvent_inplace() {
    dotestExecEvent("testEvent", "testEvent", "5,10-15");
  }

  void testExecEvent_copy() {
    dotestExecEvent("testEvent", "testEvent2", "5,10-15");
  }

  void testExecEvent_going_too_far() {
    dotestExecEvent("testEvent", "testEvent2", "5,10-15, 500-600");
  }

  void dotestExecEvent(std::string inName, std::string outName,
                       std::string indices_list) {
    int numPixels = 100;
    int numBins = 20;
    int numEvents = 20;
    EventWorkspace_sptr input = WorkspaceCreationHelper::CreateEventWorkspace(
        numPixels, numBins, numEvents);
    AnalysisDataService::Instance().addOrReplace(inName, input);

    Mantid::Algorithms::SumSpectra alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    // Set the properties
    alg2.setPropertyValue("InputWorkspace", inName);
    alg2.setPropertyValue("OutputWorkspace", outName);
    alg2.setProperty("IncludeMonitors", false);
    alg2.setPropertyValue("ListOfWorkspaceIndices", indices_list);
    alg2.setPropertyValue("StartWorkspaceIndex", "4");
    alg2.setPropertyValue("EndWorkspaceIndex", "6");

    alg2.execute();
    TS_ASSERT(alg2.isExecuted());

    EventWorkspace_sptr output;
    output =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outName);
    TS_ASSERT(output);
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(output->getNumberEvents(), 9 * numEvents);
    TS_ASSERT_EQUALS(input->readX(0).size(), output->readX(0).size());

    TS_ASSERT(output->run().hasProperty("NumAllSpectra"))
    TS_ASSERT(output->run().hasProperty("NumMaskSpectra"))
    TS_ASSERT(output->run().hasProperty("NumZeroSpectra"))
  }

  void testRebinnedOutputSum() {
    AnalysisDataService::Instance().clear();
    RebinnedOutput_sptr ws =
        WorkspaceCreationHelper::CreateRebinnedOutputWorkspace();
    std::string inName = "rebinTest";
    std::string outName = "rebin_sum";

    AnalysisDataService::Instance().addOrReplace(inName, ws);
    size_t nHist = ws->getNumberHistograms();

    // Start with a clean algorithm
    Mantid::Algorithms::SumSpectra alg3;
    if (!alg3.isInitialized()) {
      alg3.initialize();
    }
    // Set the properties
    alg3.setPropertyValue("InputWorkspace", inName);
    alg3.setPropertyValue("OutputWorkspace", outName);
    alg3.setProperty("IncludeMonitors", false);
    alg3.execute();
    TS_ASSERT(alg3.isExecuted());

    // Check that things came out correctly
    RebinnedOutput_sptr output;
    output =
        AnalysisDataService::Instance().retrieveWS<RebinnedOutput>(outName);
    TS_ASSERT(output);
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(output->blocksize(), 6);
    // Row with full acceptance
    TS_ASSERT_EQUALS(output->dataY(0)[1], 1.);
    TS_ASSERT_DELTA(output->dataE(0)[1], 0.40824829046386296, 1.e-5);
    TS_ASSERT_EQUALS(output->dataF(0)[1], 6.);
    // Row with limited, but non-zero acceptance, shouldn't have nans!
    TS_ASSERT_DELTA(output->dataY(0)[5], 0.66666, 1.e-5);
    TS_ASSERT_DELTA(output->dataE(0)[5], 0.47140452079103173, 1.e-5);
    TS_ASSERT_EQUALS(output->dataF(0)[5], 3.);

    TS_ASSERT(output->run().hasProperty("NumAllSpectra"))
    TS_ASSERT(output->run().hasProperty("NumMaskSpectra"))
    TS_ASSERT(output->run().hasProperty("NumZeroSpectra"))

    TS_ASSERT_EQUALS(boost::lexical_cast<std::string>(nHist),
                     output->run().getLogData("NumAllSpectra")->value())
    TS_ASSERT_EQUALS(boost::lexical_cast<std::string>(0),
                     output->run().getLogData("NumMaskSpectra")->value())
    TS_ASSERT_EQUALS(boost::lexical_cast<std::string>(0),
                     output->run().getLogData("NumZeroSpectra")->value())
  }
  void testExecNoLimitsWeighted() {
    Mantid::Algorithms::SumSpectra alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    const Mantid::MantidVec &y0 = inputSpace->readY(0);
    const Mantid::MantidVec &e0 = inputSpace->readE(0);
    // Set the properties
    alg2.setProperty("InputWorkspace", inputSpace);
    const std::string outputSpace2 = "SumSpectraOut2";
    alg2.setPropertyValue("OutputWorkspace", outputSpace2);
    alg2.setProperty("IncludeMonitors", false);
    alg2.setProperty("WeightedSum", true);

    TS_ASSERT_THROWS_NOTHING(alg2.execute());
    TS_ASSERT(alg2.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve(outputSpace2));
    Workspace2D_const_sptr output2D =
        boost::dynamic_pointer_cast<const Workspace2D>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 1);

    const Mantid::MantidVec &x = output2D->readX(0);
    const Mantid::MantidVec &y = output2D->readY(0);
    const Mantid::MantidVec &e = output2D->readE(0);
    TS_ASSERT_EQUALS(x.size(), 103);
    TS_ASSERT_EQUALS(y.size(), 102);
    TS_ASSERT_EQUALS(e.size(), 102);

    TS_ASSERT(output2D->run().hasProperty("NumAllSpectra"))
    TS_ASSERT(output2D->run().hasProperty("NumMaskSpectra"))
    TS_ASSERT(output2D->run().hasProperty("NumZeroSpectra"))

    // Spectra at workspace index 1 is masked, 8 & 9 are monitors
    TS_ASSERT_EQUALS(boost::lexical_cast<std::string>(nTestHist - 3),
                     output2D->run().getLogData("NumAllSpectra")->value())
    TS_ASSERT_EQUALS(boost::lexical_cast<std::string>(1),
                     output2D->run().getLogData("NumMaskSpectra")->value())
    TS_ASSERT_EQUALS(boost::lexical_cast<std::string>(0),
                     output2D->run().getLogData("NumZeroSpectra")->value())

    size_t nSignals = nTestHist - 3;
    // Check a few bins
    TS_ASSERT_EQUALS(x[0], inputSpace->readX(0)[0]);
    TS_ASSERT_EQUALS(x[50], inputSpace->readX(0)[50]);
    TS_ASSERT_EQUALS(x[100], inputSpace->readX(0)[100]);
    TS_ASSERT_DELTA(y[7], double(nSignals) * y0[7], 1.e-6);
    TS_ASSERT_DELTA(y[38], double(nSignals) * y0[38], 1.e-6);
    TS_ASSERT_DELTA(y[72], double(nSignals) * y0[72], 1.e-6);
    TS_ASSERT_DELTA(e[28], std::sqrt(double(nSignals)) * e0[28], 0.00001);
    TS_ASSERT_DELTA(e[47], std::sqrt(double(nSignals)) * e0[47], 0.00001);
    TS_ASSERT_DELTA(e[99], std::sqrt(double(nSignals)) * e0[99], 0.00001);

    // Check the detectors mapped to the single spectra
    const ISpectrum *spec = output2D->getSpectrum(0);
    TS_ASSERT_EQUALS(spec->getSpectrumNo(), 1);
    // Spectra at workspace index 1 is masked, 8 & 9 are monitors
    TS_ASSERT_EQUALS(spec->getDetectorIDs().size(), 7);
    TS_ASSERT(spec->hasDetectorID(1));
    TS_ASSERT(spec->hasDetectorID(3));
    TS_ASSERT(spec->hasDetectorID(4));
    TS_ASSERT(spec->hasDetectorID(5));
    TS_ASSERT(spec->hasDetectorID(6));
    TS_ASSERT(spec->hasDetectorID(7));
  }

  void testExecNoLimitsSpecialWeighted() {
    Mantid::Algorithms::SumSpectra alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    int nBins = 10;
    int nHist = 4;

    MatrixWorkspace_sptr tws =
        WorkspaceCreationHelper::Create2DWorkspaceBinned(nHist, nBins);
    std::string inName = "rebinTest";
    std::string outName = "sumWS";

    AnalysisDataService::Instance().addOrReplace(inName, tws);

    std::vector<double> testVal(4, 0);
    double testRez(0), testSig(0), sum(0), weightSum(0);
    testVal[0] = 2;
    testVal[1] = 3;
    testVal[2] = 1;
    testVal[3] = 10;
    for (int i = 0; i < nHist; i++) {
      Mantid::MantidVec &y0 = tws->dataY(i);
      Mantid::MantidVec &e0 = tws->dataE(i);
      for (int j = 0; j < nBins; j++) {
        y0[j] = testVal[i];
        e0[j] = std::sqrt(testVal[i]);
      }
      sum += 1.;
      weightSum += 1 / testVal[i];
      testSig += testVal[i];
    }
    testRez = nHist * sum / weightSum;
    testSig = std::sqrt(testSig);
    // Set the properties
    alg2.setProperty("InputWorkspace", tws);
    const std::string outputSpace2 = "SumSpectraOut2";
    alg2.setPropertyValue("OutputWorkspace", outName);
    alg2.setProperty("IncludeMonitors", false);
    alg2.setProperty("WeightedSum", true);

    TS_ASSERT_THROWS_NOTHING(alg2.execute());
    TS_ASSERT(alg2.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve(outName));
    Workspace2D_const_sptr output2D =
        boost::dynamic_pointer_cast<const Workspace2D>(output);

    TS_ASSERT_EQUALS(output2D->getNumberHistograms(), 1);

    const Mantid::MantidVec &x = output2D->readX(0);
    const Mantid::MantidVec &y = output2D->readY(0);
    const Mantid::MantidVec &e = output2D->readE(0);
    TS_ASSERT_EQUALS(x.size(), nBins + 1);
    TS_ASSERT_EQUALS(y.size(), nBins);
    TS_ASSERT_EQUALS(e.size(), nBins);

    TS_ASSERT(output2D->run().hasProperty("NumAllSpectra"))
    TS_ASSERT(output2D->run().hasProperty("NumMaskSpectra"))
    TS_ASSERT(output2D->run().hasProperty("NumZeroSpectra"))

    TS_ASSERT_EQUALS(boost::lexical_cast<std::string>(nHist),
                     output2D->run().getLogData("NumAllSpectra")->value())
    TS_ASSERT_EQUALS(boost::lexical_cast<std::string>(0),
                     output2D->run().getLogData("NumMaskSpectra")->value())
    TS_ASSERT_EQUALS(boost::lexical_cast<std::string>(0),
                     output2D->run().getLogData("NumZeroSpectra")->value())

    // Check a few bins
    TS_ASSERT_EQUALS(x[0], tws->readX(0)[0]);
    TS_ASSERT_EQUALS(x[5], tws->readX(0)[5]);
    TS_ASSERT_EQUALS(x[10], tws->readX(0)[10]);
    TS_ASSERT_DELTA(y[0], testRez, 1.e-6);
    TS_ASSERT_DELTA(y[5], testRez, 1.e-6);
    TS_ASSERT_DELTA(y[9], testRez, 1.e-6);
    TS_ASSERT_DELTA(e[0], testSig, 1.e-6);
    TS_ASSERT_DELTA(e[4], testSig, 1.e-6);
    TS_ASSERT_DELTA(e[9], testSig, 1.e-6);

    AnalysisDataService::Instance().remove(inName);
    AnalysisDataService::Instance().remove(outName);
  }

private:
  int nTestHist;
  Mantid::Algorithms::SumSpectra alg; // Test with range limits
  MatrixWorkspace_sptr inputSpace;
};

#endif /*SUMSPECTRATEST_H_*/
