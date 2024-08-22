// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <memory>

#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/MDFrameValidator.h"
#include "MantidFrameworkTestHelpers/FakeObjects.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidKernel/UnitLabelTypes.h"

using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Kernel;

class MDFrameValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDFrameValidatorTest *createSuite() { return new MDFrameValidatorTest(); }
  static void destroySuite(MDFrameValidatorTest *suite) { delete suite; }

  void testGetType() {
    MDFrameValidator unitValidator(Mantid::Geometry::HKL::HKLName);
    TS_ASSERT_EQUALS(unitValidator.getType(), "mdframe");
  }

  void testHKLMDWorkspaceIsValidForValidatorWithHKLFrame() {
    MDFrameValidator frameValidator(Mantid::Geometry::HKL::HKLName);

    HKLFrameFactory factory;
    auto frame = factory.create(MDFrameArgument{Mantid::Geometry::HKL::HKLName, Units::Symbol::RLU});
    auto dim = std::make_shared<MDHistoDimension>("x", "x", *frame, 0.0f, 100.0f, 10);
    auto ws = std::make_shared<MDHistoWorkspaceTester>(dim, dim, dim);
    TS_ASSERT_EQUALS(frameValidator.isValid(ws), "")
  };

  void testHKLMDWorkspaceIsNotValidForValidatorWithQLabFrame() {
    MDFrameValidator frameValidator(QLab::QLabName);

    MDFrameArgument args{Mantid::Geometry::HKL::HKLName, Units::Symbol::RLU};
    auto frame = HKLFrameFactory().create(args);
    auto dim = std::make_shared<MDHistoDimension>("x", "x", *frame, 0.0f, 100.0f, 10);
    auto ws = std::make_shared<MDHistoWorkspaceTester>(dim, dim, dim);
    TS_ASSERT_EQUALS(frameValidator.isValid(ws), "MDWorkspace must be in the " + QLab::QLabName + " frame.")
  };

  void testMixedAxisMDWorkspaceIsNotValidForValidatorWithQLabFrame() {
    MDFrameValidator frameValidator(QLab::QLabName);

    MDFrameArgument axisArgs1{Mantid::Geometry::HKL::HKLName, Units::Symbol::RLU};
    MDFrameArgument axisArgs2{QLab::QLabName, Units::Symbol::InverseAngstrom};

    auto frame1 = HKLFrameFactory().create(axisArgs1);
    auto frame2 = QLabFrameFactory().create(axisArgs2);
    auto dim1 = std::make_shared<MDHistoDimension>("x", "x", *frame1, 0.0f, 100.0f, 10);
    auto dim2 = std::make_shared<MDHistoDimension>("x", "x", *frame1, 0.0f, 100.0f, 10);
    auto ws = std::make_shared<MDHistoWorkspaceTester>(dim1, dim2, dim2);
    TS_ASSERT_EQUALS(frameValidator.isValid(ws), "MDWorkspace must be in the " + QLab::QLabName + " frame.")
  };
};
