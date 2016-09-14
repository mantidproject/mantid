#ifndef CORRECTKIKFTEST_H_
#define CORRECTKIKFTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/CorrectKiKf.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <MantidHistogramData/LinearGenerator.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;

namespace {
void createWorkspace2D(const std::string &inputWSname, bool isHistogram,
                       const int nHist = 1, const int nBins = 5) {

  double h = isHistogram ? 0.5 : 0;

  Workspace2D_sptr ws2D(new Workspace2D);
  ws2D->initialize(nHist, isHistogram ? nBins + 1 : nBins, nBins);
  ws2D->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");

  Mantid::MantidVec xv;
  if (isHistogram) {
    xv.resize(nBins + 1, 0.0);
  } else {
    xv.resize(nBins, 0.0);
  }
  for (int i = 0; i < nBins; ++i) {
    xv[i] = static_cast<double>((i - 2. - h) * 5.);
  }
  if (isHistogram) {
    xv[nBins] = static_cast<double>((nBins - 2.5) * 5.);
  }

  Counts counts(ws2D->y(0).size(), LinearGenerator(1, 1));

  std::vector<double> e(nBins);
  std::transform(counts.cbegin(), counts.cend(), e.begin(), [&](double y)
  {
	  return sqrt(y);
  });

  auto cow_xv = make_cow<HistogramX>(std::move(xv));
  for (int i = 0; i < nHist; i++) {
    ws2D->setSharedX(i, cow_xv);
	ws2D->setCounts(i, counts);
	ws2D->mutableE(i) = e;
    ws2D->getSpectrum(i).setSpectrumNo(i);
  }

  AnalysisDataService::Instance().add(inputWSname, ws2D);
}

void createEventWorkspace(const std::string &inputEvWSname,
                          const int nPixels = 1, const int nBins = 5,
                          const int nEvents = 5) {
  EventWorkspace_sptr event = WorkspaceCreationHelper::CreateEventWorkspace(
      nPixels, nBins, nEvents, 0, 0.9, 2, 0);
  event->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");
  AnalysisDataService::Instance().add(inputEvWSname, event);
}
}

class CorrectKiKfTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CorrectKiKfTest *createSuite() { return new CorrectKiKfTest(); }
  static void destroySuite(CorrectKiKfTest *suite) { delete suite; }

  CorrectKiKfTest()
      : inputWSname("testInput"), inputEvWSname("testEvInput"),
        outputWSname("testOutput"), outputEvWSname("testEvOutput") {}

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void testExec() {
    bool isHistogram = true;

    // check direct histogram
    createWorkspace2D(inputWSname, isHistogram);
    alg.setPropertyValue("InputWorkspace", inputWSname);
    alg.setPropertyValue("OutputWorkspace", outputWSname);
    alg.setPropertyValue("EMode", "Direct");
    alg.setPropertyValue("EFixed", "7.5");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    Workspace2D_sptr result =
        AnalysisDataService::Instance().retrieveWS<Workspace2D>(outputWSname);
    double ei, ef, factor, deltaE, stdval;
    for (size_t i = 0; i < result->blocksize(); ++i) {
      ei = 7.5;
      deltaE = (static_cast<double>(i) - 2.) * 5.;
      ef = ei - deltaE;
      stdval = static_cast<double>(i) + 1.;
      if (ei * ef < 0) {
        factor = 0.;
      } else {
        factor = std::sqrt(ei / ef);
      }
      TS_ASSERT_DELTA(factor, (result->y(0)[i]) / (stdval), 1e-8);
      TS_ASSERT_DELTA(factor, (result->e(0)[i]) / std::sqrt(stdval), 1e-8);
    }
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);

    // check direct not histogram
    isHistogram = false;
    createWorkspace2D(inputWSname, isHistogram);
    alg.setPropertyValue("InputWorkspace", inputWSname);
    alg.setPropertyValue("OutputWorkspace", outputWSname);
    alg.setPropertyValue("EMode", "Direct");
    alg.setPropertyValue("EFixed", "7.5");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    result =
        AnalysisDataService::Instance().retrieveWS<Workspace2D>(outputWSname);
    for (size_t i = 0; i < result->blocksize(); ++i) {
      ei = 7.5;
      deltaE = (static_cast<double>(i) - 2.) * 5.;
      ef = ei - deltaE;
      stdval = static_cast<double>(i) + 1.;
      if (ei * ef < 0) {
        factor = 0.;
      } else {
        factor = std::sqrt(ei / ef);
      }
      TS_ASSERT_DELTA(factor, (result->y(0)[i]) / (stdval), 1e-8);
      TS_ASSERT_DELTA(factor, (result->e(0)[i]) / std::sqrt(stdval), 1e-8);
    }
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);

    // check indirect not histogram
    isHistogram = false;
    createWorkspace2D(inputWSname, isHistogram);
    alg.setPropertyValue("InputWorkspace", inputWSname);
    alg.setPropertyValue("OutputWorkspace", outputWSname);
    alg.setPropertyValue("EMode", "Indirect");
    alg.setPropertyValue("EFixed", "7.5");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    result =
        AnalysisDataService::Instance().retrieveWS<Workspace2D>(outputWSname);
    for (size_t i = 0; i < result->blocksize(); ++i) {
      ef = 7.5;
      deltaE = (static_cast<double>(i) - 2.) * 5.;
      ei = ef + deltaE;
      stdval = static_cast<double>(i) + 1.;
      if (ei * ef < 0) {
        factor = 0.;
      } else {
        factor = std::sqrt(ei / ef);
      }
      TS_ASSERT_DELTA(factor, (result->y(0)[i]) / (stdval), 1e-8);
      TS_ASSERT_DELTA(factor, (result->e(0)[i]) / std::sqrt(stdval), 1e-8);
    }
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);

    // check indirect histogram
    isHistogram = true;
    createWorkspace2D(inputWSname, isHistogram);
    alg.setPropertyValue("InputWorkspace", inputWSname);
    alg.setPropertyValue("OutputWorkspace", outputWSname);
    alg.setPropertyValue("EMode", "Indirect");
    alg.setPropertyValue("EFixed", "7.5");
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    result =
        AnalysisDataService::Instance().retrieveWS<Workspace2D>(outputWSname);
    for (size_t i = 0; i < result->blocksize(); ++i) {
      ef = 7.5;
      deltaE = (static_cast<double>(i) - 2.) * 5.;
      ei = ef + deltaE;
      stdval = static_cast<double>(i) + 1.;
      if (ei * ef < 0) {
        factor = 0.;
      } else {
        factor = std::sqrt(ei / ef);
      }
      TS_ASSERT_DELTA(factor, (result->y(0)[i]) / (stdval), 1e-8);
      TS_ASSERT_DELTA(factor, (result->e(0)[i]) / std::sqrt(stdval), 1e-8);
    }
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);
  }

  void testEventCorrection() {

    createEventWorkspace(inputEvWSname);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("InputWorkspace", inputEvWSname);
    alg.setPropertyValue("OutputWorkspace", outputEvWSname);
    alg.setPropertyValue("EMode", "Direct");
    alg.setPropertyValue("EFixed", "3.");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    EventWorkspace_sptr in_ws, out_ws;
    TS_ASSERT_THROWS_NOTHING(
        in_ws = boost::dynamic_pointer_cast<EventWorkspace>(
            AnalysisDataService::Instance().retrieve(inputEvWSname)));
    TS_ASSERT_THROWS_NOTHING(
        out_ws = boost::dynamic_pointer_cast<EventWorkspace>(
            AnalysisDataService::Instance().retrieve(outputEvWSname)));

    TS_ASSERT(out_ws);
    if (!out_ws)
      return;

    TS_ASSERT_DELTA(
        out_ws->getSpectrum(0).getEvent(0).weight(),
        std::sqrt(3. / (3. - out_ws->getSpectrum(0).getEvent(0).tof())), 1e-7);
    TS_ASSERT_DELTA(
        out_ws->getSpectrum(0).getEvent(3).weight(),
        std::sqrt(3. / (3. - out_ws->getSpectrum(0).getEvent(3).tof())), 1e-7);
    TS_ASSERT_LESS_THAN(
        out_ws->getNumberEvents(),
        in_ws->getNumberEvents()); // Check that events with Ef<0 are dropped

    AnalysisDataService::Instance().remove(outputEvWSname);
    AnalysisDataService::Instance().remove(inputEvWSname);
  }

  void testReadEffromIDF() {
    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "IRS38633.raw");
    const std::string initialWS("IRS");
    const std::string intermediaryWS("IRSenergy");
    const std::string finalWS("Corrected");
    loader.setPropertyValue("OutputWorkspace", initialWS);
    loader.setPropertyValue("SpectrumList", "3");
    loader.setPropertyValue("LoadMonitors", "Exclude");
    loader.execute();

    Mantid::Algorithms::ConvertUnits convert;
    convert.initialize();
    convert.setPropertyValue("InputWorkspace", initialWS);
    convert.setPropertyValue("OutputWorkspace", intermediaryWS);
    convert.setPropertyValue("Target", "DeltaE");
    convert.setPropertyValue("EMode", "Indirect");
    convert.setPropertyValue("EFixed", "1.845");
    convert.execute();

    CorrectKiKf alg1; // I use alg1 because I cannot remove Efixed property
    alg1.initialize();

    TS_ASSERT_THROWS_NOTHING(
        alg1.setPropertyValue("InputWorkspace", intermediaryWS));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("OutputWorkspace", finalWS));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("EMode", "Indirect"));
    TS_ASSERT_THROWS_NOTHING(alg1.setPropertyValue("EFixed", "1.845"));
    TS_ASSERT_THROWS_NOTHING(alg1.execute());
    TS_ASSERT(alg1.isExecuted());

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = boost::dynamic_pointer_cast<Workspace2D>(
            AnalysisDataService::Instance().retrieve(finalWS)));

    TS_ASSERT_DELTA(result->x(0)[1976], 1.18785, 0.0001);
    TS_ASSERT_DELTA(result->x(0)[1977], 1.18912, 0.0001);
    TS_ASSERT_DELTA(result->y(0)[1976], 1.28225, 0.0001);

    // Ef=1.845, Ei=Ef+0.5*(x[1977]+x[1976]), Y [1976] uncorrected=1,
    // ki/kf=sqrt(Ei/Ef)
    TS_ASSERT_DELTA(
        sqrt(((result->x(0)[1976] + result->x(0)[1977]) * 0.5 + 1.845) / 1.845),
        result->y(0)[1976], 0.0001);

    AnalysisDataService::Instance().remove(initialWS);
    AnalysisDataService::Instance().remove(intermediaryWS);
    AnalysisDataService::Instance().remove(finalWS);
  }

