#ifndef ALGORITHMS_TEST_REFLECTOMETRYREDUCTIONONETEST_H_
#define ALGORITHMS_TEST_REFLECTOMETRYREDUCTIONONETEST_H_

#include <cxxtest/TestSuite.h>
#include <algorithm>
#include "MantidAlgorithms/ReflectometryReductionOne.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Unit.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::Geometry;
using namespace WorkspaceCreationHelper;

class ReflectometryReductionOneTest : public CxxTest::TestSuite {
private:
  MatrixWorkspace_sptr m_tinyReflWS;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryReductionOneTest *createSuite() {
    return new ReflectometryReductionOneTest();
  }
  static void destroySuite(ReflectometryReductionOneTest *suite) {
    delete suite;
  }

  ReflectometryReductionOneTest() {
    FrameworkManager::Instance();
    m_tinyReflWS = create2DWorkspaceWithReflectometryInstrument();
  }

  void test_tolam() {
    MatrixWorkspace_sptr toConvert = m_tinyReflWS;
    std::vector<int> detectorIndexRange;
    size_t workspaceIndexToKeep1 = 1;
    const int monitorIndex = 0;

    specnum_t specId1 =
        toConvert->getSpectrum(workspaceIndexToKeep1).getSpectrumNo();
    specnum_t monitorSpecId =
        toConvert->getSpectrum(monitorIndex).getSpectrumNo();

    // Define one spectra to keep
    detectorIndexRange.push_back(static_cast<int>(workspaceIndexToKeep1));
    std::stringstream buffer;
    buffer << workspaceIndexToKeep1;
    const std::string detectorIndexRangesStr = buffer.str();

    // Define a wavelength range for the detector workspace
    const double wavelengthMin = 1.0;
    const double wavelengthMax = 15;
    const double backgroundWavelengthMin = 17;
    const double backgroundWavelengthMax = 20;

    ReflectometryReductionOne alg;

    // Run the conversion.
    ReflectometryWorkflowBase::DetectorMonitorWorkspacePair inLam =
        alg.toLam(toConvert, detectorIndexRangesStr, monitorIndex,
                  boost::tuple<double, double>(wavelengthMin, wavelengthMax),
                  boost::tuple<double, double>(backgroundWavelengthMin,
                                               backgroundWavelengthMax));

    // Unpack the results
    MatrixWorkspace_sptr detectorWS = inLam.get<0>();
    MatrixWorkspace_sptr monitorWS = inLam.get<1>();

    /* ---------- Checks for the detector workspace ------------------*/

    // Check units.
    TS_ASSERT_EQUALS("Wavelength", detectorWS->getAxis(0)->unit()->unitID());

    // Check the number of spectrum kept.
    TS_ASSERT_EQUALS(1, detectorWS->getNumberHistograms());

    auto map = detectorWS->getSpectrumToWorkspaceIndexMap();
    // Check the spectrum Nos retained.
    TS_ASSERT_EQUALS(map[specId1], 0);

    // Check the cropped x range
    auto copyX = detectorWS->x(0);
    std::sort(copyX.begin(), copyX.end());
    TS_ASSERT(copyX.front() >= wavelengthMin);
    TS_ASSERT(copyX.back() <= wavelengthMax);

    /* ------------- Checks for the monitor workspace --------------------*/
    // Check units.
    TS_ASSERT_EQUALS("Wavelength", monitorWS->getAxis(0)->unit()->unitID());

    // Check the number of spectrum kept. This should only ever be 1.
    TS_ASSERT_EQUALS(1, monitorWS->getNumberHistograms());

    map = monitorWS->getSpectrumToWorkspaceIndexMap();
    // Check the spectrum Nos retained.
    TS_ASSERT_EQUALS(map[monitorSpecId], 0);
  }

  IAlgorithm_sptr construct_standard_algorithm() {
    auto alg =
        AlgorithmManager::Instance().create("ReflectometryReductionOne", 1);
    alg->setRethrows(true);
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", m_tinyReflWS);
    alg->setProperty("WavelengthMin", 1.0);
    alg->setProperty("WavelengthMax", 2.0);
    alg->setProperty("I0MonitorIndex", 1);
    alg->setProperty("MonitorBackgroundWavelengthMin", 1.0);
    alg->setProperty("MonitorBackgroundWavelengthMax", 2.0);
    alg->setProperty("MonitorIntegrationWavelengthMin", 1.2);
    alg->setProperty("MonitorIntegrationWavelengthMax", 1.5);
    alg->setProperty("MomentumTransferStep", 0.1);
    alg->setPropertyValue("ProcessingInstructions", "0");
    alg->setPropertyValue("OutputWorkspace", "x");
    alg->setPropertyValue("OutputWorkspaceWavelength", "y");
    alg->setRethrows(true);
    return alg;
  }

