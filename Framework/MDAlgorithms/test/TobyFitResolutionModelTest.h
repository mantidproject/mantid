// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_TOBYFITRESOLUTIONMODELTEST_H_
#define MANTID_MDALGORITHMS_TOBYFITRESOLUTIONMODELTEST_H_

#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitResolutionModel.h"
#include "MantidMDAlgorithms/Quantification/ResolutionConvolvedCrossSection.h"

#include "MDFittingTestHelpers.h"
#include <cxxtest/TestSuite.h>

class TobyFitResolutionModelTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TobyFitResolutionModelTest *createSuite() {
    return new TobyFitResolutionModelTest();
  }
  static void destroySuite(TobyFitResolutionModelTest *suite) { delete suite; }

  TobyFitResolutionModelTest() {
    using namespace Mantid::MDAlgorithms;
    ForegroundModelFactory::Instance().subscribe<FakeForegroundModel>(
        "FakeForegroundModel");
  }

  ~TobyFitResolutionModelTest() override {
    using namespace Mantid::MDAlgorithms;
    ForegroundModelFactory::Instance().unsubscribe("FakeForegroundModel");
  }

  void test_Construction_With_Unknown_Model_Throws_Invalid_Argument() {
    using namespace Mantid::MDAlgorithms;
    ResolutionConvolvedCrossSection *conv = new ResolutionConvolvedCrossSection;

    TS_ASSERT_THROWS(TobyFitResolutionModel(*conv, "_NotAKnownModel"),
                     const std::invalid_argument &);

    delete conv;
  }

  void test_Construction_With_Valid_Arguments_Creates_Object() {
    using namespace Mantid::MDAlgorithms;
    ResolutionConvolvedCrossSection *conv = new ResolutionConvolvedCrossSection;

    TS_ASSERT_THROWS_NOTHING(
        TobyFitResolutionModel(*conv, "FakeForegroundModel"));

    delete conv;
  }

  void test_uninitialized_object_has_no_attributes() {
    using namespace Mantid::MDAlgorithms;
    TobyFitResolutionModel mdconvolution;

    TS_ASSERT_EQUALS(mdconvolution.nAttributes(), 0);
  }

  void test_initialized_object_has_expected_number_of_attributes() {
    using namespace Mantid::MDAlgorithms;
    TobyFitResolutionModel mdconvolution;
    mdconvolution.initialize();

    TS_ASSERT_EQUALS(mdconvolution.nAttributes(), 14);
  }

  void test_crossSection_Returns_Expected_Value_For_Specific_Parameters() {
    using namespace Mantid::MDAlgorithms;
    ResolutionConvolvedCrossSection *conv = new ResolutionConvolvedCrossSection;
    TobyFitResolutionModel mcResConvoution(*conv, "FakeForegroundModel");

    delete conv;
  }
};

#endif /* MANTID_MDALGORITHMS_TOBYFITRESOLUTIONMODELTEST_H_ */
