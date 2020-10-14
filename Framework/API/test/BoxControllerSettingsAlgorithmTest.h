// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/BoxControllerSettingsAlgorithm.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/HistoWorkspace.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/ScopedFileHelper.h"

#include <utility>

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using ScopedFileHelper::ScopedFile;

//------------------------------------------------------------------------------------------------
/** Concrete declaration of BoxControllerSettingsAlgorithm for testing */
class BoxControllerSettingsAlgorithmImpl
    : public BoxControllerSettingsAlgorithm {
  // Make all the members public so I can test them.
  friend class BoxControllerSettingsAlgorithmTest;

public:
  const std::string name() const override {
    return "BoxControllerSettingsAlgorithmImpl";
  }
  int version() const override { return 1; }
  const std::string category() const override { return "Testing"; }
  const std::string summary() const override { return "Summary of this test."; }
  void init() override {}
  void exec() override {}
};

class BoxControllerSettingsAlgorithmTest : public CxxTest::TestSuite {

private:
  MatrixWorkspace_sptr
  create_workspace_with_splitting_params(int splitThreshold, int splitInto,
                                         int maxRecursionDepth) {
    auto ws = std::make_shared<WorkspaceTester>();
    ws->initialize(1, 2, 1);
    auto baseInstrument =
        ComponentCreationHelper::createTestInstrumentRectangular(6, 1, 0.0);
    auto parameters = std::make_shared<Mantid::Geometry::ParameterMap>();
    parameters->add("double", baseInstrument.get(), "SplitThreshold",
                    static_cast<double>(splitThreshold));
    parameters->add("double", baseInstrument.get(), "SplitInto",
                    static_cast<double>(splitInto));
    parameters->add("double", baseInstrument.get(), "MaxRecursionDepth",
                    static_cast<double>(maxRecursionDepth));
    ws->setInstrument(std::make_shared<Mantid::Geometry::Instrument>(
        baseInstrument, parameters));

    return ws;
  }

public:
  void test_defaultProps() {
    BoxControllerSettingsAlgorithmImpl alg;
    alg.initBoxControllerProps();
    BoxController_sptr bc(new BoxController(3));
    alg.setBoxController(bc);
    TS_ASSERT_EQUALS(bc->getSplitInto(0), 5);
    TS_ASSERT_EQUALS(bc->getSplitThreshold(), 1000);
    TS_ASSERT_EQUALS(bc->getMaxDepth(), 5);
  }

  /** You can change the defaults given to the props */
  void test_initProps_otherDefaults() {
    BoxControllerSettingsAlgorithmImpl alg;
    alg.initBoxControllerProps("6", 1234, 34);
    BoxController_sptr bc(new BoxController(3));
    alg.setBoxController(bc);
    TS_ASSERT_EQUALS(bc->getSplitInto(0), 6);
    TS_ASSERT_EQUALS(bc->getSplitThreshold(), 1234);
    TS_ASSERT_EQUALS(bc->getMaxDepth(), 34);
  }

  void doTest(const BoxController_sptr &bc, const std::string &SplitInto = "",
              const std::string &SplitThreshold = "",
              const std::string &MaxRecursionDepth = "") {
    BoxControllerSettingsAlgorithmImpl alg;
    alg.initBoxControllerProps();
    if (!SplitInto.empty())
      alg.setPropertyValue("SplitInto", SplitInto);
    if (!SplitThreshold.empty())
      alg.setPropertyValue("SplitThreshold", SplitThreshold);
    if (!MaxRecursionDepth.empty())
      alg.setPropertyValue("MaxRecursionDepth", MaxRecursionDepth);
    alg.setBoxController(std::move(bc));
  }

  void test_SplitInto() {
    BoxController_sptr bc(new BoxController(3));
    TSM_ASSERT_THROWS_ANYTHING("Too few parameters", doTest(bc, "5,5"));
    TSM_ASSERT_THROWS_ANYTHING("Too many parameters", doTest(bc, "1,2,3,4"));
    doTest(bc, "4");
    TS_ASSERT_EQUALS(bc->getSplitInto(2), 4);
    doTest(bc, "7,6,5");
    TS_ASSERT_EQUALS(bc->getSplitInto(0), 7);
    TS_ASSERT_EQUALS(bc->getSplitInto(1), 6);
    TS_ASSERT_EQUALS(bc->getSplitInto(2), 5);
  }

  void test_SplitThreshold() {
    BoxController_sptr bc(new BoxController(3));
    TSM_ASSERT_THROWS_ANYTHING("Negative threshold", doTest(bc, "", "-3"));
    doTest(bc, "", "1234");
    TS_ASSERT_EQUALS(bc->getSplitThreshold(), 1234);
  }

