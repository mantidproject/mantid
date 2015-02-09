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
#include <math.h>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Crystal;
using namespace Mantid::MDEvents;
using namespace Mantid::DataObjects;

class PeakClusterProjectionTest: public CxxTest::TestSuite
{

private:

  // Helper function to create a peaks workspace.
  IPeaksWorkspace_sptr create_peaks_WS(Instrument_sptr inst) const
  {
    PeaksWorkspace* pPeaksWS = new PeaksWorkspace();
    pPeaksWS->setCoordinateSystem(Mantid::Kernel::HKL);
    IPeaksWorkspace_sptr peakWS(pPeaksWS);
    peakWS->setInstrument(inst);
    return peakWS;
  }

  // Helper function to create a MD Image workspace of labels.
  IMDHistoWorkspace_sptr create_HKL_MDWS(double min = -10, double max = 10, int numberOfBins = 3,
      double signalValue = 1, double errorValue = 1) const
  {
    const int dimensionality = 3;
    int totalBins = 1;
    for (int i = 0; i < dimensionality; ++i)
    {
      totalBins *= numberOfBins;
    }
    auto mdworkspaceAlg = AlgorithmManager::Instance().createUnmanaged("CreateMDHistoWorkspace");
    mdworkspaceAlg->setChild(true);
    mdworkspaceAlg->initialize();
    mdworkspaceAlg->setProperty("Dimensionality", dimensionality);
    std::vector<int> numbersOfBins(dimensionality, numberOfBins);
    mdworkspaceAlg->setProperty("NumberOfBins", numbersOfBins);
    std::vector<double> extents =
        boost::assign::list_of(min)(max)(min)(max)(min)(max).convert_to_container<std::vector<double> >();
    mdworkspaceAlg->setProperty("Extents", extents);
    std::vector<double> signalValues(totalBins, signalValue);
    mdworkspaceAlg->setProperty("SignalInput", signalValues);
    std::vector<double> errorValues(totalBins, errorValue);
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
    return inWS;
  }

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
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 3, 1);
    inWS->setCoordinateSystem(Mantid::Kernel::None);

    TSM_ASSERT_THROWS("Must have a known coordinate system", PeakClusterProjection object(inWS),
        std::invalid_argument&);
  }

  void test_throws_if_mdws_is_less_than_three_dimensional()
  {
    IMDHistoWorkspace_sptr inWS = MDEventsTestHelper::makeFakeMDHistoWorkspace(1, 2, 1);
    inWS->setCoordinateSystem(Mantid::Kernel::HKL);

    TSM_ASSERT_THROWS("Must be +3 dimensional", PeakClusterProjection object(inWS),
        std::invalid_argument&);
  }

  void test_labelAtPeakCenter_nan_if_is_off_image()
  {
    const double min = -10; // HKL
    const double max = 10; // HKL

    auto inWS = create_HKL_MDWS(min, max);

    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100, 0.05);
    IPeaksWorkspace_sptr peakWS = create_peaks_WS(inst);

    Peak outOfBoundsPeak(inst, 15050, 1.0);
    outOfBoundsPeak.setHKL(1, 1, 11); // Off the edge because L is too large.
    peakWS->addPeak(outOfBoundsPeak);

    PeakClusterProjection projection(inWS);
    Mantid::signal_t value = projection.signalAtPeakCenter(outOfBoundsPeak);

    TSM_ASSERT("Should indicate is out of bounds via a NAN.", boost::math::isnan(value));
  }

  void test_labelAtPeakCenter_with_peak_at_0_0_0()
  {
    const double min = -10; // HKL
    const double max = 10; // HKL
    const int nBins = 5;

    auto inWS = create_HKL_MDWS(min, max, nBins);
    const double labelValue = 4;
    inWS->setSignalAt(static_cast<int>(nBins * nBins * nBins / 2), labelValue); // Set label at 0, 0, 0

    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100, 0.05);
    IPeaksWorkspace_sptr peakWS = create_peaks_WS(inst);

    Peak peak(inst, 15050, 1.0);
    peak.setHKL(0, 0, 0); // At 0, 0, 0
    peakWS->addPeak(peak);

    PeakClusterProjection projection(inWS);
    Mantid::signal_t value = projection.signalAtPeakCenter(peak);
    TS_ASSERT_EQUALS(labelValue, value);
  }

  void test_labelAtPeakCenter_with_peak_at_almost_10_10_10()
  {
    const double min = -10; // HKL
    const double max = 10; // HKL
    const int nBins = 5;

    auto inWS = create_HKL_MDWS(min, max, nBins);
    const double labelValue = 4;
    const int index = static_cast<int>((nBins * nBins * nBins) - 1);
    inWS->setSignalAt(index, labelValue); // Set label at 10, 10, 10

    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100, 0.05);
    IPeaksWorkspace_sptr peakWS = create_peaks_WS(inst);

    Peak peak(inst, 15050, 1.0);
    peak.setHKL(9.999, 9.999, 9.999); // At almost 10, 10, 10
    peakWS->addPeak(peak);

    PeakClusterProjection projection(inWS);
    Mantid::signal_t value = projection.signalAtPeakCenter(peak);
    TS_ASSERT_EQUALS(labelValue, value);
  }

  void test_labelAtPeakCenter_with_peak_at_exactly_10_10_10()
  {
    const double min = -10; // HKL
    const double max = 10; // HKL
    const int nBins = 5;

    auto inWS = create_HKL_MDWS(min, max, nBins);
    const double labelValue = 4;
    const int index = static_cast<int>((nBins * nBins * nBins) - 1);
    inWS->setSignalAt(index, labelValue); // Set label at 10, 10, 10

    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100, 0.05);
    IPeaksWorkspace_sptr peakWS = create_peaks_WS(inst);
    Peak outOfBoundsPeak(inst, 15050, 1.0);
    outOfBoundsPeak.setHKL(10, 10, 10); // At exactly 10, 10, 10 (offlimits!!!)
    peakWS->addPeak(outOfBoundsPeak);

    PeakClusterProjection projection(inWS);
    Mantid::signal_t value = projection.signalAtPeakCenter(outOfBoundsPeak);
    TS_ASSERT(boost::math::isnan(value));
  }

};

#endif /* MANTID_CRYSTAL_PEAKCLUSTERPROJECTIONTEST_H_ */
