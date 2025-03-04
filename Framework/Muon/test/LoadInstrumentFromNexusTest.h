// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidMuon/LoadInstrumentFromNexus.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;

class LoadInstrumentFromNexusTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadInstrumentFromNexusTest *createSuite() { return new LoadInstrumentFromNexusTest(); }
  static void destroySuite(LoadInstrumentFromNexusTest *suite) { delete suite; }

  void testLoadInstrumentFromNexus() {
    // setup and run the algorithm (includes basic checks)
    LoadInstrumentFromNexus alg;
    const MatrixWorkspace_sptr ws = setupAlgorithm(alg, "MUSR00015189.nxs");
    runAlgorithm(alg);

    // specific checks
    const Instrument_const_sptr inst = ws->getInstrument();
    TS_ASSERT(inst);
    TS_ASSERT_EQUALS(inst->getName(), "MUSR");

    const IComponent_const_sptr sample = inst->getSample();
    const V3D samplepos = sample->getPos();
    TS_ASSERT_EQUALS(sample->getName(), "Unknown");
    TS_ASSERT_DELTA(samplepos.X(), 0.0, 1e-6);
    TS_ASSERT_DELTA(samplepos.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(samplepos.Z(), 0.0, 1e-6);

    const IComponent_const_sptr source = inst->getSource();
    const V3D sourcepos = source->getPos();
    TS_ASSERT_EQUALS(source->getName(), "Unknown");
    TS_ASSERT_DELTA(sourcepos.X(), 0.0, 1e-6);
    TS_ASSERT_DELTA(sourcepos.Y(), -10.0, 1e-6);
    TS_ASSERT_DELTA(sourcepos.Z(), 0.0, 1e-6);
  }

private:
  const MatrixWorkspace_sptr makeFakeWorkspace() {
    // create the workspace
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(2, 10);
    return ws;
  }

  // Initialise the algorithm and set the properties. Creates a fake
  // workspace for the input.
  const MatrixWorkspace_sptr setupAlgorithm(LoadInstrumentFromNexus &alg, const std::string &filename) {
    // create the workspace
    const MatrixWorkspace_sptr inWS = makeFakeWorkspace();

    // set up the algorithm
    if (!alg.isInitialized())
      alg.initialize();
    alg.setProperty("Workspace", inWS);
    alg.setProperty("Filename", filename);

    return inWS;
  }

  // Run the algorithm and do some basic checks
  void runAlgorithm(LoadInstrumentFromNexus &alg) {
    // run the algorithm
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }
};
