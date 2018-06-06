#ifndef MANTID_MDUNITVALIDATOR_TEST_H
#define MANTID_MDUNITVALIDATOR_TEST_H

#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/MDFrameValidator.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid::Geometry;
using namespace Mantid::API;

class MDFrameValidatorTest : public CxxTest::TestSuite {
public:
  void testGetType() {
    MDFrameValidator unitValidator(HKL::HKLName);
    TS_ASSERT_EQUALS(unitValidator.getType(), "mdframe");
  }

  void testHKLMDWorkspaceIsValidForValidatorWithHKLFrame() {
    MDFrameValidator frameValidator(HKL::HKLName);

    HKLFrameFactory factory;
    auto frame =
        factory.create(MDFrameArgument{HKL::HKLName, Units::Symbol::RLU});
    auto dim = boost::make_shared<MDHistoDimension>("x", "x", *frame, 0.0f,
                                                    100.0f, 10);
    auto ws = boost::make_shared<MDHistoWorkspaceTester>(dim, dim, dim);
    TS_ASSERT_EQUALS(frameValidator.isValid(ws), "")
  };

  void testHKLMDWorkspaceIsNotValidForValidatorWithQLabFrame() {
    MDFrameValidator frameValidator(QLab::QLabName);

    MDFrameArgument args{HKL::HKLName, Units::Symbol::RLU};
    auto frame = HKLFrameFactory().create(args);
    auto dim = boost::make_shared<MDHistoDimension>("x", "x", *frame, 0.0f,
                                                    100.0f, 10);
    auto ws = boost::make_shared<MDHistoWorkspaceTester>(dim, dim, dim);
    TS_ASSERT_EQUALS(frameValidator.isValid(ws),
                     "MDWorkspace must be in the " + QLab::QLabName + " frame.")
  };

  void testMixedAxisMDWorkspaceIsNotValidForValidatorWithQLabFrame() {
    MDFrameValidator frameValidator(QLab::QLabName);

    MDFrameArgument axisArgs1 {HKL::HKLName, Units::Symbol::RLU};
    MDFrameArgument axisArgs2 {QLab::QLabName, Units::Symbol::InverseAngstrom};

    auto frame1 = HKLFrameFactory().create(axisArgs1);
    auto frame2 = QLabFrameFactory().create(axisArgs2);
    auto dim1 = boost::make_shared<MDHistoDimension>("x", "x", *frame1, 0.0f,
                                                    100.0f, 10);
    auto dim2 = boost::make_shared<MDHistoDimension>("x", "x", *frame1, 0.0f,
                                                     100.0f, 10);
    auto ws = boost::make_shared<MDHistoWorkspaceTester>(dim1, dim2, dim2);
    TS_ASSERT_EQUALS(frameValidator.isValid(ws),
                     "MDWorkspace must be in the " + QLab::QLabName + " frame.")
  };
};

#endif // MANTID_MDUNITVALIDATOR_TEST_H
