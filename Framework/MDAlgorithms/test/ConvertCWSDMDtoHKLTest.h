#ifndef MANTID_MDALGORITHMS_CONVERTCWSDMDTOHKLTEST_H_
#define MANTID_MDALGORITHMS_CONVERTCWSDMDTOHKLTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/ConvertCWSDMDtoHKL.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/MDGeometry/HKL.h"

using Mantid::MDAlgorithms::ConvertCWSDMDtoHKL;

using namespace Mantid::API;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::DataObjects;
using Mantid::Geometry::Instrument_sptr;
using Mantid::Kernel::PropertyWithValue;

class ConvertCWSDMDtoHKLTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertCWSDMDtoHKLTest *createSuite() {
    return new ConvertCWSDMDtoHKLTest();
  }
  static void destroySuite(ConvertCWSDMDtoHKLTest *suite) { delete suite; }

  //-------------------------------------------------------------------------------
  void test_init() {
    createMDEW();

    ConvertCWSDMDtoHKL alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  //-------------------------------------------------------------------------------
  void test_convertToHKL() {
    ConvertCWSDMDtoHKL alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", m_qsampleWS->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "UBMatrix", "1.0, 0.5, 0., -0.2, 2.0, 0.4, 0., 1.11, 3.9"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "HKLMD"));

    alg.execute();
    TS_ASSERT(alg.isExecuted());

    IMDEventWorkspace_sptr hklws =
        boost::dynamic_pointer_cast<IMDEventWorkspace>(
            AnalysisDataService::Instance().retrieve("HKLMD"));
    TS_ASSERT(hklws);

    size_t numevents = hklws->getNEvents();
    TS_ASSERT_EQUALS(numevents, 100);

    TS_ASSERT_EQUALS(hklws->getSpecialCoordinateSystem(),
                     Mantid::Kernel::SpecialCoordinateSystem::HKL);
    // Test the frame type
    for (size_t dim = 0; dim < hklws->getNumDims(); ++dim) {
      const auto &frame = hklws->getDimension(dim)->getMDFrame();
      TSM_ASSERT_EQUALS("Should be convertible to a HKL frame",
                        Mantid::Geometry::HKL::HKLName, frame.name());
    }
  }

private:
  //-------------------------------------------------------------------------------
  /** Create the (blank) MDEW */
  void createMDEW() {
    // ---- Start with empty MDEW ----
    FrameworkManager::Instance().exec(
        "CreateMDWorkspace", 20, "Dimensions", "3", "EventType", "MDEvent",
        "Extents", "-10,10,-10,10,-10,10", "Names",
        "Q_sample_x,Q_sample_y,Q_sample_z", "Units",
        "Q_Sample_X,Q_Sample_Y,Q_Sample_Z", "Frames", "QSample,QSample,QSample",
        "SplitInto", "5", "SplitThreshold", "20", "MaxRecursionDepth", "15",
        "OutputWorkspace", "MDEWS");

    // Give it an instrument
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular2(1, 100, 0.05);

    TS_ASSERT_THROWS_NOTHING(
        m_qsampleWS =
            AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(
                "MDEWS"));
    ExperimentInfo_sptr ei(new ExperimentInfo());
    ei->setInstrument(inst);
    // Give it a run number
    ei->mutableRun().addProperty(
        new PropertyWithValue<std::string>("run_number", "12345"), true);
    m_qsampleWS->addExperimentInfo(ei);

    // Add events
    size_t num_events = 100;
    double peakcentre_x = 0.4;
    double peakcentre_y = -1.2;
    double peakcentre_z = -1.0;
    double radius = 0.5;

    // Add half of the events
    std::ostringstream mess;
    mess << num_events / 2 << ", " << peakcentre_x << ", " << peakcentre_y
         << ", " << peakcentre_z << ", " << radius;
    std::cout << mess.str() << "\n";
    FrameworkManager::Instance().exec("FakeMDEventData", 4, "InputWorkspace",
                                      "MDEWS", "PeakParams",
                                      mess.str().c_str());

    // Add a center with more events (half radius, half the total), to create a
    // "peak"
    std::ostringstream mess2;
    mess2 << num_events / 2 << ", " << peakcentre_x << ", " << peakcentre_y
          << ", " << peakcentre_z << ", " << radius * 0.25;
    std::cout << mess2.str() << "\n";
    FrameworkManager::Instance().exec("FakeMDEventData", 4, "InputWorkspace",
                                      "MDEWS", "PeakParams",
                                      mess2.str().c_str());

    // Check
    size_t numevents = m_qsampleWS->getNEvents();
    TS_ASSERT_EQUALS(numevents, 100);

    m_qsampleWS->setCoordinateSystem(
        Mantid::Kernel::SpecialCoordinateSystem::QSample);

    return;
  }

  IMDEventWorkspace_sptr m_qsampleWS;
};

#endif /* MANTID_MDALGORITHMS_CONVERTCWSDMDTOHKLTEST_H_ */
