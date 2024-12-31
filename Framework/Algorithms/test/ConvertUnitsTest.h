// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/ConvertToDistribution.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Counts;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::CountVariances;
using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::Points;

namespace {

/// Creates a BinEdges workspace with TOF XUnits
void setup_WS(std::string &inputSpace) {
  // Set up a small workspace for testing
  auto space2D = createWorkspace<Workspace2D>(256, 11, 10);
  BinEdges x{0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};
  Counts a{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  CountVariances variances{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  CountStandardDeviations e(variances);
  for (int j = 0; j < 256; ++j) {
    space2D->setBinEdges(j, x);
    space2D->setCounts(j, a);
    space2D->setCountStandardDeviations(j, e);
    // Just set the spectrum number to match the index
    space2D->getSpectrum(j).setSpectrumNo(j);
    space2D->getSpectrum(j).setDetectorID(j);
  }
  space2D->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

  // Register the workspace in the data service
  AnalysisDataService::Instance().addOrReplace(inputSpace, space2D);

  // Load the instrument data
  Mantid::DataHandling::LoadInstrument loader;
  loader.initialize();
  // Path to test input file assumes Test directory checked out from SVN
  const std::string inputFile = ConfigService::Instance().getInstrumentDirectory() + "HET_Definition_old.xml";
  loader.setPropertyValue("Filename", inputFile);
  loader.setPropertyValue("Workspace", inputSpace);
  loader.setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
  loader.execute();
}

/// Creates a Points workspace with TOF XUnits
void setup_Points_WS(std::string &inputSpace) {
  // Set up a small workspace for testing
  auto space2D = createWorkspace<Workspace2D>(256, 10, 10);

  // these are the converted points from the BinEdges in setup_WS()
  Points x{500, 1500, 2500, 3500, 4500, 5500, 6500, 7500, 8500, 9500};
  Counts a{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  CountVariances variances{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  CountStandardDeviations e(variances);
  for (int j = 0; j < 256; ++j) {
    space2D->setPoints(j, x);
    space2D->setCounts(j, a);
    space2D->setCountStandardDeviations(j, e);
    // Just set the spectrum number to match the index
    space2D->getSpectrum(j).setSpectrumNo(j);
    space2D->getSpectrum(j).setDetectorID(j);
  }
  space2D->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

  // Register the workspace in the data service
  AnalysisDataService::Instance().addOrReplace(inputSpace, space2D);

  // Load the instrument data
  Mantid::DataHandling::LoadInstrument loader;
  loader.initialize();
  // Path to test input file assumes Test directory checked out from SVN
  const std::string inputFile = ConfigService::Instance().getInstrumentDirectory() + "HET_Definition_old.xml";
  loader.setPropertyValue("Filename", inputFile);
  loader.setPropertyValue("Workspace", inputSpace);
  loader.setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
  loader.execute();
}
} // namespace

class ConvertUnitsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertUnitsTest *createSuite() { return new ConvertUnitsTest(); }
  static void destroySuite(ConvertUnitsTest *suite) { delete suite; }

  void setUp() override { inputSpace = "testWorkspace"; }
  void testInit() {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  /// Tests the execution of the algorithm with a Points Workspace
  void test_Exec_Points_Input() {
    setup_Points_WS(inputSpace);

    ConvertUnits convertUnits;
    TS_ASSERT_THROWS_NOTHING(convertUnits.initialize());
    TS_ASSERT(convertUnits.isInitialized());
    TS_ASSERT_THROWS_NOTHING(convertUnits.setPropertyValue("InputWorkspace", inputSpace));
    TS_ASSERT_THROWS_NOTHING(convertUnits.setPropertyValue("OutputWorkspace", "outWS"));
    TS_ASSERT_THROWS_NOTHING(convertUnits.setPropertyValue("Target", "Wavelength"));
    TS_ASSERT_THROWS_NOTHING(convertUnits.execute());
    TS_ASSERT(convertUnits.isExecuted());

    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieve(inputSpace));
    Workspace2D_sptr input2D = std::dynamic_pointer_cast<Workspace2D>(input);

    // make sure input WS is not changed, i.e. still not Histogram
    TS_ASSERT(!input2D->isHistogramData());

    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS"));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    // Test that X data is still Points (it was converted back)
    TS_ASSERT(!output2D->isHistogramData());

    // Check that the output unit is correct
    TS_ASSERT_EQUALS(output2D->getAxis(0)->unit()->unitID(), "Wavelength");

    // Test that X data is still Points (it was converted back)
    TS_ASSERT_EQUALS(output2D->x(101).size(), 10);
    // Test that Y & E data is unchanged
    TS_ASSERT_EQUALS(output2D->y(101).size(), 10);
    TS_ASSERT_EQUALS(output2D->e(101).size(), 10);

    TS_ASSERT_DELTA(output2D->y(101)[0], input2D->y(101)[0], 1e-6);
    TS_ASSERT_DELTA(output2D->y(101)[4], input2D->y(101)[4], 1e-6);
    TS_ASSERT_DELTA(output2D->e(101)[1], input2D->e(101)[1], 1e-6);

    // Test that spectra that should have been zeroed have been
    TS_ASSERT_EQUALS(output2D->y(0)[1], 0);
    TS_ASSERT_EQUALS(output2D->e(0)[8], 0);
    // Check that the data has truly been copied (i.e. isn't a reference to the
    // same vector in both workspaces)
    double test[10] = {11, 22, 33, 44, 55, 66, 77, 88, 99, 1010};
    Counts testY(test, test + 10);
    CountStandardDeviations testE(test, test + 10);
    output2D->setCounts(111, testY);
    output2D->setCountStandardDeviations(111, testE);

    TS_ASSERT_EQUALS(output2D->y(111)[3], 44.0);

    TS_ASSERT_EQUALS(input2D->y(111)[3], 3.0);

    // Check that a couple of x points have been correctly converted
    TS_ASSERT_DELTA(output2D->x(103)[4], 1.4228, 0.0001);
    TS_ASSERT_DELTA(output2D->x(103)[5], 1.7389, 0.0001);
    TS_ASSERT_DELTA(output2D->x(103)[9], 3.0037, 0.0001);

    // Just check that an input bin boundary is unchanged
    TS_ASSERT_EQUALS(input2D->x(66)[4], 4500.0);

    AnalysisDataService::Instance().remove("outWS");
  }

  /// Tests converting back and forth with a Points Workspace
  void test_Points_Convert_Back_and_Forth() {
    // set up points WS with units TOF
    setup_Points_WS(inputSpace);

    ConvertUnits convertUnits;
    TS_ASSERT_THROWS_NOTHING(convertUnits.initialize());

    // used to hold the middle workspace
    const std::string temp_ws_name = "tempWS";

    alg.setRethrows(true);
    // Convert to Wavelength
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", inputSpace));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", temp_ws_name));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Target", "Wavelength"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Convert back to TOF
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", temp_ws_name));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "outWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Target", "TOF"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // get the input WS to compare values
    Workspace_sptr pointsWS;
    TS_ASSERT_THROWS_NOTHING(pointsWS = AnalysisDataService::Instance().retrieve(inputSpace));
    Workspace2D_sptr pointsWS2D = std::dynamic_pointer_cast<Workspace2D>(pointsWS);

    // This is the WS with units converted back to TOF
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS"));
    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);

    // Test that X data is still Points (it was converted back)
    TS_ASSERT(!output2D->isHistogramData());

    // check if the units are successfully converted
    TS_ASSERT_EQUALS(output2D->getAxis(0)->unit()->unitID(), "TOF");

    // Test that X data is still Points (it was converted back)
    TS_ASSERT_EQUALS(output2D->x(101).size(), 10);
    // Test that Y & E data is unchanged
    TS_ASSERT_EQUALS(output2D->y(101).size(), 10);
    TS_ASSERT_EQUALS(output2D->e(101).size(), 10);

    // Test that their size is the same
    TS_ASSERT_EQUALS(output2D->blocksize(), pointsWS2D->blocksize());

    // Test that spectra that should have been zeroed have been
    TS_ASSERT_EQUALS(output2D->y(0)[1], 0);
    TS_ASSERT_EQUALS(output2D->e(0)[8], 0);

    // Compare to see if X values have changed
    const size_t xsize = output2D->blocksize();
    for (size_t i = 0; i < output2D->getNumberHistograms(); ++i) {
      auto &inX = pointsWS2D->x(i);
      auto &outX = output2D->x(i);
      for (size_t j = 0; j < xsize; ++j) {
        TS_ASSERT_DELTA(outX[j], inX[j], 1e-9);
      }
    }

    AnalysisDataService::Instance().remove(temp_ws_name);
    AnalysisDataService::Instance().remove("outWS");
  }