private:
  CorrectKiKf alg;
  std::string inputWSname;
  std::string inputEvWSname;
  std::string outputWSname;
  std::string outputEvWSname;
};

class CorrectKiKfTestPerformance : public CxxTest::TestSuite {
public:
	// This pair of boilerplate methods prevent the suite being created statically
	// This means the constructor isn't called when running other tests
	static CorrectKiKfTestPerformance *createSuite() { return new CorrectKiKfTestPerformance(); }
	static void destroySuite(CorrectKiKfTestPerformance *suite) { delete suite; }

	CorrectKiKfTestPerformance() {
		bool isHistogram = true;
		createWorkspace2D(histogramInputWS, isHistogram, 2000, 750);
		isHistogram = false;
		createWorkspace2D(notHistogramInputWS, isHistogram, 2000, 750);
		createEventWorkspace(eventInputWS, 250, 1000, 1000);
	}

	~CorrectKiKfTestPerformance() {
		AnalysisDataService::Instance().remove(histogramInputWS);
		AnalysisDataService::Instance().remove(histogramOutputWS);

		AnalysisDataService::Instance().remove(notHistogramInputWS);
		AnalysisDataService::Instance().remove(notHistogramOutputWS);

		AnalysisDataService::Instance().remove(eventInputWS);
		AnalysisDataService::Instance().remove(eventOutputWS);
	}

	void testExecHistogram() {
		CorrectKiKf alg;
		alg.initialize();
		alg.setPropertyValue("InputWorkspace", histogramInputWS);
		alg.setPropertyValue("OutputWorkspace", histogramOutputWS);
		alg.setPropertyValue("EMode", "Direct");
		alg.setPropertyValue("EFixed", "7.5");
		alg.execute();
		TS_ASSERT(alg.isExecuted());
	}

	void testExecNotHistogram() {
		CorrectKiKf alg;
		alg.initialize();
		alg.setPropertyValue("InputWorkspace", notHistogramInputWS);
		alg.setPropertyValue("OutputWorkspace", notHistogramOutputWS);
		alg.setPropertyValue("EMode", "Direct");
		alg.setPropertyValue("EFixed", "7.5");
		alg.execute();
		TS_ASSERT(alg.isExecuted());
	}

	void testExecEventWorkspace() {
		CorrectKiKf alg;
		alg.initialize();
		alg.setPropertyValue("InputWorkspace", eventInputWS);
		alg.setPropertyValue("OutputWorkspace", eventOutputWS);
		alg.setPropertyValue("EMode", "Direct");
		alg.setPropertyValue("EFixed", "3.");
		TS_ASSERT_THROWS_NOTHING(alg.execute());
	}
private:
	const std::string histogramInputWS = "histogramInputWS";
	const std::string histogramOutputWS = "histogramOutputWS";

	const std::string notHistogramInputWS = "notHistogramInputWS";
	const std::string notHistogramOutputWS = "notHistogramOutputWS";

	const std::string eventInputWS = "eventInputWS";
	const std::string eventOutputWS = "eventOutputWS";

};
#endif /*CorrectKiKfTEST_H_*/
