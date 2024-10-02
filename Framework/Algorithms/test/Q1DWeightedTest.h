// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidAlgorithms/MaskBinsIf.h"
#include "MantidAlgorithms/Q1DWeighted.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"

#include <cxxtest/TestSuite.h>
#include <random>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::Algorithms::CompareWorkspaces;
using Mantid::Algorithms::MaskBinsIf;
using Mantid::Algorithms::Q1DWeighted;
using Mantid::DataHandling::LoadNexusProcessed;
using Mantid::DataHandling::LoadSpice2D;
using Mantid::DataHandling::MoveInstrumentComponent;

namespace {
bool const USE_NANS_EQUAL(true);
bool const USE_NANS_NOT_EQUAL(false);
} // namespace

class Q1DWeightedTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(radial_average.name(), "Q1DWeighted") }

  void testVersion() { TS_ASSERT_EQUALS(radial_average.version(), 1) }

  void testCategory() { TS_ASSERT_EQUALS(radial_average.category(), "SANS") }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(radial_average.initialize())
    TS_ASSERT(radial_average.isInitialized())
  }

  void setUp() override {
    loadAndMove();
    radial_average.initialize();
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testExec() {
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("InputWorkspace", m_inputWS))
    const std::string outputWS("result");
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputBinning", "0.01,0.001,0.11"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("NPixelDivision", "3"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("ErrorWeighting", "1"))

    TS_ASSERT_THROWS_NOTHING(radial_average.execute())

    TS_ASSERT(radial_average.isExecuted())

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(result = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
                                 AnalysisDataService::Instance().retrieve(outputWS)))
    TS_ASSERT_EQUALS(result->getNumberHistograms(), 1)

    // Timer is 3600.0 for this test data file
    double tolerance(1e-03);

    // The points we are checking were computed using the HFIR IGOR package
    // For NPixelDivision = 1
    //   Y[1] = 0.0398848*3600; Y[2] = 0.0371762*3600; Y[30] = 0.030971*3600;
    //   Y[80] = 0.0275545*3600; Y[90] = 0.0270528*3600
    TS_ASSERT_EQUALS(result->x(0)[0], 0.01);
    TS_ASSERT_DELTA(result->y(0)[30], 110.9651, tolerance);
    TS_ASSERT_DELTA(result->y(0)[1], 143.2190, tolerance);
    TS_ASSERT_DELTA(result->y(0)[2], 134.2864, tolerance);
    TS_ASSERT_DELTA(result->y(0)[80], 98.3834, tolerance);
    TS_ASSERT_DELTA(result->y(0)[90], 95.9322, tolerance);
  }

  // Test whether the WedgeOffset parameter works correctly.
  void testWedgeOffset() {

    const std::string outputWS("result");
    const std::string wedgeWS1("wedge1");
    const std::string wedgeWS2("wedge2");

    // Test method:
    // We use two wedges, which implies that they have an offset of 90 degree.
    // We then call the algorithm twice, once with offset 0, once with offset
    // 90. With offset 90 the wedges are thus logically "swapped", so we check
    // if their values match.
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("InputWorkspace", m_inputWS))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputBinning", "0.01,0.001,0.11"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("NPixelDivision", "3"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("ErrorWeighting", "1"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeWorkspace", wedgeWS1))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("NumberOfWedges", "2"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeAngle", "30"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeOffset", "0"))

    TS_ASSERT_THROWS_NOTHING(radial_average.execute())
    TS_ASSERT(radial_average.isExecuted())

    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("InputWorkspace", m_inputWS))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputBinning", "0.01,0.001,0.11"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("NPixelDivision", "3"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("ErrorWeighting", "1"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeWorkspace", wedgeWS2))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("NumberOfWedges", "2"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeAngle", "30"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeOffset", "90"))

    TS_ASSERT_THROWS_NOTHING(radial_average.execute())
    TS_ASSERT(radial_average.isExecuted())

    // Get wedge 0 of the result with offset 0.
    auto result1 = std::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve(wedgeWS1));
    auto wedge1 = std::dynamic_pointer_cast<MatrixWorkspace>(result1->getItem(0));

    // Get wedge 1 of the result with offset 90.
    auto result2 = std::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve(wedgeWS2));
    auto wedge2 = std::dynamic_pointer_cast<MatrixWorkspace>(result2->getItem(1));

    double tolerance = 1e-12;

    // The two wedges should be identical.
    for (size_t i = 0; i < wedge1->y(0).size(); ++i)
      TS_ASSERT_DELTA(wedge1->y(0)[i], wedge2->y(0)[i], tolerance);
  }

  // Test with masking
  void testWithMasking() {
    MaskBinsIf masker;
    masker.initialize();
    masker.setPropertyValue("InputWorkspace", m_inputWS);
    masker.setPropertyValue("OutputWorkspace", "__masked");
    // mask all the bins where the relative error is above 10%
    masker.setPropertyValue("Criterion", "e / y > 0.1");
    masker.execute();
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("InputWorkspace", "__masked"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputWorkspace", "__iqmasked"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputBinning", "0.001,0.001,0.08"))
    TS_ASSERT_THROWS_NOTHING(radial_average.execute())
    TS_ASSERT(radial_average.isExecuted())

    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("__iqmasked")))
    TS_ASSERT_EQUALS(result->getNumberHistograms(), 1)
    TS_ASSERT_DELTA(result->y(0)[6], 247.106, 0.001);
  }

  // Test the asymmetric wedges option
  void testWedgeAsymm() {

    const std::string outputWS("result");
    const std::string wedgeWS1("wedge1");
    const std::string wedgeWS2("wedge2");

    // We will call the algorithm twice:
    // once with symmetric wedges, once with twice as much of asymmetric wedges,
    // such that they are back-to-back in azimuthal plane. The pair-wise average
    // of asymmetric wedges should match the corresponding symmetric ones.

    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("InputWorkspace", m_inputWS))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputBinning", "0.01,0.001,0.08"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("NPixelDivision", "3"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("ErrorWeighting", "0"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeWorkspace", wedgeWS1))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("NumberOfWedges", "2"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeAngle", "30"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeOffset", "0"))

    TS_ASSERT_THROWS_NOTHING(radial_average.execute())
    TS_ASSERT(radial_average.isExecuted())

    radial_average.initialize();
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("InputWorkspace", m_inputWS))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputBinning", "0.01,0.001,0.08"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("NPixelDivision", "3"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("ErrorWeighting", "0"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeWorkspace", wedgeWS2))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("NumberOfWedges", "4"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeAngle", "30"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeOffset", "0"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("AsymmetricWedges", "1"))

    TS_ASSERT_THROWS_NOTHING(radial_average.execute())
    TS_ASSERT(radial_average.isExecuted())

    // Get the results of symmetric wedges.
    auto result1 = std::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve(wedgeWS1));
    TS_ASSERT(result1)
    auto wedge1 = std::dynamic_pointer_cast<MatrixWorkspace>(result1->getItem(0));
    auto wedge2 = std::dynamic_pointer_cast<MatrixWorkspace>(result1->getItem(1));

    TS_ASSERT(wedge1)
    TS_ASSERT(wedge2)

    // Get the results of asymmetric wedges.
    auto result2 = std::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve(wedgeWS2));
    TS_ASSERT(result2)
    auto wedgeA1 = std::dynamic_pointer_cast<MatrixWorkspace>(result2->getItem(0));
    auto wedgeA2 = std::dynamic_pointer_cast<MatrixWorkspace>(result2->getItem(1));
    auto wedgeA3 = std::dynamic_pointer_cast<MatrixWorkspace>(result2->getItem(2));
    auto wedgeA4 = std::dynamic_pointer_cast<MatrixWorkspace>(result2->getItem(3));

    TS_ASSERT(wedgeA1)
    TS_ASSERT(wedgeA2)
    TS_ASSERT(wedgeA3)
    TS_ASSERT(wedgeA4)

    double tolerance = 1e-12;

    // The average of A2 and A4 should be similar to wedge 2.
    for (size_t i = 0; i < wedge1->y(0).size(); ++i) {
      TS_ASSERT_DELTA(wedge2->y(0)[i], (wedgeA2->y(0)[i] + wedgeA4->y(0)[i]) / 2, tolerance);
    }
  }

  void testWithGravity() {
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("InputWorkspace", m_inputWS))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputWorkspace", "__iqg"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setProperty("AccountForGravity", true))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputBinning", "0.001,0.001,0.08"))
    TS_ASSERT_THROWS_NOTHING(radial_average.execute())
    TS_ASSERT(radial_average.isExecuted())
    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("__iqg")))
    TS_ASSERT_EQUALS(result->getNumberHistograms(), 1)
    TS_ASSERT_DELTA(result->y(0)[6], 251.052, 0.001);
  }

  void testShapeTable() {
    // test if the shape table returns the correct number of wedges
    std::shared_ptr<ITableWorkspace> table = createShapeTable();

    std::string outputWS = "q1d_shapes";

    populateAlgorithm(outputWS, outputWS + "_wedges", true, true, -1, table);

    TS_ASSERT_THROWS_NOTHING(radial_average.execute())

    WorkspaceGroup_sptr result;
    TS_ASSERT_THROWS_NOTHING(result = std::dynamic_pointer_cast<WorkspaceGroup>(
                                 AnalysisDataService::Instance().retrieve(outputWS + "_wedges")))
    TS_ASSERT_EQUALS(result->getNumberOfEntries(), 2)
  }

  void testShapeTableResults() {
    // test the results computed by the table shape method against those from
    // the usual method

    std::shared_ptr<ITableWorkspace> table = createShapeTable(true);

    std::string outputWS = "q1d_shapes";
    std::string outputWedges = outputWS + "_wedges";

    populateAlgorithm(outputWS, outputWedges, true, false, -1, table);
    TS_ASSERT_THROWS_NOTHING(radial_average.execute())

    std::string refWS = "q1d_wedges";
    std::string refWedges = refWS + "_wedges";

    populateAlgorithm(refWS, refWedges, false, false, 2);
    TS_ASSERT_THROWS_NOTHING(radial_average.execute())

    compareWorkspaces(refWedges, outputWedges);
  }

  /**
   * The result and the expected value used in this test are two matrix
   * workspaces with NaNs in y-values and e-values.
   */
  void testShapeTableResultsAsymm() {
    // test the results computed by the table shape method against those from
    // the usual method when asymmetricWedges is set to true

    std::shared_ptr<ITableWorkspace> table = createShapeTable(true);

    std::string outputWS = "q1d_shapes";
    std::string outputWedges = outputWS + "_wedges";

    populateAlgorithm(outputWS, outputWedges, true, true, -1, table);
    TS_ASSERT_THROWS_NOTHING(radial_average.execute())

    std::string refWS = "q1d_wedges";
    std::string refWedges = refWS + "_wedges";

    populateAlgorithm(refWS, refWedges, false, true, 4);
    TS_ASSERT_THROWS_NOTHING(radial_average.execute())

    compareWorkspaces(refWedges, outputWedges, USE_NANS_EQUAL);
  }

  void testShapeCorrectOrder() {
    // exactly the same test as testShapeTableResults, except the shapes are
    // created in a different order. The result should still be 1-to-1 identical
    // with the wedges results
    std::shared_ptr<ITableWorkspace> table = createShapeTable(true, true);
    std::string outputWS = "q1d_shapes";
    std::string outputWedges = outputWS + "_wedges";

    populateAlgorithm(outputWS, outputWedges, true, false, -1, table);

    TS_ASSERT_THROWS_NOTHING(radial_average.execute())

    std::string refWS = "q1d_wedges";
    std::string refWedges = refWS + "_wedges";

    populateAlgorithm(refWS, refWedges, false, false, 2);
    TS_ASSERT_THROWS_NOTHING(radial_average.execute())

    compareWorkspaces(refWedges, outputWedges);
  }

  void testMonochromaticCase() {
    // Test behaviour when the workspace is monochromatic, and each bin is a different sample and should be kept
    // separate in the end.
    LoadNexusProcessed loader;

    loader.initialize();
    loader.setPropertyValue("Filename", "ILL/D11B/kinetic.nxs");
    std::string inputWsName = "input";
    loader.setPropertyValue("OutputWorkspace", inputWsName);
    loader.execute();

    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("InputWorkspace", inputWsName))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputWorkspace", "out"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputBinning", "0,0.002,0.1"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("NumberOfWedges", "2"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeAngle", "90"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeWorkspace", "out_wedges"))
    TS_ASSERT_THROWS_NOTHING(radial_average.setProperty("AccountForGravity", false))
    TS_ASSERT_THROWS_NOTHING(radial_average.setProperty("ErrorWeighting", false))
    TS_ASSERT_THROWS_NOTHING(radial_average.setProperty("AsymmetricWedges", false))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("NPixelDivision", "1"))

    TS_ASSERT_THROWS_NOTHING(radial_average.execute())
    TS_ASSERT(radial_average.isExecuted())
    MatrixWorkspace_sptr result, wedge1, wedge2;
    TS_ASSERT_THROWS_NOTHING(
        result = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("out")))

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 3)
    TS_ASSERT_DELTA(result->y(0)[2], 0.3125, 0.0001)
    TS_ASSERT_DELTA(result->y(1)[2], 0.3125, 0.0001)
    TS_ASSERT_DELTA(result->y(2)[2], 0.1875, 0.0001)
    TS_ASSERT(result->isCommonBins())
    TS_ASSERT_EQUALS(result->getMaxNumberBins(), 50)

    TS_ASSERT_THROWS_NOTHING(
        wedge1 = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("out_wedges_1")))
    TS_ASSERT_THROWS_NOTHING(
        wedge2 = std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("out_wedges_2")))

    TS_ASSERT(wedge1->isCommonBins())
    TS_ASSERT(wedge2->isCommonBins())
    TS_ASSERT_EQUALS(wedge1->getNumberHistograms(), 3)
    TS_ASSERT_EQUALS(wedge2->getNumberHistograms(), 3)
    TS_ASSERT_EQUALS(wedge1->getMaxNumberBins(), 50)
    TS_ASSERT_EQUALS(wedge2->getMaxNumberBins(), 50)

    // Check some random values in the wedges to assert there is some data
    TS_ASSERT_DELTA(wedge1->y(0)[5], 0.15, 1e-5)
    TS_ASSERT_DELTA(wedge2->y(0)[8], 0.125, 1e-5)
    TS_ASSERT_DELTA(wedge1->y(1)[12], 0.0625, 1e-5)
    TS_ASSERT_DELTA(wedge2->y(1)[4], 0.125, 1e-5)
    TS_ASSERT_DELTA(wedge1->y(2)[7], 0.1, 1e-5)
    TS_ASSERT_DELTA(wedge2->y(2)[15], 0.25, 1e-5)
  }

