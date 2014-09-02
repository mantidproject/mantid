#ifndef ALGORITHMS_TEST_REFLECTOMETRYREDUCTIONONETEST_H_
#define ALGORITHMS_TEST_REFLECTOMETRYREDUCTIONONETEST_H_

#include <cxxtest/TestSuite.h>
#include <algorithm>
#include "MantidAlgorithms/ReflectometryReductionOne.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace WorkspaceCreationHelper;

class ReflectometryReductionOneTest: public CxxTest::TestSuite
{
private:

  MatrixWorkspace_sptr m_tinyReflWS;

public:

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflectometryReductionOneTest *createSuite()
  {
    return new ReflectometryReductionOneTest();
  }
  static void destroySuite(ReflectometryReductionOneTest *suite)
  {
    delete suite;
  }

  ReflectometryReductionOneTest()
  {
    m_tinyReflWS = create2DWorkspaceWithReflectometryInstrument();
  }

  void test_tolam()
  {
    MatrixWorkspace_sptr toConvert = m_tinyReflWS;
    std::vector<int> detectorIndexRange;
    size_t workspaceIndexToKeep1 = 1;
    const int monitorIndex = 0;

    specid_t specId1 = toConvert->getSpectrum(workspaceIndexToKeep1)->getSpectrumNo();
    specid_t monitorSpecId = toConvert->getSpectrum(monitorIndex)->getSpectrumNo();

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
    ReflectometryWorkflowBase::DetectorMonitorWorkspacePair inLam = alg.toLam(toConvert,
        detectorIndexRangesStr, monitorIndex, boost::tuple<double, double>(wavelengthMin, wavelengthMax),
        boost::tuple<double, double>(backgroundWavelengthMin, backgroundWavelengthMax), wavelengthStep);

    // Unpack the results
    MatrixWorkspace_sptr detectorWS = inLam.get<0>();
    MatrixWorkspace_sptr monitorWS = inLam.get<1>();

    /* ---------- Checks for the detector workspace ------------------*/

    // Check units.
    TS_ASSERT_EQUALS("Wavelength", detectorWS->getAxis(0)->unit()->unitID());

    // Check the number of spectrum kept.
    TS_ASSERT_EQUALS(1, detectorWS->getNumberHistograms());

    auto map = detectorWS->getSpectrumToWorkspaceIndexMap();
    // Check the spectrum ids retained.
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
    // Check the spectrum ids retained.
    TS_ASSERT_EQUALS(map[monitorSpecId], 0);

  }

  IAlgorithm_sptr construct_standard_algorithm()
  {
    auto alg = AlgorithmManager::Instance().create("ReflectometryReductionOne");
    alg->setRethrows(true);
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("InputWorkspace", m_tinyReflWS);
    alg->setProperty("WavelengthMin", 1.0);
    alg->setProperty("WavelengthMax", 15.0);
    alg->setProperty("I0MonitorIndex", 0);
    alg->setProperty("MonitorBackgroundWavelengthMin", 14.0);
    alg->setProperty("MonitorBackgroundWavelengthMax", 15.0);
    alg->setProperty("MonitorIntegrationWavelengthMin", 4.0);
    alg->setProperty("MonitorIntegrationWavelengthMax", 10.0);
    alg->setPropertyValue("ProcessingInstructions", "1");
    alg->setPropertyValue("OutputWorkspace", "x");
    alg->setPropertyValue("OutputWorkspaceWavelength", "y");
    alg->setRethrows(true);
    return alg;
  }

  void test_execute()
  {
    auto alg = construct_standard_algorithm();
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    MatrixWorkspace_sptr workspaceInQ = alg->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr workspaceInLam = alg->getProperty("OutputWorkspaceWavelength");
    const double theta = alg->getProperty("ThetaOut");
    UNUSED_ARG(theta)
    UNUSED_ARG(workspaceInQ)
    UNUSED_ARG(workspaceInLam)
  }

  void test_calculate_theta()
  {

    auto alg = construct_standard_algorithm();

    alg->execute();
    // Should not throw

    const double outTheta = alg->getProperty("ThetaOut");

    TS_ASSERT_DELTA(45.0/2, outTheta, 0.00001);
  }

};

#endif /* ALGORITHMS_TEST_REFLECTOMETRYREDUCTIONONETEST_H_ */