  /* Test that when the units are the same between the input workspace and the
   * target, AND the output workspace name IS the same as the input workspace
   * name,
   * that the input workspace and output workspace point to the same in-memory
   * workspace.
   */
  void test_Exec_Input_Same_Output_And_Same_Units() {
    setup_WS(inputSpace);
    if (!alg.isInitialized())
      alg.initialize();

    auto inWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputSpace);
    // Set the properties
    alg.setRethrows(true);
    alg.setPropertyValue("InputWorkspace", inputSpace);
    alg.setPropertyValue("OutputWorkspace",
                         inputSpace);      // OutputWorkspace == InputWorkspace
    alg.setPropertyValue("Target", "TOF"); // Same as the input workspace.
    alg.execute();

    auto outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputSpace);
    TSM_ASSERT_EQUALS("Input and Output Workspaces should be pointer identical.", inWS.get(), outWS.get());
    AnalysisDataService::Instance().remove(inputSpace);
  }

  /* Test that when the units are the same between the input workspace and the
   * target, AND the output workspace name IS NOT the same as the input
   * workspace name,
   * that the input workspace and output workspace do not point to the same
   * in-memory workspace.
   */
  void test_Exec_Input_different_Output_But_Same_Units() {
    setup_WS(inputSpace);
    if (!alg.isInitialized())
      alg.initialize();

    auto inWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(inputSpace);
    // Set the properties
    alg.setRethrows(true);
    alg.setPropertyValue("InputWorkspace", inputSpace);
    const std::string outputWorkspaceName = "OutWSName";
    alg.setPropertyValue("OutputWorkspace",
                         outputWorkspaceName); // OutputWorkspace == InputWorkspace
    alg.setPropertyValue("Target", "TOF");     // Same as the input workspace.
    alg.execute();

    auto outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWorkspaceName);
    TSM_ASSERT_DIFFERS("Input and Output Workspaces be completely different objects.", inWS.get(), outWS.get());
    AnalysisDataService::Instance().remove(outputWorkspaceName);
    AnalysisDataService::Instance().remove(inputSpace);
  }

  void testExec() {
    setup_WS(inputSpace);
    if (!alg.isInitialized())
      alg.initialize();

    // Set the properties
    alg.setRethrows(true);
    alg.setPropertyValue("InputWorkspace", inputSpace);
    outputSpace = "outWorkspace";
    alg.setPropertyValue("OutputWorkspace", outputSpace);
    alg.setPropertyValue("Target", "Wavelength");
    alg.setPropertyValue("AlignBins", "1");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));
    Workspace_sptr input;
    TS_ASSERT_THROWS_NOTHING(input = AnalysisDataService::Instance().retrieve(inputSpace));

    Workspace2D_sptr output2D = std::dynamic_pointer_cast<Workspace2D>(output);
    Workspace2D_sptr input2D = std::dynamic_pointer_cast<Workspace2D>(input);

    // Check that the output unit is correct
    TS_ASSERT_EQUALS(output2D->getAxis(0)->unit()->unitID(), "Wavelength");

    // Test that y & e data is unchanged
    TS_ASSERT_EQUALS(output2D->y(101).size(), 10);
    TS_ASSERT_EQUALS(output2D->e(101).size(), 10);

    TS_ASSERT_DELTA(output2D->y(101)[0], input2D->y(101)[0], 1e-6);
    TS_ASSERT_DELTA(output2D->y(101)[4], input2D->y(101)[4], 1e-6);
    TS_ASSERT_DELTA(output2D->e(101)[1], input2D->e(101)[1], 1e-6);

    // Test that spectra that should have been zeroed have been
    TS_ASSERT_EQUALS(output2D->y(0)[1], 0);
    TS_ASSERT_EQUALS(output2D->e(0)[9], 0);
    // Check that the data has truly been copied
    //(i.e. isn't a reference to the same vector in both workspaces)
    double test[10] = {11, 22, 33, 44, 55, 66, 77, 88, 99, 1010};
    Counts testY(test, test + 10);
    CountStandardDeviations testE(test, test + 10);
    output2D->setCounts(111, testY);
    output2D->setCountStandardDeviations(111, testE);

    TS_ASSERT_EQUALS(output2D->y(111)[3], 44.0);

    TS_ASSERT_EQUALS(input2D->y(111)[3], 3.0);

    // Check that a couple of x bin boundaries have been correctly converted
    TS_ASSERT_DELTA(output2D->x(103)[5], 1.5808, 0.0001);
    TS_ASSERT_DELTA(output2D->x(103)[10], 3.1617, 0.0001);
    // Just check that an input bin boundary is unchanged
    TS_ASSERT_EQUALS(input2D->x(66)[4], 4000.0);

    AnalysisDataService::Instance().remove("outputSpace");
  }

  void testConvertQuickly() {
    ConvertUnits quickly;
    quickly.initialize();
    TS_ASSERT(quickly.isInitialized());
    quickly.setPropertyValue("InputWorkspace", outputSpace);
    quickly.setPropertyValue("OutputWorkspace", "quickOut2");
    quickly.setPropertyValue("Target", "Energy");
    TS_ASSERT_THROWS_NOTHING(quickly.execute());
    TS_ASSERT(quickly.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("quickOut2"));
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "Energy");
    TS_ASSERT_DELTA(output->x(1)[1], 10.10, 0.01);
    // Check EMode has been set
    TS_ASSERT_EQUALS(Mantid::Kernel::DeltaEMode::Elastic, output->getEMode());

    AnalysisDataService::Instance().remove("quickOut2");
  }

  void testConvertQuicklyCommonBins() {
    Workspace2D_sptr input = WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    input->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    AnalysisDataService::Instance().add("quickIn", input);
    ConvertUnits quickly;
    quickly.initialize();
    TS_ASSERT(quickly.isInitialized());
    quickly.setPropertyValue("InputWorkspace", "quickIn");
    quickly.setPropertyValue("OutputWorkspace", "quickOut");
    quickly.setPropertyValue("Target", "dSpacing");
    TS_ASSERT_THROWS_NOTHING(quickly.execute());
    TS_ASSERT(quickly.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("quickOut"));
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "dSpacing");
    // What is this meant to be testing should this be input vs output?
    TS_ASSERT_EQUALS(&(output->x(0)[0]), &(output->x(0)[0]));
    const size_t xsize = output->blocksize();
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      auto &outX = output->x(i);
      for (size_t j = 0; j <= xsize; ++j) {
        // Axis gets reversed by ConvertUnits to make it strictly increasing
        TS_ASSERT_EQUALS(outX[j], 2.0 * M_PI / (1.0 + static_cast<double>(xsize - j)));
      }
    }

    AnalysisDataService::Instance().remove("quickIn");
    AnalysisDataService::Instance().remove("quickOut");
  }

  void convertBackAndForth(bool inplace) {
    std::string tmp_ws_name = "tmp";
    if (inplace)
      tmp_ws_name = "output";

    double x0 = 0.1;
    // We have to make sure that bin width is non-zero and not 1.0, otherwise
    // the scaling of Y and E for the distribution case is not testable.
    double deltax = 0.123;
    Workspace2D_sptr input = WorkspaceCreationHelper::create2DWorkspaceBinned(2, 10, x0, deltax);
    input->getAxis(0)->unit() = UnitFactory::Instance().create("MomentumTransfer");
    // Y must have units, otherwise ConvertUnits does not treat data as
    // distribution.
    input->setYUnit("Counts");
    AnalysisDataService::Instance().add("input", input);

    ConvertToDistribution makeDist;
    makeDist.initialize();
    TS_ASSERT(makeDist.isInitialized());
    makeDist.setPropertyValue("Workspace", "input");
    TS_ASSERT_THROWS_NOTHING(makeDist.execute());
    TS_ASSERT(makeDist.isExecuted());
    TS_ASSERT(input->isDistribution());

    ConvertUnits convert1;
    convert1.initialize();
    TS_ASSERT(convert1.isInitialized());
    convert1.setPropertyValue("InputWorkspace", "input");
    convert1.setPropertyValue("OutputWorkspace", tmp_ws_name);
    convert1.setPropertyValue("Target", "dSpacing");
    TS_ASSERT_THROWS_NOTHING(convert1.execute());
    TS_ASSERT(convert1.isExecuted());

    ConvertUnits convert2;
    convert2.initialize();
    TS_ASSERT(convert2.isInitialized());
    convert2.setProperty("InputWorkspace", tmp_ws_name);
    convert2.setPropertyValue("OutputWorkspace", "output");
    convert2.setPropertyValue("Target", "MomentumTransfer");
    TS_ASSERT_THROWS_NOTHING(convert2.execute());
    TS_ASSERT(convert2.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("output"));
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "MomentumTransfer");
    // What is this testing? Does it have to do with copy-on-write dataX?
    TS_ASSERT_EQUALS(&(output->x(0)[0]), &(output->x(0)[0]));
    const size_t xsize = output->blocksize();
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      auto &inX = input->x(i);
      auto &inY = input->y(i);
      auto &outX = output->x(i);
      auto &outY = output->y(i);
      for (size_t j = 0; j <= xsize; ++j) {
        TS_ASSERT_DELTA(outX[j], inX[j], 1e-9);
      }
      for (size_t j = 0; j < xsize; ++j) {
        TS_ASSERT_DELTA(outY[j], inY[j], 1e-9);
      }
    }

    AnalysisDataService::Instance().remove("input");
    AnalysisDataService::Instance().remove(tmp_ws_name);
    AnalysisDataService::Instance().remove("output");
  }

  void testConvertBackAndForth() {
    bool inplace = false;
    convertBackAndForth(inplace);
  }

  void testConvertBackAndForthInPlace() {
    bool inplace = true;
    convertBackAndForth(inplace);
  }

  void testDeltaE() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 2663, 5, 7.5);
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

    Instrument_sptr testInst(new Instrument);
    // Make it look like MARI (though not bin boundaries are different to the
    // real MARI file used before)
    // Define a source and sample position
    // Define a source component
    ObjComponent *source = new ObjComponent("moderator", IObject_sptr(), testInst.get());
    source->setPos(V3D(0, 0.0, -11.739));
    testInst->add(source);
    testInst->markAsSource(source);
    // Define a sample position
    Component *sample = new Component("samplePos", testInst.get());
    testInst->setPos(0.0, 0.0, 0.0);
    testInst->add(sample);
    testInst->markAsSamplePos(sample);
    Detector *physicalPixel = new Detector("pixel", 1, testInst.get());
    physicalPixel->setPos(-0.34732, -3.28797, -2.29022);
    testInst->add(physicalPixel);
    testInst->markAsDetector(physicalPixel);
    ws->setInstrument(testInst);
    ws->getSpectrum(0).addDetectorID(physicalPixel->getID());

    ConvertUnits conv;
    conv.initialize();
    conv.setProperty("InputWorkspace", ws);
    std::string outputSpace = "outWorkspace";
    conv.setPropertyValue("OutputWorkspace", outputSpace);
    conv.setPropertyValue("Target", "DeltaE");
    conv.setPropertyValue("Emode", "Direct");
    conv.setPropertyValue("Efixed", "12.95");
    conv.execute();

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace));
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "DeltaE");
    TS_ASSERT_EQUALS(output->blocksize(), 1669);
    // Check EMode has been set
    TS_ASSERT_EQUALS(Mantid::Kernel::DeltaEMode::Direct, output->getEMode());

    ConvertUnits conv2;
    conv2.initialize();
    conv2.setProperty("InputWorkspace", ws);
    conv2.setPropertyValue("OutputWorkspace", outputSpace);
    conv2.setPropertyValue("Target", "DeltaE_inWavenumber");
    conv2.setPropertyValue("Emode", "Indirect");
    conv2.setPropertyValue("Efixed", "10");
    conv2.execute();

    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace));
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "DeltaE_inWavenumber");
    TS_ASSERT_EQUALS(output->blocksize(), 2275);
    // Check EMode has been set
    TS_ASSERT_EQUALS(Mantid::Kernel::DeltaEMode::Indirect, output->getEMode());

    ConvertUnits conv3;
    conv3.initialize();
    conv3.setProperty("InputWorkspace", ws);
    conv3.setPropertyValue("OutputWorkspace", outputSpace);
    conv3.setPropertyValue("Target", "DeltaE_inFrequency");
    conv3.setPropertyValue("Emode", "Direct");
    conv3.setPropertyValue("Efixed", "12.95");
    conv3.execute();

    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace));
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "DeltaE_inFrequency");
    TS_ASSERT_EQUALS(output->blocksize(), 1669);
    // Check EMode has been set
    TS_ASSERT_EQUALS(Mantid::Kernel::DeltaEMode::Direct, output->getEMode());

    ConvertUnits conv4;
    conv4.initialize();
    conv4.setProperty("InputWorkspace", ws);
    conv4.setPropertyValue("OutputWorkspace", outputSpace);
    conv4.setPropertyValue("Target", "dSpacingPerpendicular");
    conv4.setPropertyValue("Emode", "Direct");
    conv4.execute();

    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace));
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "dSpacingPerpendicular");
    TS_ASSERT_EQUALS(output->blocksize(), 2663);
    // Check EMode has been set
    TS_ASSERT_EQUALS(Mantid::Kernel::DeltaEMode::Direct, output->getEMode());

    AnalysisDataService::Instance().remove(outputSpace);
  }

  void testZeroLengthVectorExecutesWithNaNOutput() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 2663, 5, 7.5);
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");

    Instrument_sptr testInst(new Instrument);
    // Make it look like MARI (though not bin boundaries are different to the
    // real MARI file used before)
    // Define a source and sample position
    // Define a source component
    ObjComponent *source = new ObjComponent("moderator", IObject_sptr(), testInst.get());
    source->setPos(V3D(0, 0.0, -11.739));
    testInst->add(source);
    testInst->markAsSource(source);
    // Define a sample position
    Component *sample = new Component("samplePos", testInst.get());
    testInst->setPos(0.0, 0.0, 0.0);
    testInst->add(sample);
    testInst->markAsSamplePos(sample);
    Detector *dummyPixel = new Detector("pixel", 1, testInst.get());
    dummyPixel->setPos(0.0, 0.0, 0.0);
    testInst->add(dummyPixel);
    testInst->markAsDetector(dummyPixel);
    ws->setInstrument(testInst);
    ws->getSpectrum(0).addDetectorID(dummyPixel->getID());

    ConvertUnits conv;
    conv.initialize();
    conv.setProperty("InputWorkspace", ws);
    std::string outputSpace = "outWorkspace";
    conv.setPropertyValue("OutputWorkspace", outputSpace);
    conv.setPropertyValue("Target", "MomentumTransfer");
    conv.setPropertyValue("Emode", "Direct");
    conv.setPropertyValue("Efixed", "12.95");
    conv.execute();

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace));
    TS_ASSERT_EQUALS(output->getAxis(0)->unit()->unitID(), "MomentumTransfer");
    TS_ASSERT_EQUALS(Mantid::Kernel::DeltaEMode::Direct, output->getEMode());

    // conversion fails due to error in two theta calculation and leaves
    // spectrum masked and zeroed
    TS_ASSERT_EQUALS(output->y(0)[0], 0);
    TS_ASSERT(output->spectrumInfo().isMasked(0));
  }

  void setup_Event() {
    this->inputSpace = "eventWS";
    EventWorkspace_sptr ws = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 10, false);
    AnalysisDataService::Instance().addOrReplace(inputSpace, ws);
  }

  void testExecEvent_sameOutputWS() {
    std::size_t wkspIndex = 0;
    this->setup_Event();

    // Retrieve Workspace
    EventWorkspace_sptr WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inputSpace);
    TS_ASSERT(WS); // workspace is loaded
    size_t start_blocksize = WS->blocksize();
    size_t num_events = WS->getNumberEvents();
    EventList el = WS->getSpectrum(wkspIndex);
    double a_tof = el.getEvents()[0].tof();
    double a_x = el.x()[1];

    if (!alg.isInitialized())
      alg.initialize();
    TS_ASSERT(alg.isInitialized());

    // Set all the properties
    alg.setPropertyValue("InputWorkspace", inputSpace);
    alg.setPropertyValue("Target", "DeltaE");
    alg.setPropertyValue("EMode", "Direct");
    alg.setPropertyValue("Efixed", "15.0");
    this->outputSpace = inputSpace;
    alg.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Things that haven't changed
    TS_ASSERT_EQUALS(start_blocksize, WS->blocksize());
    TS_ASSERT_EQUALS(num_events, WS->getNumberEvents());
    // But a TOF changed.
    TS_ASSERT_DIFFERS(a_tof, WS->getSpectrum(wkspIndex).getEvents()[0].tof());
    // and a X changed
    TS_ASSERT_DIFFERS(a_x, WS->getSpectrum(wkspIndex).x()[1]);
    // Check EMode has been set
    TS_ASSERT_EQUALS(Mantid::Kernel::DeltaEMode::Direct, WS->getEMode());
  }

  void testExecEvent_TwoStepConversionWithDeltaE() {
    // Test to make sure the TOF->DeltaE->Other Quantity works for
    // EventWorkspaces
    this->setup_Event();

    ConvertUnits conv;
    conv.initialize();
    conv.setPropertyValue("InputWorkspace", this->inputSpace);
    conv.setPropertyValue("OutputWorkspace", this->inputSpace);
    conv.setPropertyValue("Target", "DeltaE");
    conv.setPropertyValue("Emode", "Direct");
    conv.setPropertyValue("Efixed", "15.0");
    conv.execute();

    ConvertUnits conv2;
    conv2.initialize();
    conv2.setPropertyValue("InputWorkspace", this->inputSpace);
    conv2.setPropertyValue("OutputWorkspace", this->inputSpace);
    conv2.setPropertyValue("Target", "Wavelength");
    conv2.setPropertyValue("Emode", "Direct");
    conv2.setPropertyValue("Efixed", "15.0");
    TS_ASSERT_THROWS_NOTHING(conv2.execute());
    TS_ASSERT(conv2.isExecuted());
  }

  /** Ticket #3934: If the workspace is sorted by TOF, it should remain so even
   * if
   * sorting flips the direction
   */
  void do_testExecEvent_RemainsSorted(EventSortType sortType, const std::string &targetUnit) {
    EventWorkspace_sptr ws = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 10, false);
    ws->getAxis(0)->setUnit("TOF");
    ws->sortAll(sortType, nullptr);

    // 0th detector unfortunately has difc=0 which doesn't support conversion to
    // d spacing so give it a more helpful difc value
    auto instrument = ws->getInstrument();
    auto det = instrument->getDetector(100);
    auto &paramMap = ws->instrumentParameters();
    paramMap.addDouble(det->getComponentID(), "DIFC", 1000);

    if (sortType == TOF_SORT) {
      // Only threadsafe if all the event lists are sorted
      TS_ASSERT(ws->threadSafe());
    }
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 100 * 200);

    ConvertUnits conv;
    conv.initialize();
    conv.setProperty("InputWorkspace", std::dynamic_pointer_cast<MatrixWorkspace>(ws));
    conv.setPropertyValue("OutputWorkspace", "out");
    conv.setPropertyValue("Target", targetUnit);
    conv.execute();
    TS_ASSERT(conv.isExecuted());

    EventWorkspace_sptr out = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("out");
    TS_ASSERT(out);
    if (!out)
      return;
    TS_ASSERT_EQUALS(out->getNumberEvents(), 100 * 200);

    EventList &el = out->getSpectrum(0);
    TS_ASSERT(el.getSortType() == sortType);

    if (sortType == TOF_SORT) {
      // Only threadsafe if all the event lists are sorted by TOF
      TS_ASSERT(out->threadSafe());

      // Check directly that it is indeed increasing
      double last_x = -1e10;
      for (size_t i = 0; i < el.getNumberEvents(); i++) {
        double x = el.getEvent(i).tof();
        TS_ASSERT(x >= last_x);
        last_x = x;
      }
    } else if (sortType == PULSETIME_SORT) {
      // Check directly that it is indeed increasing
      Mantid::Types::Core::DateAndTime last_x;
      for (size_t i = 0; i < el.getNumberEvents(); i++) {
        Mantid::Types::Core::DateAndTime x = el.getEvent(i).pulseTime();
        TS_ASSERT(x >= last_x);
        last_x = x;
      }
    }
  }

  void testExecEvent_RemainsSorted_TOF() { do_testExecEvent_RemainsSorted(TOF_SORT, "dSpacing"); }

  void testExecEvent_RemainsSorted_Pulsetime() { do_testExecEvent_RemainsSorted(PULSETIME_SORT, "dSpacing"); }

  void testExecEvent_RemainsSorted_TOF_to_Energy() { do_testExecEvent_RemainsSorted(TOF_SORT, "Energy"); }

  void testExecEvent_RemainsSorted_Pulsetime_to_Energy() { do_testExecEvent_RemainsSorted(PULSETIME_SORT, "Energy"); }

  void testDeltaEFailDoesNotAlterInPlaceWorkspace() {

    std::string wsName = "ConvertUnits_testDeltaEFailDoesNotAlterInPlaceWorkspace";
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 200, false);
    // set to a distribution
    ws->setDistribution(true);
    AnalysisDataService::Instance().add(wsName, ws);

    // get a copy of some original values
    auto originalUnit = ws->getAxis(0)->unit();
    auto originalEMode = ws->getEMode();
    TS_ASSERT_THROWS_ANYTHING(ws->getEFixed());
    auto originalYdata = ws->readY(0);

    ConvertUnits conv;
    conv.initialize();
    conv.setPropertyValue("InputWorkspace", wsName);
    // in place conversion
    conv.setPropertyValue("OutputWorkspace", wsName);
    conv.setPropertyValue("Target", "DeltaE");
    // do not set emode - this will cause a failure
    // do not set efixed either
    conv.execute();

    TSM_ASSERT("Expected ConvertUnits to throw on deltaE conversion without "
               "eMode or eFixed set",
               !conv.isExecuted());

    TS_ASSERT_EQUALS(originalUnit, ws->getAxis(0)->unit());
    TS_ASSERT_EQUALS(originalEMode, ws->getEMode());
    TS_ASSERT_THROWS_ANYTHING(ws->getEFixed());
    TS_ASSERT_EQUALS(originalYdata, ws->readY(0));

    AnalysisDataService::Instance().remove(wsName);
  }

  MatrixWorkspace_sptr createRaggedWS(const bool edges, const bool distribution) {
    // create and replace histograms with ragged ones - no monitors
    MatrixWorkspace_sptr raggedWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 1, false);
    if (edges) {
      raggedWS->setHistogram(0, Histogram(BinEdges{.5, 1., 1.5, 2.}, Counts{1., 2., 3.}));
      raggedWS->setHistogram(1, Histogram(BinEdges{.25, .75, 1.25}, Counts{4., 5.}));
    } else {
      raggedWS->setHistogram(0, Histogram(Points{.5, 1., 1.5}, Counts{1., 2., 3.}));
      raggedWS->setHistogram(1, Histogram(Points{.25, .75}, Counts{4., 5.}));
    }
    raggedWS->setDistribution(distribution);
    raggedWS->getAxis(0)->unit() = UnitFactory::Instance().create("dSpacing");

    // quick checks of the input workspace
    TS_ASSERT(raggedWS->isRaggedWorkspace());
    TS_ASSERT_EQUALS(raggedWS->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(raggedWS->isDistribution(), distribution);

    return raggedWS;
  }

  void test_ragged_Workspace2D_edges() {
    const std::string outname("raggedWSout_edges");
    const std::string TOF("TOF");
    constexpr bool bin_edges(true);

    for (const auto distribution : {true, false}) {
      MatrixWorkspace_sptr raggedWS = createRaggedWS(bin_edges, distribution); // not registered with ADS

      // d->Q avoids the toTof branch, d->TOF goes right to it
      for (const auto &targetUnits : {std::string("MomentumTransfer"), TOF}) {
        // run the algorithm - out-of-place to force creating new output workspace
        ConvertUnits convertUnits;
        TS_ASSERT_THROWS_NOTHING(convertUnits.initialize());
        TS_ASSERT(convertUnits.isInitialized());
        TS_ASSERT_THROWS_NOTHING(convertUnits.setProperty("InputWorkspace", raggedWS));
        TS_ASSERT_THROWS_NOTHING(convertUnits.setPropertyValue("OutputWorkspace", outname));
        TS_ASSERT_THROWS_NOTHING(convertUnits.setPropertyValue("Target", targetUnits));
        TS_ASSERT_THROWS_NOTHING(convertUnits.execute());
        TS_ASSERT(convertUnits.isExecuted());

        // for some reason the output has to be gotten from the ADS
        if (AnalysisDataService::Instance().doesExist(outname)) {
          const auto numHist = raggedWS->getNumberHistograms();
          MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outname);
          TS_ASSERT(outputWS->isRaggedWorkspace()); // output is still ragged
          TS_ASSERT_EQUALS(outputWS->isDistribution(), distribution);
          TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), numHist);
          TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), targetUnits);
          if (distribution) {
            // counts are scaled by bin width in new units
            TS_ASSERT_EQUALS(raggedWS->readY(0).size(), outputWS->readY(0).size());
            TS_ASSERT_EQUALS(raggedWS->readY(1).size(), outputWS->readY(1).size());
          } else {
            // counts are the same
            if (targetUnits.compare(TOF) == 0) {
              TS_ASSERT_EQUALS(raggedWS->readY(0), outputWS->readY(0));
              TS_ASSERT_EQUALS(raggedWS->readY(1), outputWS->readY(1));
            } else { // reversed for MomentumTransfer
              for (size_t i = 0; i < numHist; ++i) {
                auto Yorig = outputWS->readY(i); // make a copy
                std::reverse(Yorig.begin(), Yorig.end());
                TS_ASSERT_EQUALS(Yorig, raggedWS->readY(i));
              }
            }
          }
          // size of bins hasn't changed
          TS_ASSERT_EQUALS(raggedWS->readX(0).size(), outputWS->readX(0).size());
          TS_ASSERT_EQUALS(raggedWS->readX(1).size(), outputWS->readX(1).size());

          // remove output workspace
          AnalysisDataService::Instance().remove(outname);
        } else {
          TS_FAIL(std::string("OutputWorkspace was not created when targeting ") + targetUnits);
        }
      } // for targetUnits
    } // for distribution
  }

  /*
   * This test is disabled because it requires changes to ConvertToHistogram which requires changes to it's parent
   * XDataConverter. Maybe this could be combined with previous test
   */
  void xtest_ragged_Workspace2D_centers() {
    const std::string outname("raggedWSout_edges");
    const std::string TOF("TOF");
    constexpr bool bin_edges(false);

    for (const auto distribution : {true, false}) {
      MatrixWorkspace_sptr raggedWS = createRaggedWS(bin_edges, distribution); // not registered with ADS

      // d->Q avoids the toTof branch, d->TOF goes right to it
      for (const auto &targetUnits : {std::string("MomentumTransfer"), TOF}) {
        // run the algorithm - out-of-place to force creating new output workspace
        ConvertUnits convertUnits;
        TS_ASSERT_THROWS_NOTHING(convertUnits.initialize());
        TS_ASSERT(convertUnits.isInitialized());
        TS_ASSERT_THROWS_NOTHING(convertUnits.setProperty("InputWorkspace", raggedWS));
        TS_ASSERT_THROWS_NOTHING(convertUnits.setPropertyValue("OutputWorkspace", outname));
        TS_ASSERT_THROWS_NOTHING(convertUnits.setPropertyValue("Target", targetUnits));
        TS_ASSERT_THROWS_NOTHING(convertUnits.execute());
        TS_ASSERT(convertUnits.isExecuted());

        // for some reason the output has to be gotten from the ADS
        if (AnalysisDataService::Instance().doesExist(outname)) {
          const auto numHist = raggedWS->getNumberHistograms();
          MatrixWorkspace_sptr outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outname);
          TS_ASSERT(outputWS->isRaggedWorkspace()); // output is still ragged
          TS_ASSERT_EQUALS(outputWS->isDistribution(), distribution);
          TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), numHist);
          TS_ASSERT_EQUALS(outputWS->getAxis(0)->unit()->unitID(), targetUnits);
          if (distribution) {
            // counts are scaled by bin width in new units
            TS_ASSERT_EQUALS(raggedWS->readY(0).size(), outputWS->readY(0).size());
            TS_ASSERT_EQUALS(raggedWS->readY(1).size(), outputWS->readY(1).size());
          } else {
            // counts are the same
            if (targetUnits.compare(TOF) == 0) {
              TS_ASSERT_EQUALS(raggedWS->readY(0), outputWS->readY(0));
              TS_ASSERT_EQUALS(raggedWS->readY(1), outputWS->readY(1));
            } else { // reversed for MomentumTransfer
              for (size_t i = 0; i < numHist; ++i) {
                auto Yorig = outputWS->readY(i); // make a copy
                std::reverse(Yorig.begin(), Yorig.end());
                TS_ASSERT_EQUALS(Yorig, raggedWS->readY(i));
              }
            }
          }
          // size of bins hasn't changed
          TS_ASSERT_EQUALS(raggedWS->readX(0).size(), outputWS->readX(0).size());
          TS_ASSERT_EQUALS(raggedWS->readX(1).size(), outputWS->readX(1).size());

          // remove output workspace
          AnalysisDataService::Instance().remove(outname);
        } else {
          TS_FAIL(std::string("OutputWorkspace was not created when targeting ") + targetUnits);
        }
      } // for targetUnits
    } // for distribution
  }