private:
  void loadAndMove() {
    // This generates an appropriate real life workspace for testing.
    LoadSpice2D loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "BioSANS_exp61_scan0004_0001.xml");
    m_inputWS = "wav";
    loader.setPropertyValue("OutputWorkspace", m_inputWS);
    loader.execute();

    // Move detector to its correct position
    MoveInstrumentComponent mover;
    mover.initialize();
    mover.setPropertyValue("Workspace", m_inputWS);
    mover.setPropertyValue("ComponentName", "detector1");

    // According to the instrument geometry, the center of the detector is
    // located at N_pixel / 2 + 0.5
    // X = (16-192.0/2.0+0.5)*5.15/1000.0 = -0.409425
    // Y = (95-192.0/2.0+0.5)*5.15/1000.0 = -0.002575
    // mover.setPropertyValue("X", "0.409425");
    // mover.setPropertyValue("Y", "0.002575");

    mover.setPropertyValue("X", "0.009425");
    mover.setPropertyValue("Y", "0.002575");
    mover.setPropertyValue("Z", "-0.8114");

    mover.execute();
  }

  /**
   * Create a table containing the description of sectors
   *
   * @param alignWithWedges if true, the sectors correspond to wedges as defined
   * by Q1DWeighted. Else they are arbitrary defined
   * @param reverseOrder if true, the sectors are defined in the reverse order
   * from the way they are defined by Q1DWeighted. Else, standard way.
   */

  std::shared_ptr<ITableWorkspace> createShapeTable(bool alignWithWedges = false, bool reverseOrder = false) {
    // since the instrument viewer mostly lacks an API, we create a dummy
    // MaskShapes table

    using namespace Mantid::API;
    std::shared_ptr<ITableWorkspace> table = WorkspaceFactory::Instance().createTable();
    table->addColumn("str", "Index");
    table->addColumn("str", "Parameters");

    std::string sector;
    std::string viewport;
    if (!alignWithWedges) {
      sector = createDummySector(0.1, 0.5, 230, 10, 0.2, -0.1);
      TableRow row = table->appendRow();
      row << std::to_string(1) << sector;

      sector = createDummySector(0, 10, 0, 15, -0.2, 0);
      row = table->appendRow();
      row << std::to_string(2) << sector;

      viewport = createDummyViewport(0.2, 0.1, 1.2, 0, 0, 1, 0);
      row = table->appendRow();
      row << std::to_string(-1) << viewport;
    } else {
      double centerXOffset = 0.5;
      double centerYOffset = -0.3;

      double zoom = 1.2;
      double innerRadius = 0;
      double outerRadius = 100;
      double startAngle = 3 * M_PI / 4;
      double endAngle = 5 * M_PI / 4;

      for (size_t i = 0; i < 4; ++i) {

        sector = createDummySector(innerRadius * zoom, outerRadius * zoom, startAngle, endAngle, centerXOffset * zoom,
                                   centerYOffset * zoom);
        TableRow row = table->appendRow();
        row << std::to_string(i) << sector;
        if (!reverseOrder) {
          startAngle = fmod(startAngle + M_PI / 2, 2 * M_PI);
          endAngle = fmod(endAngle + M_PI / 2, 2 * M_PI);
        } else {
          startAngle = fmod(startAngle - M_PI / 2, 2 * M_PI);
          endAngle = fmod(endAngle - M_PI / 2, 2 * M_PI);
        }
      }

      viewport = createDummyViewport(centerXOffset * zoom, centerYOffset * zoom, zoom, 0, 0, 1, 0);
      TableRow row = table->appendRow();
      row << std::to_string(-1) << viewport;
    }

    return table;
  }

  std::string createDummySector(double innerRadius, double outerRadius, double startAngle, double endAngle,
                                double centerX, double centerY) {
    std::stringstream ss;
    ss << "Type\tsector" << std::endl;
    ss << "Parameters\t" << innerRadius << "\t" << outerRadius << "\t" << startAngle << "\t" << endAngle << "\t"
       << centerX << "\t" << centerY;
    // the other parameters are omitted, since they are not useful for us
    return ss.str();
  }

  std::string createDummyViewport(double transX, double transY, double zoom, double rotation0, double rotation1,
                                  double rotation2, double rotation3) {
    std::stringstream ss;
    ss << "Translation\t" << transX << "\t" << transY << std::endl;
    ss << "Zoom\t" << zoom << std::endl;
    ss << "Rotation\t" << rotation0 << "\t" << rotation1 << "\t" << rotation2 << "\t" << rotation3;
    return ss.str();
  }

  void populateAlgorithm(std::string outputWS, std::string wedgesWS, bool useTable, bool asymmetric = false,
                         const int wedgesTotal = 4,
                         std::shared_ptr<ITableWorkspace> shapeWS = std::shared_ptr<ITableWorkspace>(),
                         const std::string binning = "0.001,0.001,0.08") {
    std::string asymm_flag = asymmetric ? "1" : "0";
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("InputWorkspace", m_inputWS))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputWorkspace", outputWS))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("OutputBinning", binning))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeWorkspace", wedgesWS))
    TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("AsymmetricWedges", asymm_flag))

    if (useTable) {
      TS_ASSERT_THROWS_NOTHING(radial_average.setProperty("ShapeTable", shapeWS))
    } else {
      TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("NumberOfWedges", std::to_string(wedgesTotal)))
      TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeAngle", "90"))
      TS_ASSERT_THROWS_NOTHING(radial_average.setPropertyValue("WedgeOffset", "0"))
    }
  }

  void compareWorkspaces(std::string refWS, std::string toCompare, bool nansEqual = USE_NANS_NOT_EQUAL) {
    WorkspaceGroup_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = std::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve(toCompare)))

    WorkspaceGroup_sptr ref =
        std::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve(refWS));

    TS_ASSERT_EQUALS(result->getNumberOfEntries(), ref->getNumberOfEntries())
    std::string tolerance = "1e-12";

    CompareWorkspaces comparison;
    comparison.initialize();

    comparison.setProperty("Workspace1", ref);
    comparison.setProperty("Workspace2", result);
    comparison.setPropertyValue("Tolerance", tolerance);
    comparison.setPropertyValue("CheckAllData", "1");
    comparison.setPropertyValue("CheckType", "1");
    comparison.setPropertyValue("ToleranceRelErr", "1");
    comparison.setProperty("NaNsEqual", nansEqual);
    TS_ASSERT(comparison.execute())
    TS_ASSERT(comparison.isExecuted())
    TS_ASSERT_EQUALS(comparison.getPropertyValue("Result"), "1");
  }

  Q1DWeighted radial_average;
  std::string m_inputWS;
};

