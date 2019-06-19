// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SOLIDANGLETEST_H_
#define SOLIDANGLETEST_H_

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/SolidAngle.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::CountVariances;
using Mantid::HistogramData::Counts;

class SolidAngleTest : public CxxTest::TestSuite {
public:
  static SolidAngleTest *createSuite() { return new SolidAngleTest(); }
  static void destroySuite(SolidAngleTest *suite) { delete suite; }

  SolidAngleTest() : inputSpace(""), outputSpace("") {
    SolidAngle alg;
    // Set up a small workspace for testing
    // Nhist = 144;
    auto space2D = createWorkspace<Workspace2D>(Nhist, 11, 10);
    BinEdges x{0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};
    Counts a{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    CountVariances e(a.begin(), a.end());

    for (int j = 0; j < Nhist; ++j) {
      space2D->setBinEdges(j, x);
      space2D->setCounts(j, a);
      space2D->setCountVariances(j, e);
    }

    // Register the workspace in the data service
    inputSpace = "SATestWorkspace";
    AnalysisDataService::Instance().add(inputSpace, space2D);

    // Load the instrument data
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    // Path to test input file assumes Test directory checked out from SVN
    std::string inputFile = "INES_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);
    loader.setPropertyValue("Workspace", inputSpace);
    loader.setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
    loader.execute();

    space2D->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

    // Mark one detector dead to test that it leads to zero solid angle
    space2D->mutableSpectrumInfo().setMasked(143, true);
  }

  void testInit() {
    SolidAngle alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    // Set the properties
    alg.setPropertyValue("InputWorkspace", inputSpace);
    outputSpace = "outWorkspace";
    alg.setPropertyValue("OutputWorkspace", outputSpace);
  }

  void testExec() {
    SolidAngle alg;
    if (!alg.isInitialized()) {
      alg.initialize();
      // Set the properties
      alg.setPropertyValue("InputWorkspace", inputSpace);
      outputSpace = "outWorkspace";
      alg.setPropertyValue("OutputWorkspace", outputSpace);
    }
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(
        input = AnalysisDataService::Instance().retrieve(inputSpace));

    Workspace2D_sptr output2D =
        boost::dynamic_pointer_cast<Workspace2D>(output);
    Workspace2D_sptr input2D = boost::dynamic_pointer_cast<Workspace2D>(input);
    // Check that the output unit is correct
    TS_ASSERT_EQUALS(output2D->getAxis(0)->unit()->unitID(), "TOF");

    const size_t numberOfSpectra = output2D->getNumberHistograms();
    TS_ASSERT_EQUALS(numberOfSpectra, (int)Nhist);
    for (size_t i = 0; i < numberOfSpectra - 1; ++i) {
      // all of the values should fall in this range for INES
      TS_ASSERT_DELTA(output2D->y(i)[0], 0.00139, 0.00001);

      TS_ASSERT_DELTA(output2D->x(i)[0], 0.0, 0.000001);
      TS_ASSERT_DELTA(output2D->x(i)[1], 10000.0, 0.000001);
      TS_ASSERT_DELTA(output2D->e(i)[0], 0.0, 0.000001);
    }

    // some specific, more accurate values
    TS_ASSERT_DELTA(output2D->y(5)[0], 0.00139822, 0.0000001);
    TS_ASSERT_DELTA(output2D->y(10)[0], 0.00139822, 0.0000001);
    TS_ASSERT_DELTA(output2D->y(20)[0], 0.00139822, 0.0000001);
    TS_ASSERT_DELTA(output2D->y(50)[0], 0.00139822, 0.0000001);

    // Check 'dead' detector spectrum gives zero solid angle
    TS_ASSERT_EQUALS(output2D->y(143).front(), 0);
  }

  void testExecSubset() {
    SolidAngle alg;
    if (!alg.isInitialized())
      alg.initialize();
    alg.setPropertyValue("InputWorkspace", inputSpace);
    alg.setPropertyValue("OutputWorkspace", outputSpace);
    alg.setPropertyValue("StartWorkspaceIndex", "50");
    alg.setPropertyValue("EndWorkspaceIndex", "59");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(
        input = AnalysisDataService::Instance().retrieve(inputSpace));

    Workspace2D_sptr output2D =
        boost::dynamic_pointer_cast<Workspace2D>(output);
    Workspace2D_sptr input2D = boost::dynamic_pointer_cast<Workspace2D>(input);
    // Check that the output unit is correct
    TS_ASSERT_EQUALS(output2D->getAxis(0)->unit()->unitID(), "TOF")

    const size_t numberOfSpectra = output2D->getNumberHistograms();
    size_t unmaskedSpectra{0};
    auto spectrumInfo = output2D->spectrumInfo();
    for (size_t i = 0; i < numberOfSpectra; ++i) {
      // all of the values should fall in this range for INES
      if (!spectrumInfo.isMasked(i)) {
        TS_ASSERT_DELTA(output2D->y(i)[0], 0.0013, 0.0001);
        TS_ASSERT_DELTA(output2D->x(i)[0], 0.0, 0.000001);
        TS_ASSERT_DELTA(output2D->x(i)[1], 10000.0, 0.000001);
        TS_ASSERT_DELTA(output2D->e(i)[0], 0.0, 0.000001);
        ++unmaskedSpectra;
      }
    }
    TS_ASSERT_EQUALS(unmaskedSpectra, 10);
  }

  void testCorrectWithIndex() {
    SolidAngle alg1;
    SolidAngle alg2;
    CreateSampleWorkspace createWS;
    createWS.initialize();

    if (!alg1.isInitialized())
      alg1.initialize();
    std::string outputWorkspace1 = "wholeOutput";
    std::string outputWorkspace2 = "50OnwardsOutput";
    std::string inputSpace2 = "IndexTestWS";
    createWS.setPropertyValue("OutputWorkspace", inputSpace2);
    createWS.execute();
    alg1.setPropertyValue("InputWorkspace", inputSpace2);
    alg1.setPropertyValue("OutputWorkspace", outputWorkspace1);
    TS_ASSERT_THROWS_NOTHING(alg1.execute());
    TS_ASSERT(alg1.isExecuted());
    Workspace_sptr output1;
    TS_ASSERT_THROWS_NOTHING(
        output1 = AnalysisDataService::Instance().retrieve(outputWorkspace1));

    if (!alg2.isInitialized())
      alg2.initialize();
    alg2.setPropertyValue("InputWorkspace", inputSpace2);
    alg2.setPropertyValue("OutputWorkspace", outputWorkspace2);
    alg2.setPropertyValue("StartWorkspaceIndex", "50");
    TS_ASSERT_THROWS_NOTHING(alg2.execute());
    TS_ASSERT(alg2.isExecuted());
    Workspace_sptr output2;
    TS_ASSERT_THROWS_NOTHING(
        output2 = AnalysisDataService::Instance().retrieve(outputWorkspace2));

    Workspace2D_sptr output2D_1 =
        boost::dynamic_pointer_cast<Workspace2D>(output1);
    Workspace2D_sptr output2D_2 =
        boost::dynamic_pointer_cast<Workspace2D>(output2);
    const size_t numberOfSpectra1 = output2D_1->getNumberHistograms();
    const size_t numberOfSpectra2 = output2D_2->getNumberHistograms();
    TS_ASSERT_EQUALS(numberOfSpectra1, numberOfSpectra2);
    auto spectrumInfo1 = output2D_1->spectrumInfo();
    auto spectrumInfo2 = output2D_2->spectrumInfo();
    for (size_t i = 0; i < numberOfSpectra1; i++) {
      // all values after the start point of the second workspace should match
      if (!(spectrumInfo2.isMasked(i) || spectrumInfo2.isMasked(i))) {
        TS_ASSERT_EQUALS(output2D_1->y(i)[0], output2D_2->y(i)[0]);
      }
    }
  }

private:
  std::string inputSpace;
  std::string outputSpace;
  enum { Nhist = 144 };
};

#endif /*SOLIDANGLETEST_H_*/