  void test_MaxRecursionDepth() {
    BoxController_sptr bc(new BoxController(3));
    TSM_ASSERT_THROWS_ANYTHING("Negative MaxRecursionDepth",
                               doTest(bc, "", "", "-1"));
    doTest(bc, "", "", "34");
    TS_ASSERT_EQUALS(bc->getMaxDepth(), 34);
  }

  void test_take_instrument_parameters() {
    const int splitInto = 4;
    const int splitThreshold = 16;
    const int maxRecursionDepth = 5;

    // Workspace has instrument has parameters for all box splitting parameters.
    auto ws = create_workspace_with_splitting_params(splitThreshold, splitInto,
                                                     maxRecursionDepth);

    BoxController_sptr bc(new BoxController(1));

    BoxControllerSettingsAlgorithmImpl alg;
    alg.initBoxControllerProps();
    /* Note, not properties are set, so the algorithm will have defaults set.
    and should therefore look to
    pick-up any available in the instrument parameters.*/
    alg.setBoxController(bc, ws->getInstrument());

    int actualSplitThreshold = alg.getProperty("SplitThreshold");
    TS_ASSERT_EQUALS(splitThreshold, actualSplitThreshold);

    std::vector<int> actualSplitInto = alg.getProperty("SplitInto");
    TS_ASSERT_EQUALS(bc->getNDims(), actualSplitInto.size());
    std::vector<int> expectedSplitInto(bc->getNDims(), splitInto);
    TS_ASSERT_EQUALS(expectedSplitInto, actualSplitInto);

    int actualMaxRecursionDepth = alg.getProperty("MaxRecursionDepth");
    TS_ASSERT_EQUALS(maxRecursionDepth, actualMaxRecursionDepth);
  }

  // Test that the user providied values for spliting have precedence.
  void test_ignore_instrument_parameters() {
    const int splitInto = 8;
    const int splitThreshold = 16;
    const int maxRecursionDepth = 5;

    // Workspace has instrument has parameters for all box splitting parameters.
    auto ws = create_workspace_with_splitting_params(splitThreshold, splitInto,
                                                     maxRecursionDepth);

    BoxController_sptr bc(new BoxController(1));

    // Create splitting parameters that are not default and not the same as
    // those on the instrument parameters.
    const std::vector<int> nonDefaultSplitInto =
        std::vector<int>(bc->getNDims(), splitInto + 1);
    const int nonDefaultSplitThreshold = splitThreshold + 1;
    const int nonDefaultMaxRecursionDepth = maxRecursionDepth + 1;

    BoxControllerSettingsAlgorithmImpl alg;
    alg.initBoxControllerProps();
    alg.setProperty("SplitInto", nonDefaultSplitInto);
    alg.setProperty("SplitThreshold", nonDefaultSplitThreshold);
    alg.setProperty("MaxRecursionDepth", nonDefaultMaxRecursionDepth);
    alg.setBoxController(bc, ws->getInstrument());

    int actualSplitThreshold = alg.getProperty("SplitThreshold");
    TS_ASSERT_EQUALS(nonDefaultSplitThreshold, actualSplitThreshold);

    std::vector<int> actualSplitInto = alg.getProperty("SplitInto");
    TS_ASSERT_EQUALS(bc->getNDims(), actualSplitInto.size());
    TS_ASSERT_EQUALS(nonDefaultSplitInto, actualSplitInto);

    int actualMaxRecursionDepth = alg.getProperty("MaxRecursionDepth");
    TS_ASSERT_EQUALS(nonDefaultMaxRecursionDepth, actualMaxRecursionDepth);
  }

  void test_with_no_instrument_parameters() {
    // Create a workspace with an instrument, but no instrument parameters for
    // box splitting.
    auto ws = std::make_shared<WorkspaceTester>();
    ws->initialize(1, 2, 1);
    ws->setInstrument(
        ComponentCreationHelper::createTestInstrumentRectangular(6, 1, 0));

    BoxController_sptr bc(new BoxController(1));

    BoxControllerSettingsAlgorithmImpl alg;
    alg.initBoxControllerProps();
    // Note that no properties are actually set. All properties should fall-back
    // to their default values.
    alg.setRethrows(true);
    TSM_ASSERT_THROWS_NOTHING("Lack of specific instrument parameters should "
                              "not cause algorithm to fail.",
                              alg.setBoxController(bc, ws->getInstrument()));

    // Check that the properties are unaffected. Should just reflect the
    // defaults.
    Mantid::Kernel::Property *p = alg.getProperty("SplitThreshold");
    TS_ASSERT(p->isDefault());
    p = alg.getProperty("SplitInto");
    TS_ASSERT(p->isDefault());
    p = alg.getProperty("MaxRecursionDepth");
    TS_ASSERT(p->isDefault());
  }
};
