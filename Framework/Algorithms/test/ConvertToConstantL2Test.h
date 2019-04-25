// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CONVERTTOCONSTANTL2TEST_H_
#define MANTID_ALGORITHMS_CONVERTTOCONSTANTL2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/ConvertToConstantL2.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/DetectorEfficiencyCorUser.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cmath>

using namespace Mantid::API;
using namespace Mantid;
using namespace Kernel;

using Mantid::Algorithms::ConvertToConstantL2;

class ConvertToConstantL2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertToConstantL2Test *createSuite() {
    return new ConvertToConstantL2Test();
  }
  static void destroySuite(ConvertToConstantL2Test *suite) { delete suite; }

  ConvertToConstantL2Test() : m_l2(4) { FrameworkManager::Instance(); }

  void testTheBasics() {
    ConvertToConstantL2 c;
    TS_ASSERT_EQUALS(c.name(), "ConvertToConstantL2");
    TS_ASSERT_EQUALS(c.version(), 1);
  }

  void test_detectors_move() {
    int numberOfAngles = 5;
    int numberOfBins = 10;

    auto inputWS = createTestWorkspace(numberOfAngles, numberOfBins);

    do_test_move(inputWS);
  }

  void test_monitors_do_not_get_moved() {
    auto inputWS = createTestMonitorWorkspace();
    do_test_move(inputWS);
  }

  void do_test_move(MatrixWorkspace_sptr inputWS) {
    // BEFORE - check the L2 values differ
    const auto &spectrumInfoInput = inputWS->spectrumInfo();
    for (size_t i = 0; i < inputWS->getNumberHistograms(); i++) {
      double r, theta, phi;
      Mantid::Kernel::V3D pos = spectrumInfoInput.position(i);
      pos.getSpherical(r, theta, phi);
      TS_ASSERT_DIFFERS(r, m_l2)
    }

    ConvertToConstantL2 c;
    if (!c.isInitialized())
      c.initialize();

    c.setPropertyValue("InputWorkspace", inputWSName);
    c.setPropertyValue("OutputWorkspace", outputWSName);
    c.execute();
    TS_ASSERT(c.isExecuted());

    Mantid::API::MatrixWorkspace_const_sptr outputWS =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>(outputWSName);

    // AFTER - test the first tube to see if distance was well corrected to l2
    const auto &spectrumInfoOutput = outputWS->spectrumInfo();
    for (size_t i = 0; i < outputWS->getNumberHistograms(); i++) {
      double r, theta, phi;
      Mantid::Kernel::V3D pos = spectrumInfoOutput.position(i);
      pos.getSpherical(r, theta, phi);

      // Corrected distance to l2, but not form monitors.
      if (spectrumInfoOutput.isMonitor(i)) {
        TS_ASSERT_DIFFERS(r, m_l2)
      } else {
        TS_ASSERT_DELTA(r, m_l2, 0.001)
      }
    }

    // Check the contents of the y and e parts of the histogram are the same,
    // and x differs
    for (size_t i = 0; i < outputWS->getNumberHistograms(); i++) {
      if (spectrumInfoOutput.isMonitor(i)) {
        TS_ASSERT_EQUALS(outputWS->x(i).rawData(), inputWS->x(i).rawData());
      } else {
        TS_ASSERT_DIFFERS(outputWS->x(i).rawData(), inputWS->x(i).rawData());
      }
      TS_ASSERT_EQUALS(outputWS->y(i).rawData(), inputWS->y(i).rawData());
      TS_ASSERT_EQUALS(outputWS->e(i).rawData(), inputWS->e(i).rawData());
    }

    AnalysisDataService::Instance().remove(outputWSName);
    AnalysisDataService::Instance().remove(inputWSName);
  }

private:
  int m_l2;
  double m_wavelength = 5.0;

  std::string inputWSName = "test_input_ws";
  std::string outputWSName = "test_output_ws";

  MatrixWorkspace_sptr createTestWorkspace(int numberOfAngles,
                                           int numberOfBins) {
    std::vector<double> L2(numberOfAngles, 5);
    std::vector<double> polar(numberOfAngles, (30. / 180.) * M_PI);
    polar[0] = 0;
    std::vector<double> azimuthal(numberOfAngles, 0);
    azimuthal[1] = (45. / 180.) * M_PI;
    azimuthal[2] = (90. / 180.) * M_PI;
    azimuthal[3] = (135. / 180.) * M_PI;
    azimuthal[4] = (180. / 180.) * M_PI;

    Mantid::API::MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::createProcessedInelasticWS(
            L2, polar, azimuthal, numberOfBins, -1, 3, 3);

    inputWS->getAxis(0)->setUnit("TOF");

    addSampleLogs(inputWS);

    API::AnalysisDataService::Instance().addOrReplace(inputWSName, inputWS);

    return inputWS;
  }

  MatrixWorkspace_sptr createTestMonitorWorkspace() {
    /*
     * Ideally this would test the detectors from CreateSampleWorkspace too, but
     * due to the way they are created in banks they do not get moved properly.
     * This may need fixing if the IN5 instrument definition is changed to use
     * banks.
     */

    Mantid::Algorithms::CreateSampleWorkspace create;
    create.initialize();
    create.setProperty("NumBanks", 0);
    create.setProperty("NumMonitors", 1);
    create.setProperty("BankDistanceFromSample", 5.0);
    create.setPropertyValue("OutputWorkspace", inputWSName);
    create.execute();

    MatrixWorkspace_sptr inputWS =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>(inputWSName);
    inputWS->populateInstrumentParameters();

    addSampleLogs(inputWS);

    return inputWS;
  }

  void addSampleLogs(MatrixWorkspace_sptr inputWS) {
    inputWS->mutableRun().addProperty("wavelength", m_wavelength);
    inputWS->instrumentParameters().addDouble(
        inputWS->getInstrument()->getComponentID(), "l2",
        m_l2);
  }
};

#endif /* MANTID_ALGORITHMS_CONVERTTOCONSTANTL2TEST_H_ */
