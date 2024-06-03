// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidJson/Json.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Sample.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/ShowPossibleCells.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class ShowPossibleCellsTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    ShowPossibleCells alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Name of the output workspace.
    std::string WSName("peaks");
    LoadNexusProcessed loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", "TOPAZ_3007.peaks.nxs");
    loader.setPropertyValue("OutputWorkspace", WSName);

    TS_ASSERT(loader.execute());
    TS_ASSERT(loader.isExecuted());
    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(WSName)));
    TS_ASSERT(ws);
    // set a Niggli UB in the
    // oriented lattice
    V3D row_0(-0.101246, -0.040644, -0.061869);
    V3D row_1(0.014004, -0.079212, 0.007344);
    V3D row_2(-0.063451, 0.011072, 0.064430);

    Matrix<double> UB(3, 3, false);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);

    auto lattice = std::make_unique<Mantid::Geometry::OrientedLattice>();
    lattice->setUB(UB);
    ws->mutableSample().setOrientedLattice(std::move(lattice));

    // now get the UB back from the WS
    UB = ws->sample().getOrientedLattice().getUB();

    ShowPossibleCells alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaxScalarError", "0.2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BestOnly", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // check output cell information
    std::vector<std::string> cells = alg.getProperty("Cells");
    TS_ASSERT_EQUALS(cells.size(), 2);
    Json::Value cell;
    // first cell
    Mantid::JsonHelpers::parse(cells[0], &cell);
    TS_ASSERT_DELTA(cell["Error"].asDouble(), 0.0184, 1e-4)
    TS_ASSERT_EQUALS(cell["FormNumber"].asInt(), 25)
    TS_ASSERT_EQUALS(cell["CellType"].asString(), "Monoclinic")
    TS_ASSERT_EQUALS(cell["Centering"].asString(), "C")
    TS_ASSERT(cell.isMember("UB"))
    TS_ASSERT_DELTA(cell["a"].asDouble(), 14.1691, 1e-4)
    TS_ASSERT_DELTA(cell["b"].asDouble(), 19.2544, 1e-4)
    TS_ASSERT_DELTA(cell["c"].asDouble(), 8.6096, 1e-4)
    TS_ASSERT_DELTA(cell["alpha"].asDouble(), 89.900, 1e-3)
    TS_ASSERT_DELTA(cell["beta"].asDouble(), 105.100, 1e-3)
    TS_ASSERT_DELTA(cell["gamma"].asDouble(), 89.967, 1e-3)
    TS_ASSERT_DELTA(cell["volume"].asDouble(), 2267.75, 1e-2)
    // second cell
    Mantid::JsonHelpers::parse(cells[1], &cell);
    TS_ASSERT_DELTA(cell["Error"].asDouble(), 0.0, 1e-4)
    TS_ASSERT_EQUALS(cell["FormNumber"].asInt(), 44)
    TS_ASSERT_EQUALS(cell["CellType"].asString(), "Triclinic")
    TS_ASSERT_EQUALS(cell["Centering"].asString(), "P")
    TS_ASSERT(cell.isMember("UB"))
    TS_ASSERT_DELTA(cell["a"].asDouble(), 8.6096, 1e-4)
    TS_ASSERT_DELTA(cell["b"].asDouble(), 11.9497, 1e-4)
    TS_ASSERT_DELTA(cell["c"].asDouble(), 11.9562, 1e-4)
    TS_ASSERT_DELTA(cell["alpha"].asDouble(), 107.302, 1e-3)
    TS_ASSERT_DELTA(cell["beta"].asDouble(), 98.798, 1e-3)
    TS_ASSERT_DELTA(cell["gamma"].asDouble(), 98.966, 1e-3)
    TS_ASSERT_DELTA(cell["volume"].asDouble(), 1133.87, 1e-2)

    // Check the number of cells found for different input parameters
    int num_cells = alg.getProperty("NumberOfCells");
    TS_ASSERT_EQUALS(num_cells, 2);

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaxScalarError", "10"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BestOnly", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    num_cells = alg.getProperty("NumberOfCells");
    TS_ASSERT_EQUALS(num_cells, 14);

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaxScalarError", "10"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BestOnly", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    num_cells = alg.getProperty("NumberOfCells");
    TS_ASSERT_EQUALS(num_cells, 42);

    AnalysisDataService::Instance().remove(WSName);
  }

  void test_exec_LeanElasticPeaks() {
    // Name of the output workspace.
    std::string WSName("peaks");
    LoadNexusProcessed loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("Filename", "TOPAZ_3007.peaks.nxs");
    loader.setPropertyValue("OutputWorkspace", WSName);

    TS_ASSERT(loader.execute());
    TS_ASSERT(loader.isExecuted());
    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = std::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(WSName)));
    TS_ASSERT(ws);

    // Convert PeaksWorkspace to LeanElasticPeaksWorkspace
    auto lpw = std::make_shared<LeanElasticPeaksWorkspace>();
    for (auto peak : ws->getPeaks())
      lpw->addPeak(peak);
    AnalysisDataService::Instance().addOrReplace(WSName, lpw);

    // set a Niggli UB in the
    // oriented lattice
    V3D row_0(-0.101246, -0.040644, -0.061869);
    V3D row_1(0.014004, -0.079212, 0.007344);
    V3D row_2(-0.063451, 0.011072, 0.064430);

    Matrix<double> UB(3, 3, false);
    UB.setRow(0, row_0);
    UB.setRow(1, row_1);
    UB.setRow(2, row_2);

    auto lattice = std::make_unique<Mantid::Geometry::OrientedLattice>();
    lattice->setUB(UB);
    lpw->mutableSample().setOrientedLattice(std::move(lattice));

    // now get the UB back from the WS
    UB = lpw->sample().getOrientedLattice().getUB();

    ShowPossibleCells alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaxScalarError", "0.2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BestOnly", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // check output cell information
    std::vector<std::string> cells = alg.getProperty("Cells");
    TS_ASSERT_EQUALS(cells.size(), 2);
    Json::Value cell;
    // first cell
    Mantid::JsonHelpers::parse(cells[0], &cell);
    TS_ASSERT_DELTA(cell["Error"].asDouble(), 0.0184, 1e-4)
    TS_ASSERT_EQUALS(cell["FormNumber"].asInt(), 25)
    TS_ASSERT_EQUALS(cell["CellType"].asString(), "Monoclinic")
    TS_ASSERT_EQUALS(cell["Centering"].asString(), "C")
    TS_ASSERT(cell.isMember("UB"))
    TS_ASSERT_DELTA(cell["a"].asDouble(), 14.1691, 1e-4)
    TS_ASSERT_DELTA(cell["b"].asDouble(), 19.2544, 1e-4)
    TS_ASSERT_DELTA(cell["c"].asDouble(), 8.6096, 1e-4)
    TS_ASSERT_DELTA(cell["alpha"].asDouble(), 89.900, 1e-3)
    TS_ASSERT_DELTA(cell["beta"].asDouble(), 105.100, 1e-3)
    TS_ASSERT_DELTA(cell["gamma"].asDouble(), 89.967, 1e-3)
    TS_ASSERT_DELTA(cell["volume"].asDouble(), 2267.75, 1e-2)
    // second cell
    Mantid::JsonHelpers::parse(cells[1], &cell);
    TS_ASSERT_DELTA(cell["Error"].asDouble(), 0.0, 1e-4)
    TS_ASSERT_EQUALS(cell["FormNumber"].asInt(), 44)
    TS_ASSERT_EQUALS(cell["CellType"].asString(), "Triclinic")
    TS_ASSERT_EQUALS(cell["Centering"].asString(), "P")
    TS_ASSERT(cell.isMember("UB"))
    TS_ASSERT_DELTA(cell["a"].asDouble(), 8.6096, 1e-4)
    TS_ASSERT_DELTA(cell["b"].asDouble(), 11.9497, 1e-4)
    TS_ASSERT_DELTA(cell["c"].asDouble(), 11.9562, 1e-4)
    TS_ASSERT_DELTA(cell["alpha"].asDouble(), 107.302, 1e-3)
    TS_ASSERT_DELTA(cell["beta"].asDouble(), 98.798, 1e-3)
    TS_ASSERT_DELTA(cell["gamma"].asDouble(), 98.966, 1e-3)
    TS_ASSERT_DELTA(cell["volume"].asDouble(), 1133.87, 1e-2)

    // Check the number of cells found for different input parameters
    int num_cells = alg.getProperty("NumberOfCells");
    TS_ASSERT_EQUALS(num_cells, 2);

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaxScalarError", "10"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BestOnly", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    num_cells = alg.getProperty("NumberOfCells");
    TS_ASSERT_EQUALS(num_cells, 14);

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeaksWorkspace", WSName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaxScalarError", "10"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BestOnly", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    num_cells = alg.getProperty("NumberOfCells");
    TS_ASSERT_EQUALS(num_cells, 42);

    AnalysisDataService::Instance().remove(WSName);
  }
};