  void test_execute() {
    auto alg = construct_standard_algorithm();
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    MatrixWorkspace_sptr workspaceInQ = alg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr workspaceInLam =
        alg->getProperty("OutputWorkspaceWavelength");
    const double theta = alg->getProperty("ThetaOut");
    UNUSED_ARG(theta)
    UNUSED_ARG(workspaceInQ)
    UNUSED_ARG(workspaceInLam)
  }

  void test_calculate_theta() {

    auto alg = construct_standard_algorithm();

    alg->execute();
    // Should not throw

    const double outTwoTheta = alg->getProperty("ThetaOut");
    TS_ASSERT_DELTA(45.0, outTwoTheta, 0.00001);
  }

  void test_source_rotation_after_second_reduction() {
    // set up the axis for the instrument
    Instrument_sptr instrument = boost::make_shared<Instrument>();
    instrument->setReferenceFrame(boost::make_shared<ReferenceFrame>(
        Y /*up*/, Z /*along*/, Right, "0,0,0"));

    // add a source
    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0, 0, -1));
    instrument->add(source);
    instrument->markAsSource(source);

    // add a sample
    ObjComponent *sample = new ObjComponent("some-surface-holder");
    sample->setPos(V3D(0, 0, 0));
    instrument->add(sample);
    instrument->markAsSamplePos(sample);

    // add a detector
    Detector *det = new Detector("point-detector", 1, NULL);
    det->setPos(V3D(0, 1, 1));
    instrument->add(det);
    instrument->markAsDetector(det);

    // set the instrument to this workspace
    m_tinyReflWS->setInstrument(instrument);
    // set this detector ready for processing instructions
    m_tinyReflWS->getSpectrum(0).setDetectorID(det->getID());

    auto alg =
        AlgorithmManager::Instance().create("ReflectometryReductionOne", 1);
    alg->setRethrows(true);
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", m_tinyReflWS);
    alg->setProperty("WavelengthMin", 1.0);
    alg->setProperty("WavelengthMax", 15.0);
    alg->setProperty("I0MonitorIndex", 0);
    alg->setProperty("MonitorBackgroundWavelengthMin", 0.0);
    alg->setProperty("MonitorBackgroundWavelengthMax", 0.0);
    alg->setProperty("MonitorIntegrationWavelengthMin", 0.0);
    alg->setProperty("MonitorIntegrationWavelengthMax", 0.0);
    alg->setProperty("MomentumTransferStep", 0.1);
    alg->setProperty("NormalizeByIntegratedMonitors", false);
    alg->setProperty("CorrectDetectorPositions", true);
    alg->setProperty("CorrectionAlgorithm", "None");
    alg->setPropertyValue("ProcessingInstructions", "1");
    alg->setPropertyValue("OutputWorkspace", "x");
    alg->setPropertyValue("OutputWorkspaceWavelength", "y");
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    MatrixWorkspace_sptr outLam = alg->getProperty("OutputWorkspaceWavelength");
    MatrixWorkspace_sptr outQ = alg->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(m_tinyReflWS->getInstrument()->getSource()->getPos(),
                     outLam->getInstrument()->getSource()->getPos());
    TS_ASSERT_EQUALS(outLam->getInstrument()->getSource()->getPos(),
                     outQ->getInstrument()->getSource()->getPos());
  }
  void test_post_processing_scale_step() {
    auto alg = construct_standard_algorithm();
    auto inWS =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
            2.0);
    inWS->getAxis(0)->setUnit("Wavelength");
    alg->setProperty("InputWorkspace", inWS);
    alg->setProperty("ScaleFactor", 1.0);
    alg->setProperty("ThetaIn", 1.5);
    alg->setProperty("OutputWorkspace", "Test");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    MatrixWorkspace_sptr nonScaledWS = alg->getProperty("OutputWorkspace");
    alg->setProperty("InputWorkspace", inWS);
    alg->setProperty("ScaleFactor", 0.5);
    alg->setProperty("OutputWorkspace", "scaledTest");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    MatrixWorkspace_sptr scaledWS = alg->getProperty("OutputWorkspace");
    // compare y data instead of workspaces.
    auto &scaledYData = scaledWS->y(0);
    auto &nonScaledYData = nonScaledWS->y(0);
    TS_ASSERT_EQUALS(scaledYData.front(), 2 * nonScaledYData.front());
    TS_ASSERT_EQUALS(scaledYData[scaledYData.size() / 2],
                     2 * nonScaledYData[nonScaledYData.size() / 2]);
    TS_ASSERT_EQUALS(scaledYData.back(), 2 * nonScaledYData.back());
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove("Test");
    AnalysisDataService::Instance().remove("scaledTest");
  }
  void test_post_processing_rebin_step_with_params_not_provided() {
    auto alg = construct_standard_algorithm();
    auto inWS = create2DWorkspace154(1, 10, true);
    // this instrument does not have a "slit-gap" property
    // defined in the IPF, so CalculateResolution should throw.
    inWS->setInstrument(m_tinyReflWS->getInstrument());
    inWS->getAxis(0)->setUnit("Wavelength");
    // Setup bad bin edges, Rebin will throw (not CalculateResolution?)
    inWS->mutableX(0) = inWS->x(0)[0];
    alg->setProperty("InputWorkspace", inWS);
    alg->setProperty("OutputWorkspace", "rebinnedWS");
    TS_ASSERT_THROWS(alg->execute(), std::invalid_argument);
  }
  void test_post_processing_rebin_step_with_partial_params_provided() {
    auto alg = construct_standard_algorithm();
    auto inWS = create2DWorkspace154(1, 10, true);
    inWS->setInstrument(m_tinyReflWS->getInstrument());
    inWS->getAxis(0)->setUnit("Wavelength");
    alg->setProperty("InputWorkspace", inWS);
    alg->setProperty("MomentumTransferMaximum", 15.0);
    alg->setProperty("OutputWorkspace", "rebinnedWS");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    MatrixWorkspace_sptr rebinnedIvsQWS = alg->getProperty("OutputWorkspace");
    auto &xData = rebinnedIvsQWS->x(0);
    // based off the equation for logarithmic binning X(i+1)=X(i)(1+|dX|)
    double binWidthFromLogarithmicEquation = fabs((xData[1] / xData[0]) - 1);
    TSM_ASSERT_DELTA("DQQ should be the same as abs(x[1]/x[0] - 1)",
                     binWidthFromLogarithmicEquation, 0.1, 1e-06);
    TSM_ASSERT_DELTA("Qmax should be the same as last Params entry (5.0)",
                     xData.back(), 15.0, 1e-06);
  }
  void test_post_processing_rebin_step_with_logarithmic_rebinning() {
    auto alg = construct_standard_algorithm();
    auto inWS = create2DWorkspace154(1, 10, true);
    inWS->setInstrument(m_tinyReflWS->getInstrument());
    inWS->getAxis(0)->setUnit("Wavelength");
    alg->setProperty("InputWorkspace", inWS);
    alg->setProperty("MomentumTransferMinimum", 1.0);
    alg->setProperty("MomentumTransferStep", 0.2);
    alg->setProperty("MomentumTransferMaximum", 5.0);
    alg->setProperty("OutputWorkspace", "rebinnedWS");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    MatrixWorkspace_sptr rebinnedIvsQWS = alg->getProperty("OutputWorkspace");
    auto &xData = rebinnedIvsQWS->x(0);
    TSM_ASSERT_EQUALS("QMin should be the same as first Param entry (1.0)",
                      xData[0], 1.0);
    // based off the equation for logarithmic binning X(i+1)=X(i)(1+|dX|)
    double binWidthFromLogarithmicEquation = fabs((xData[1] / xData[0]) - 1);
    TSM_ASSERT_DELTA("DQQ should be the same as abs(x[1]/x[0] - 1)",
                     binWidthFromLogarithmicEquation, 0.2, 1e-06);
    TSM_ASSERT_EQUALS("QMax should be the same as last Param entry",
                      xData.back(), 5.0);
  }
  void test_post_processing_rebin_step_with_linear_rebinning() {
    auto alg = construct_standard_algorithm();
    auto inWS = create2DWorkspace154(1, 10, true);
    inWS->setInstrument(m_tinyReflWS->getInstrument());
    inWS->getAxis(0)->setUnit("Wavelength");
    alg->setProperty("InputWorkspace", inWS);
    alg->setProperty("MomentumTransferMinimum", 1.577);
    alg->setProperty("MomentumTransferStep", -0.2);
    alg->setProperty("MomentumTransferMaximum", 5.233);
    alg->setProperty("OutputWorkspace", "rebinnedWS");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    MatrixWorkspace_sptr rebinnedIvsQWS = alg->getProperty("OutputWorkspace");
    auto &xData = rebinnedIvsQWS->x(0);
    TSM_ASSERT_DELTA("QMin should be the same as the first Param entry (1.577)",
                     xData[0], 1.577, 1e-06);
    TSM_ASSERT_DELTA("DQQ should the same as 0.2", xData[1] - xData[0], 0.2,
                     1e-06);
    TSM_ASSERT_DELTA("QMax should be the same as the last Param entry (5.233)",
                     xData.back(), 5.233, 1e-06);
  }
  void test_Qrange() {
    // set up the axis for the instrument
    Instrument_sptr instrument = boost::make_shared<Instrument>();
    instrument->setReferenceFrame(boost::make_shared<ReferenceFrame>(
        Y /*up*/, Z /*along*/, Right, "0,0,0"));

    // add a source
    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0, 0, -1));
    instrument->add(source);
    instrument->markAsSource(source);

    // add a sample
    ObjComponent *sample = new ObjComponent("some-surface-holder");
    sample->setPos(V3D(0, 0, 0));
    instrument->add(sample);
    instrument->markAsSamplePos(sample);

    // add a detector
    Detector *det = new Detector("point-detector", 1, NULL);
    det->setPos(V3D(0, 1, 1));
    instrument->add(det);
    instrument->markAsDetector(det);

    // set the instrument to this workspace
    m_tinyReflWS->setInstrument(instrument);
    // set this detector ready for processing instructions
    m_tinyReflWS->getSpectrum(0).setDetectorID(det->getID());

    auto alg =
        AlgorithmManager::Instance().create("ReflectometryReductionOne", 1);
    alg->setRethrows(true);
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", m_tinyReflWS);
    alg->setProperty("WavelengthMin", 1.0);
    alg->setProperty("WavelengthMax", 15.0);
    alg->setProperty("I0MonitorIndex", 0);
    alg->setProperty("MonitorBackgroundWavelengthMin", 0.0);
    alg->setProperty("MonitorBackgroundWavelengthMax", 0.0);
    alg->setProperty("MonitorIntegrationWavelengthMin", 0.0);
    alg->setProperty("MonitorIntegrationWavelengthMax", 0.0);
    alg->setProperty("MomentumTransferStep", 0.1);
    alg->setProperty("NormalizeByIntegratedMonitors", false);
    alg->setProperty("CorrectDetectorPositions", true);
    alg->setProperty("CorrectionAlgorithm", "None");
    alg->setPropertyValue("ProcessingInstructions", "1");
    alg->setPropertyValue("OutputWorkspace", "x");
    alg->setPropertyValue("OutputWorkspaceWavelength", "y");
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg->execute());

    // retrieve the IvsLam workspace
    MatrixWorkspace_sptr inLam = alg->getProperty("OutputWorkspaceWavelength");
    // retrieve the IvsQ workspace
    MatrixWorkspace_sptr inQ = alg->getProperty("OutputWorkspace");
    // retrieve our Theta
    double outTheta = alg->getProperty("ThetaOut");

    TS_ASSERT_DELTA(45.0, outTheta, 0.00001);
    TS_ASSERT_EQUALS(source->getPos(),
                     inQ->getInstrument()->getSource()->getPos());
    // convert from degrees to radians for sin() function
    double outThetaInRadians = outTheta * M_PI / 180;

    double lamMin = inLam->x(0).front();
    double lamMax = inLam->x(0).back();

    // Derive our QMin and QMax from the equation
    double qMinFromEQ = (4 * M_PI * sin(outThetaInRadians)) / lamMax;
    double qMaxFromEQ = (4 * M_PI * sin(outThetaInRadians)) / lamMin;

    // Get our QMin and QMax from the workspace
    auto qMinFromWS = inQ->x(0).front();
    auto qMaxFromWS = inQ->x(0).back();

    // Compare the two values (they should be identical)
    TS_ASSERT_DELTA(qMinFromEQ, qMinFromWS, 0.00001);
    TS_ASSERT_DELTA(qMaxFromEQ, qMaxFromWS, 0.00001);
  }
};

#endif /* ALGORITHMS_TEST_REFLECTOMETRYREDUCTIONONETEST_H_ */
