// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SETCRYSTALLOCATIONTEST_H_
#define SETCRYSTALLOCATIONTEST_H_
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/SetCrystalLocation.h"
#include "MantidCrystal/ShowPeakHKLOffsets.h"
#include "MantidDataHandling/Load.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include <cxxtest/TestSuite.h>

using Mantid::DataHandling::Load;
using namespace Mantid::DataObjects;
using Mantid::Crystal::LoadIsawUB;
using Mantid::Crystal::SetCrystalLocation;
using Mantid::Crystal::ShowPeakHKLOffsets;
using Mantid::DataObjects::TableWorkspace;
using Mantid::Kernel::V3D;
using namespace Mantid::API;

class SetCrystalLocationTest : public CxxTest::TestSuite {
public:
  void test_algo() {
    std::string WSName = "events";
    std::string file_name = "BSS_11841_event.nxs";
    Load loader;
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
    loader.setPropertyValue("OutputWorkspace", WSName);
    loader.setPropertyValue("Filename", file_name);
    TS_ASSERT(loader.execute());
    TS_ASSERT(loader.isExecuted());

    auto workspace = AnalysisDataService::Instance().retrieve(WSName);
    EventWorkspace_sptr events =
        boost::dynamic_pointer_cast<EventWorkspace>(workspace);
    TS_ASSERT(events);
    auto inst = events->getInstrument();
    TS_ASSERT(inst);
    auto sample = inst->getSample();
    TS_ASSERT(sample);

    SetCrystalLocation algo;
    TS_ASSERT_THROWS_NOTHING(algo.initialize());
    TS_ASSERT(algo.isInitialized());
    algo.setProperty("InputWorkspace", WSName);
    algo.setProperty("OutputWorkspace", WSName);
    algo.setProperty("NewX", 1.0);
    algo.setProperty("NewY", -0.30);
    algo.setProperty("NewZ", 10.0);

    // Check the sample is at the origin by default
    V3D sampPos0 = sample->getPos();
    TS_ASSERT_DELTA(sampPos0.X(), 0.0, 1.e-3);
    TS_ASSERT_DELTA(sampPos0.Y(), 0.0, 1.e-3);
    TS_ASSERT_DELTA(sampPos0.Z(), 0.0, 1.e-3);

    // Move the sample to (1.0, -0.3, 10.0)
    TS_ASSERT(algo.execute());
    TS_ASSERT(algo.isExecuted());

    // Check that the sample moved
    V3D sampPos1 = sample->getPos();
    TS_ASSERT_DELTA(sampPos1.X(), 1.0, 1.e-3);
    TS_ASSERT_DELTA(sampPos1.Y(), -0.30, 1.e-3);
    TS_ASSERT_DELTA(sampPos1.Z(), 10.0, 1.e-3);

    // Try it on a separate workspace
    SetCrystalLocation algo2;
    TS_ASSERT_THROWS_NOTHING(algo2.initialize());
    TS_ASSERT(algo2.isInitialized());
    algo2.setProperty("InputWorkspace", WSName);
    algo2.setProperty("OutputWorkspace", "events_new");
    algo2.setProperty("NewX", 2.0);
    algo2.setProperty("NewY", 4.0);
    algo2.setProperty("NewZ", 0.0);
    TS_ASSERT(algo2.execute());
    TS_ASSERT(algo2.isExecuted());

    // Check that the original is unchanged.  "Events" should be at
    // the same sampPos1 = (1.0, -0.3, 10.0)
    V3D sampPos2 = sample->getPos();
    TS_ASSERT_DELTA(sampPos2.X(), 1.0, 1.e-3);
    TS_ASSERT_DELTA(sampPos2.Y(), -0.30, 1.e-3);
    TS_ASSERT_DELTA(sampPos2.Z(), 10.0, 1.e-3);

    // Get pointers to the new workspace
    auto workspace_new = AnalysisDataService::Instance().retrieve("events_new");
    EventWorkspace_sptr events_new =
        boost::dynamic_pointer_cast<EventWorkspace>(workspace_new);
    TS_ASSERT(events_new)
    auto inst_new = events_new->getInstrument();
    TS_ASSERT(inst_new);
    auto sample_new = inst_new->getSample();
    TS_ASSERT(sample_new);

    // the new workspace should be at (2.,4.,0.,)
    V3D sampPos3 = sample_new->getPos();
    TS_ASSERT_DELTA(sampPos3.X(), 2.0, 1.e-3);
    TS_ASSERT_DELTA(sampPos3.Y(), 4.0, 1.e-3);
    TS_ASSERT_DELTA(sampPos3.Z(), 0.0, 1.e-3);
  }
};

#endif /* SETCRYSTALLOCATIONTEST_H_ */
