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
        toConvert->getSpectrum(workspaceIndexToKeep1)->getSpectrumNo();
    specnum_t monitorSpecId =
        toConvert->getSpectrum(monitorIndex)->getSpectrumNo();

    // Define one spectra to keep
    detectorIndexRange.push_back(static_cast<int>(workspaceIndexToKeep1));
    std::stringstream buffer;
    buffer << workspaceIndexToKeep1;
    const std::string detectorIndexRangesStr = buffer.str();

    // Define a wavelength range for the detector workspace
    const double wavelengthMin = 1.0;
    const double wavelengthMax = 15;
    const double wavelengthStep = 0.05;
    const double backgroundWavelengthMin = 17;
    const double backgroundWavelengthMax = 20;

    ReflectometryReductionOne alg;

    // Run the conversion.
    ReflectometryWorkflowBase::DetectorMonitorWorkspacePair inLam =
        alg.toLam(toConvert, detectorIndexRangesStr, monitorIndex,
                  boost::tuple<double, double>(wavelengthMin, wavelengthMax),
                  boost::tuple<double, double>(backgroundWavelengthMin,
                                               backgroundWavelengthMax),
                  wavelengthStep);

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
    Mantid::MantidVec copyX = detectorWS->readX(0);
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
    auto alg = AlgorithmManager::Instance().create("ReflectometryReductionOne");
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
    alg->setPropertyValue("ProcessingInstructions", "1");
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
    m_tinyReflWS->getSpectrum(0)->setDetectorID(det->getID());

    auto alg = AlgorithmManager::Instance().create("ReflectometryReductionOne");
    alg->setRethrows(true);
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", m_tinyReflWS);
    alg->setProperty("WavelengthMin", 1.0);
    alg->setProperty("WavelengthMax", 15.0);
    alg->setProperty("WavelengthStep", 0.1);
    alg->setProperty("I0MonitorIndex", 0);
    alg->setProperty("MonitorBackgroundWavelengthMin", 0.0);
    alg->setProperty("MonitorBackgroundWavelengthMax", 0.0);
    alg->setProperty("MonitorIntegrationWavelengthMin", 0.0);
    alg->setProperty("MonitorIntegrationWavelengthMax", 0.0);
    alg->setProperty("NormalizeByIntegratedMonitors", false);
    alg->setProperty("CorrectDetectorPositions", true);
    alg->setProperty("CorrectionAlgorithm", "None");
    alg->setPropertyValue("ProcessingInstructions", "1");
    alg->setPropertyValue("OutputWorkspace", "x");
    alg->setPropertyValue("OutputWorkspaceWavelength", "y");
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    MatrixWorkspace_sptr outLam = alg->getProperty("OutputWorkspaceWavelength");
    alg->setProperty("InputWorkspace", outLam);
    alg->setProperty("OutputWorkspace", "IvsQ");
    alg->setProperty("OutputWorkspaceWavelength", "IvsLam");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    MatrixWorkspace_sptr outQ = alg->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(m_tinyReflWS->getInstrument()->getSource()->getPos(),
                     outLam->getInstrument()->getSource()->getPos());
    TS_ASSERT_EQUALS(outLam->getInstrument()->getSource()->getPos(),
                     outQ->getInstrument()->getSource()->getPos());
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
    m_tinyReflWS->getSpectrum(0)->setDetectorID(det->getID());

    auto alg = AlgorithmManager::Instance().create("ReflectometryReductionOne");
    alg->setRethrows(true);
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", m_tinyReflWS);
    alg->setProperty("WavelengthMin", 1.0);
    alg->setProperty("WavelengthMax", 15.0);
    alg->setProperty("WavelengthStep", 0.1);
    alg->setProperty("I0MonitorIndex", 0);
    alg->setProperty("MonitorBackgroundWavelengthMin", 0.0);
    alg->setProperty("MonitorBackgroundWavelengthMax", 0.0);
    alg->setProperty("MonitorIntegrationWavelengthMin", 0.0);
    alg->setProperty("MonitorIntegrationWavelengthMax", 0.0);
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

    double lamMin = inLam->readX(0).front();
    double lamMax = inLam->readX(0).back();

    // Derive our QMin and QMax from the equation
    double qMinFromEQ = (4 * M_PI * sin(outThetaInRadians)) / lamMax;
    double qMaxFromEQ = (4 * M_PI * sin(outThetaInRadians)) / lamMin;

    // Get our QMin and QMax from the workspace
    auto qMinFromWS = inQ->readX(0).front();
    auto qMaxFromWS = inQ->readX(0).back();

    // Compare the two values (they should be identical)
    TS_ASSERT_DELTA(qMinFromEQ, qMinFromWS, 0.00001);
    TS_ASSERT_DELTA(qMaxFromEQ, qMaxFromWS, 0.00001);
  }
};

#endif /* ALGORITHMS_TEST_REFLECTOMETRYREDUCTIONONETEST_H_ */
