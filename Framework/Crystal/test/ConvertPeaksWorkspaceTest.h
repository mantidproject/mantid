// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Run.h"
#include "MantidCrystal/ConvertPeaksWorkspace.h"
#include "MantidDataObjects/LeanElasticPeak.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::Crystal::ConvertPeaksWorkspace;

namespace {
/// static logger
Logger g_log("ConvertPeaksWorkspaceTest");
} // namespace

class ConvertPeaksWorkspaceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertPeaksWorkspaceTest *createSuite() { return new ConvertPeaksWorkspaceTest(); }
  static void destroySuite(ConvertPeaksWorkspaceTest *suite) { delete suite; }

  void test_Name() {
    ConvertPeaksWorkspace alg;
    TS_ASSERT_EQUALS(alg.name(), "ConvertPeaksWorkspace");
  }

  void test_Init() {
    ConvertPeaksWorkspace alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  void test_PeaksWorkspace_to_LeanElasticPeaksWorkspace() {
    PeaksWorkspace_sptr pws = make_pws();
    LeanElasticPeaksWorkspace_sptr lpws = make_lpws();
    g_log.notice() << lpws->getPeak(0).getQSampleFrame() << "\n" << lpws->getPeak(0).getWavelength() << "\n";

    // TODO
    // add calling to the convertor
  }

  void test_LeanElasticPeaksWorkspace_to_PeaksWorkspace() {
    PeaksWorkspace_sptr pws = make_pws();
    LeanElasticPeaksWorkspace_sptr lpws = make_lpws();

    // TODO
    // add calling to the convertor
  }

private:
  // ------------------- //
  // ----- members ----- //
  // ------------------- //
  PeaksWorkspace_sptr make_pws() {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(2, 10);
    inst->setName("TestInstrument");

    auto pw = std::make_shared<PeaksWorkspace>();
    pw->setInstrument(inst);
    std::string val = "test";
    pw->mutableRun().addProperty("TestProp", val);
    Peak pk(inst, 1, 1.0);
    pw->addPeak(pk);
    return pw;
  }

  LeanElasticPeaksWorkspace_sptr make_lpws() {
    auto lpw = std::make_shared<LeanElasticPeaksWorkspace>();
    LeanElasticPeak lpk(V3D(-6.27496, 0.200799, 6.03219), 1.0);
    lpw->addPeak(lpk);
    return lpw;
  }
};
