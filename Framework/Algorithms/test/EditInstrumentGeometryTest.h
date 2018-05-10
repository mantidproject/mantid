#ifndef MANTID_ALGORITHMS_EDITTOFPOWDERDIFFRACTOMERGEOMETRYTEST_H_
#define MANTID_ALGORITHMS_EDITTOFPOWDERDIFFRACTOMERGEOMETRYTEST_H_

#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/EditInstrumentGeometry.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class EditInstrumentGeometryTest : public CxxTest::TestSuite {
public:
  /** Test algorithm initialization
   */
  void test_Initialize() {

    EditInstrumentGeometry editdetector;
    TS_ASSERT_THROWS_NOTHING(editdetector.initialize());
    TS_ASSERT(editdetector.isInitialized());
  }

  /** Test for a workspace containing a single spectrum
   */
  void test_SingleSpectrum() {
    // 1. Init
    EditInstrumentGeometry editdetector;
    editdetector.initialize();

    // 2. Load Data
    DataObjects::Workspace2D_sptr workspace2d =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 100,
                                                                     false);
    API::AnalysisDataService::Instance().add("inputWS", workspace2d);

    // 3. Set Property
    TS_ASSERT_THROWS_NOTHING(
        editdetector.setPropertyValue("Workspace", "inputWS"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("SpectrumIDs", "1"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("L2", "3.45"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue("Polar", "90.09"));
    TS_ASSERT_THROWS_NOTHING(
        editdetector.setPropertyValue("Azimuthal", "1.84"));

    // 4. Run
    TS_ASSERT_THROWS_NOTHING(editdetector.execute());
    TS_ASSERT(editdetector.isExecuted());

    // 5. Check result
    Mantid::API::MatrixWorkspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(
        workspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve("inputWS")));

    const auto &spectrumInfo = workspace->spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.hasUniqueDetector(0), true);

    double r, tth, phi;
    spectrumInfo.position(0).getSpherical(r, tth, phi);
    TS_ASSERT_DELTA(r, 3.45, 0.000001);
    TS_ASSERT_DELTA(tth, 90.09, 0.000001);
    TS_ASSERT_DELTA(phi, 1.84, 0.000001);
  }

  /** Unit test to edit instrument parameters of all spectrums (>1)
   */
  void test_MultipleWholeSpectrumEdit() {
    // 1. Init
    EditInstrumentGeometry editdetector;
    editdetector.initialize();

    // 2. Load Data
    DataObjects::Workspace2D_sptr workspace2d =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(6, 100,
                                                                     false);
    API::AnalysisDataService::Instance().add("inputWS2", workspace2d);

    // 3. Set Property
    TS_ASSERT_THROWS_NOTHING(
        editdetector.setPropertyValue("Workspace", "inputWS2"));
    // TS_ASSERT_THROWS_NOTHING(
    // editdetector.setPropertyValue("SpectrumIDs","3072,19456,40960,55296,74752,93184")
    // );
    TS_ASSERT_THROWS_NOTHING(
        editdetector.setPropertyValue("L2", "1.1,2.2,3.3,4.4,5.5,6.6"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue(
        "Polar", "90.1,90.2,90.3,90.4,90.5,90.6"));
    TS_ASSERT_THROWS_NOTHING(
        editdetector.setPropertyValue("Azimuthal", "1,2,3,4,5,6"));

    // 4. Run
    TS_ASSERT_THROWS_NOTHING(editdetector.execute());
    TS_ASSERT(editdetector.isExecuted());

    // 5. Check result
    Mantid::API::MatrixWorkspace_sptr workspace;
    TS_ASSERT_THROWS_NOTHING(
        workspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve("inputWS2")));

    checkDetectorParameters(workspace, 0, 1.1, 90.1, 1.0);
    checkDetectorParameters(workspace, 1, 2.2, 90.2, 2.0);
    checkDetectorParameters(workspace, 3, 4.4, 90.4, 4.0);
    checkDetectorParameters(workspace, 5, 6.6, 90.6, 6.0);
  }

  //----------------------------------------------------------------------------------------------
  /** Unit test to edit instrument parameters of all spectrums (>1) and using
   * new detector IDs
   */
  void test_MultiplePartialSpectrumEdit() {
    // Init
    EditInstrumentGeometry editdetector;
    editdetector.initialize();

    // Load Data
    DataObjects::Workspace2D_sptr workspace2d =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(6, 100,
                                                                     false);
    API::AnalysisDataService::Instance().addOrReplace("inputWS3", workspace2d);

    // Set Property
    TS_ASSERT_THROWS_NOTHING(
        editdetector.setPropertyValue("Workspace", "inputWS3"));
    TS_ASSERT_THROWS_NOTHING(
        editdetector.setPropertyValue("SpectrumIDs", "1,2,3,4,5,6"));
    TS_ASSERT_THROWS_NOTHING(
        editdetector.setPropertyValue("L2", "1.1,2.2,3.3, 4.4, 5.5, 6.6"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue(
        "Polar", "90.1,90.2,90.3, 90.4, 90.5, 90.6"));
    TS_ASSERT_THROWS_NOTHING(
        editdetector.setPropertyValue("Azimuthal", "1,2,3,4,5,6"));
    TS_ASSERT_THROWS_NOTHING(editdetector.setPropertyValue(
        "DetectorIDs", "200, 201, 300, 301, 400, 401"));

    // Run
    editdetector.execute();
    TS_ASSERT(editdetector.isExecuted());

    // Check
    checkDetectorID(workspace2d, 0, 200);
    checkDetectorID(workspace2d, 1, 201);
    checkDetectorID(workspace2d, 2, 300);
    checkDetectorID(workspace2d, 3, 301);
    checkDetectorID(workspace2d, 4, 400);
    checkDetectorID(workspace2d, 5, 401);

    // Clean
    AnalysisDataService::Instance().remove("inputWS3");
  }

  //----------------------------------------------------------------------------------------------
  /** Check detector parameter
   */
  void checkDetectorParameters(API::MatrixWorkspace_sptr workspace,
                               size_t wsindex, double realr, double realtth,
                               double realphi) {

    const auto &spectrumInfo = workspace->spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.hasUniqueDetector(wsindex), true);

    double r, tth, phi;
    spectrumInfo.position(wsindex).getSpherical(r, tth, phi);
    TS_ASSERT_DELTA(r, realr, 0.000001);
    TS_ASSERT_DELTA(tth, realtth, 0.000001);
    TS_ASSERT_DELTA(phi, realphi, 0.000001);
  }

  //----------------------------------------------------------------------------------------------
  /** Check detector parameter
   */
  void checkDetectorID(API::MatrixWorkspace_sptr workspace, size_t wsindex,
                       detid_t detid) {

    const auto &spectrumInfo = workspace->spectrumInfo();
    TS_ASSERT_EQUALS(spectrumInfo.hasUniqueDetector(wsindex), true);
    TS_ASSERT_EQUALS(spectrumInfo.detector(wsindex).getID(), detid);
  }
};

#endif /* MANTID_ALGORITHMS_EDITTOFPOWDERDIFFRACTOMERGEOMETRYTEST_H_ */
