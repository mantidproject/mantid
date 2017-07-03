#ifndef MANTID_CRYSTAL_NormaliseVanadiumTEST_H_
#define MANTID_CRYSTAL_NormaliseVanadiumTEST_H_

#include "MantidAPI/Axis.h"
#include "MantidCrystal/NormaliseVanadium.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/FacilityHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <boost/random/linear_congruential.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <cxxtest/TestSuite.h>
#include <math.h>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::LinearGenerator;

namespace {
/** Create an EventWorkspace containing fake data of single-crystal diffraction.
*
* @return EventWorkspace_sptr
*/
EventWorkspace_sptr createDiffractionEventWorkspace(int numEvents) {
  FacilityHelper::ScopedFacilities loadTESTFacility(
      "IDFs_for_UNIT_TESTING/UnitTestFacilities.xml", "TEST");

  int numPixels = 10000;
  int numBins = 16;
  double binDelta = 0.10;
  boost::mt19937 rng;
  boost::uniform_real<double> u2(0, 1.0); // Random from 0 to 1.0
  boost::variate_generator<boost::mt19937 &, boost::uniform_real<double>>
      genUnit(rng, u2);
  int randomSeed = 1;
  rng.seed((unsigned int)(randomSeed));
  size_t nd = 1;
  // Make a random generator for each dimensions
  typedef boost::variate_generator<boost::mt19937 &,
                                   boost::uniform_real<double>> gen_t;
  gen_t *gens[1];
  for (size_t d = 0; d < nd; ++d) {
    double min = -1.;
    double max = 1.;
    if (max <= min)
      throw std::invalid_argument(
          "UniformParams: min must be < max for all dimensions.");
    boost::uniform_real<double> u(min, max); // Range
    gen_t *gen = new gen_t(rng, u);
    gens[d] = gen;
  }

  EventWorkspace_sptr retVal(new EventWorkspace);
  retVal->initialize(numPixels, 1, 1);

  // --------- Load the instrument -----------
  LoadInstrument *loadInst = new LoadInstrument();
  loadInst->initialize();
  loadInst->setPropertyValue("Filename",
                             "IDFs_for_UNIT_TESTING/MINITOPAZ_Definition.xml");
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", retVal);
  loadInst->setProperty("RewriteSpectraMap",
                        Mantid::Kernel::OptionalBool(true));
  loadInst->execute();
  delete loadInst;
  // Populate the instrument parameters in this workspace - this works around
  // a bug
  retVal->populateInstrumentParameters();

  DateAndTime run_start("2010-01-01T00:00:00");

  for (int pix = 0; pix < numPixels; pix++) {
    EventList &el = retVal->getSpectrum(pix);
    el.setSpectrumNo(pix);
    el.addDetectorID(pix);
    // Background
    for (int i = 0; i < numBins; i++) {
      // Two events per bin
      el += TofEvent((i + 0.5) * binDelta, run_start + double(i));
      el += TofEvent((i + 0.5) * binDelta, run_start + double(i));
    }

    // Peak
    int r = static_cast<int>(
        numEvents / std::sqrt((pix / 100 - 50.5) * (pix / 100 - 50.5) +
                              (pix % 100 - 50.5) * (pix % 100 - 50.5)));
    for (int i = 0; i < r; i++) {
      el += TofEvent(
          0.75 +
              binDelta *
                  (((*gens[0])() + (*gens[0])() + (*gens[0])()) * 2. - 3.),
          run_start + double(i));
    }
  }

  /// Clean up the generators
  for (size_t d = 0; d < nd; ++d)
    delete gens[d];

  // Set all the histograms at once.
  retVal->setAllX(BinEdges(numBins, LinearGenerator(0.0, binDelta)));

  // Some sanity checks
  TS_ASSERT_EQUALS(retVal->getInstrument()->getName(), "MINITOPAZ");
  std::map<int, Geometry::IDetector_const_sptr> dets;
  retVal->getInstrument()->getDetectors(dets);
  TS_ASSERT_EQUALS(dets.size(), 100 * 100);

  return retVal;
}

/** Creates an instance of the NormaliseVanadium algorithm and sets its
* properties.
*
* @return NormaliseVanadium alg
*/
IAlgorithm_sptr createAlgorithm() {
  int numEventsPer = 100;
  MatrixWorkspace_sptr inputW = createDiffractionEventWorkspace(numEventsPer);
  inputW->getAxis(0)->setUnit("Wavelength");

  IAlgorithm_sptr alg = boost::make_shared<NormaliseVanadium>();
  TS_ASSERT_THROWS_NOTHING(alg->initialize())
  TS_ASSERT(alg->isInitialized())
  alg->setProperty("InputWorkspace", inputW);
  alg->setProperty("OutputWorkspace", "TOPAZ");
  alg->setProperty("Wavelength", 1.0);

  return alg;
}
}

class NormaliseVanadiumImpl : public NormaliseVanadium {
public:
  void exec() override { NormaliseVanadium::exec(); };
};

class NormaliseVanadiumTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    NormaliseVanadium alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void do_test_MINITOPAZ() {

    IAlgorithm_sptr alg = createAlgorithm();
    TS_ASSERT_THROWS_NOTHING(alg->execute();)
    TS_ASSERT(alg->isExecuted())

    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "TOPAZ"));
    TS_ASSERT(ws);
    if (!ws)
      return;
    TS_ASSERT_DELTA(ws->y(5050)[5], 7.7142, 0.01);
    AnalysisDataService::Instance().remove("TOPAZ");
  }

  void test_MINITOPAZ() {
    for (int i = 0; i < 1; i++)
      do_test_MINITOPAZ();
  }
};

class NormaliseVanadiumTestPerformance : public CxxTest::TestSuite {
public:
  static NormaliseVanadiumTestPerformance *createSuite() {
    return new NormaliseVanadiumTestPerformance();
  }
  static void destroySuite(NormaliseVanadiumTestPerformance *suite) {
    delete suite;
  }

  void setUp() override { normaliseVanadiumAlg = createAlgorithm(); }

  void tearDown() override { AnalysisDataService::Instance().remove("TOPAZ"); }

  void testNormaliseVanadiumPerformance() {
    TS_ASSERT_THROWS_NOTHING(normaliseVanadiumAlg->execute());
  }

private:
  IAlgorithm_sptr normaliseVanadiumAlg;
};

#endif /* MANTID_CRYSTAL_NormaliseVanadiumTEST_H_ */
