#ifndef MANTID_CRYSTAL_PEAKCLUSTERPROJECTIONTEST_H_
#define MANTID_CRYSTAL_PEAKCLUSTERPROJECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/PeakClusterProjection.h"

#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"

#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/assign/list_of.hpp>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Crystal;
using namespace Mantid::MDEvents;
using namespace Mantid::DataObjects;

namespace
{
}

class PeakClusterProjectionTest: public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeakClusterProjectionTest *createSuite()
  {
    return new PeakClusterProjectionTest();
  }
  static void destroySuite(PeakClusterProjectionTest *suite)
  {
    delete suite;
  }

  PeakClusterProjectionTest()
  {
    FrameworkManager::Instance();
  }

  void test_throws_if_mdws_has_no_coordinate_system()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 1, 1);
    inWS->setCoordinateSystem(None);

    TSM_ASSERT_THROWS("Must have a known coordinate system", PeakClusterProjection object(inWS),
        std::invalid_argument&);
  }

  void test_labelAtPeakCenter_nan_if_is_off_image()
  {
    const double min = -10; // HKL
    const double max = 10; // HKL

    auto mdworkspaceAlg = AlgorithmManager::Instance().createUnmanaged("CreateMDHistoWorkspace");
    mdworkspaceAlg->setChild(true);
    mdworkspaceAlg->initialize();
    mdworkspaceAlg->setProperty("Dimensionality", 3);
    std::vector<int> numbersOfBins(3, 3);
    mdworkspaceAlg->setProperty("NumberOfBins", numbersOfBins);
    std::vector<double> extents =
        boost::assign::list_of(min)(max)(min)(max)(min)(max).convert_to_container<std::vector<double> >();
    mdworkspaceAlg->setProperty("Extents", extents);
    std::vector<double> signalValues(27, 1);
    mdworkspaceAlg->setProperty("SignalInput", signalValues);
    std::vector<double> errorValues(27, 1);
    mdworkspaceAlg->setProperty("ErrorInput", errorValues);
    mdworkspaceAlg->setPropertyValue("Names", "H,K,L");
    mdworkspaceAlg->setPropertyValue("Units", "-,-,-");
    mdworkspaceAlg->setPropertyValue("OutputWorkspace", "IntegratePeaksMDTest_MDEWS");
    mdworkspaceAlg->execute();
    IMDHistoWorkspace_sptr inWS = mdworkspaceAlg->getProperty("OutputWorkspace");

    // --- Set speical coordinates on fake mdworkspace --
    auto coordsAlg = AlgorithmManager::Instance().createUnmanaged("SetSpecialCoordinates");
    coordsAlg->setChild(true);
    coordsAlg->initialize();
    coordsAlg->setProperty("InputWorkspace", inWS);
    coordsAlg->setProperty("SpecialCoordinates", "HKL");
    coordsAlg->execute();

    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100, 0.05);
    PeaksWorkspace* pPeaksWS = new PeaksWorkspace();
    pPeaksWS->setCoordinateSystem(HKL);
    IPeaksWorkspace_sptr peakWS(pPeaksWS);
    peakWS->setInstrument(inst);

    Peak outOfBoundsPeak(inst, 15050, 1.0);
    outOfBoundsPeak.setHKL(1, 1, 11);
    peakWS->addPeak(outOfBoundsPeak);

    PeakClusterProjection projection(inWS);
    Mantid::signal_t value = projection.signalAtPeakCenter(outOfBoundsPeak);

    TSM_ASSERT("Should indicate is out of bounds via a NAN.", boost::math::isnan(value));
  }

};

#endif /* MANTID_CRYSTAL_PEAKCLUSTERPROJECTIONTEST_H_ */