private:
  ConvertUnits alg;
  std::string inputSpace;
  std::string outputSpace;
};

class ConvertUnitsTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertUnitsTestPerformance *createSuite() { return new ConvertUnitsTestPerformance(); }
  static void destroySuite(ConvertUnitsTestPerformance *suite) { delete suite; }

  void setUp() override {
    auto algo = AlgorithmManager::Instance().create("Load");
    algo->setPropertyValue("Filename", "HET15869");
    algo->setPropertyValue("OutputWorkspace", "hist_tof");
    algo->execute();
    algo = AlgorithmManager::Instance().create("Load");
    algo->setPropertyValue("Filename", "CNCS_7860_event");
    algo->setPropertyValue("OutputWorkspace", "event_tof");
    algo->execute();
    std::string WSname = "inputWS";
    setup_Points_WS(WSname);
  }

  void tearDown() override {
    AnalysisDataService::Instance().remove("outWS");
    AnalysisDataService::Instance().remove("hist_wave");
    AnalysisDataService::Instance().remove("hist_");
    AnalysisDataService::Instance().remove("event_wave");
    AnalysisDataService::Instance().remove("event_");
  }

  void test_points_workspace() {

    ConvertUnits alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "inputWS");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.setPropertyValue("Target", "Wavelength");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }

  void test_histogram_workspace() {
    auto alg = AlgorithmManager::Instance().create("ConvertUnits");
    alg->setPropertyValue("InputWorkspace", "hist_tof");
    alg->setPropertyValue("OutputWorkspace", "hist_wave");
    alg->setPropertyValue("Target", "Wavelength");
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    alg = AlgorithmManager::Instance().create("ConvertUnits");
    alg->setPropertyValue("InputWorkspace", "hist_tof");
    alg->setPropertyValue("OutputWorkspace", "hist_dSpacing");
    alg->setPropertyValue("Target", "dSpacing");
    alg->execute();
    TS_ASSERT(alg->isExecuted());
  }

  void test_event_workspace() {
    auto alg = AlgorithmManager::Instance().create("ConvertUnits");
    alg->setPropertyValue("InputWorkspace", "event_tof");
    alg->setPropertyValue("OutputWorkspace", "event_wave");
    alg->setPropertyValue("Target", "Wavelength");
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    alg = AlgorithmManager::Instance().create("ConvertUnits");
    alg->setPropertyValue("InputWorkspace", "event_tof");
    alg->setPropertyValue("OutputWorkspace", "event_dSpacing");
    alg->setPropertyValue("Target", "dSpacing");
    alg->execute();
    TS_ASSERT(alg->isExecuted());
  }

private:
  MatrixWorkspace_sptr histWS;
  MatrixWorkspace_sptr eventWS;
};