class Q1DWeightedTestPerformance : public CxxTest::TestSuite {
public:
  static Q1DWeightedTestPerformance *createSuite() { return new Q1DWeightedTestPerformance(); }
  static void destroySuite(Q1DWeightedTestPerformance *suite) { delete suite; }

  Q1DWeightedTestPerformance() {}

  void setUp() override {
    // We use the largest D33 detector in LTOF mode and with bin masking,
    // which makes up presumably the heaviest duty for the algorithm.
    LoadNexusProcessed loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "ILL/D33/LTOF_red.nxs");
    loader.setPropertyValue("OutputWorkspace", "__in");
    loader.execute();
    MaskBinsIf masker;
    masker.initialize();
    masker.setPropertyValue("InputWorkspace", "__in");
    masker.setPropertyValue("OutputWorkspace", "__in");
    masker.setPropertyValue("Criterion", "x < 1 || x > 10");
    masker.execute();
    m_alg.initialize();
    m_alg.setPropertyValue("InputWorkspace", "__in");
    m_alg.setPropertyValue("OutputBinning", "0.0003,-0.1,10.");
    m_alg.setProperty("NumberOfWedges", 2);
    m_alg.setProperty("NPixelDivision", 2);
    m_alg.setProperty("AccountForGravity", true);
    m_alg.setPropertyValue("OutputWorkspace", "__out");
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_performance() { TS_ASSERT_THROWS_NOTHING(m_alg.execute()); }

private:
  Q1DWeighted m_alg;
};
