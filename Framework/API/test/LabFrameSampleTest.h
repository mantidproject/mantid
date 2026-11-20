// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/LabFrameSample.h"
#include "MantidAPI/Run.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Quat.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using Mantid::Kernel::Matrix;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;

namespace {
// 40 mm along x, 4 mm along z, so a quarter turn about y swaps the two.
const std::string PLATE_XML = "<cuboid id=\"plate\">"
                              "<left-front-bottom-point x=\"-0.02\" y=\"-0.02\" z=\"-0.002\"/>"
                              "<left-front-top-point x=\"-0.02\" y=\"0.02\" z=\"-0.002\"/>"
                              "<left-back-bottom-point x=\"-0.02\" y=\"-0.02\" z=\"0.002\"/>"
                              "<right-front-bottom-point x=\"0.02\" y=\"-0.02\" z=\"-0.002\"/>"
                              "</cuboid>";
const Matrix<double> QUARTER_TURN_ABOUT_Y(Quat(90.0, V3D(0, 1, 0)).getRotation());

double xWidth(const IObject &shape) { return shape.getBoundingBox().width().X(); }
} // namespace

class LabFrameSampleTest : public CxxTest::TestSuite {
public:
  void test_shape_in_its_own_frame_is_rotated_by_the_goniometer() {
    ExperimentInfo expt = experiment(PLATE_XML);
    TS_ASSERT_DELTA(xWidth(*labFrameSampleShape(expt)), 0.004, 1e-9);
  }

  void test_shape_already_in_the_lab_frame_is_not_rotated_again() {
    ExperimentInfo expt = experiment(ShapeFactory().addGoniometerTag(QUARTER_TURN_ABOUT_Y, PLATE_XML));
    TS_ASSERT_EQUALS(labFrameSampleShape(expt), expt.sample().getShapePtr());
    TS_ASSERT_DELTA(xWidth(*labFrameSampleShape(expt)), 0.004, 1e-9);
  }

  void test_sample_turns_with_the_mounted_parts_of_its_environment() {
    ExperimentInfo expt = experiment(PLATE_XML);
    addEnvironment(expt);

    const auto sample = labFrameSample(expt);

    TS_ASSERT_DELTA(xWidth(sample.getShape()), 0.004, 1e-9);
    TS_ASSERT_DELTA(xWidth(sample.getEnvironment().getContainer()), 0.004, 1e-9);
    TS_ASSERT_EQUALS(sample.getEnvironment().getComponentPtr(1), expt.sample().getEnvironment().getComponentPtr(1));
    // and the workspace's own environment is untouched
    TS_ASSERT_DELTA(xWidth(expt.sample().getEnvironment().getContainer()), 0.04, 1e-9);
  }

  void test_a_baked_sample_and_its_stored_can_both_end_up_in_the_lab_frame() {
    // as CopySample and a 3MF file leave them: the sample already turned, the can as defined
    ExperimentInfo expt = experiment(ShapeFactory().addGoniometerTag(QUARTER_TURN_ABOUT_Y, PLATE_XML));
    addEnvironment(expt);

    const auto sample = labFrameSample(expt);

    TS_ASSERT_DELTA(xWidth(sample.getShape()), 0.004, 1e-9);
    TS_ASSERT_DELTA(xWidth(sample.getEnvironment().getContainer()), 0.004, 1e-9);
  }

  void test_an_identity_goniometer_shares_the_environment() {
    ExperimentInfo expt = experiment(PLATE_XML);
    addEnvironment(expt);
    expt.mutableRun().mutableGoniometer().setR(Matrix<double>(3, 3, true));

    TS_ASSERT_EQUALS(&labFrameSample(expt).getEnvironment(), &expt.sample().getEnvironment());
  }

  void test_a_lab_frame_view_is_rebuilt_when_the_goniometer_is_reset() {
    ExperimentInfo expt = experiment(PLATE_XML);
    addEnvironment(expt);
    expt.mutableSample() = labFrameSample(expt);
    expt.mutableRun().mutableGoniometer().setR(Matrix<double>(3, 3, true));

    TS_ASSERT_DELTA(xWidth(labFrameSample(expt).getEnvironment().getContainer()), 0.04, 1e-9);
  }

  void test_lab_frame_sample_keeps_everything_but_the_shape() {
    ExperimentInfo expt = experiment(PLATE_XML);
    expt.mutableSample().setName("plate");
    const auto sample = labFrameSample(expt);
    TS_ASSERT_EQUALS(sample.getName(), "plate");
    TS_ASSERT_DELTA(xWidth(sample.getShape()), 0.004, 1e-9);
    TS_ASSERT_DELTA(xWidth(expt.sample().getShape()), 0.04, 1e-9);
  }

private:
  ExperimentInfo experiment(const std::string &shapeXML) {
    ExperimentInfo expt;
    expt.mutableSample().setShape(ShapeFactory().createShape(shapeXML));
    expt.mutableRun().mutableGoniometer().setR(QUARTER_TURN_ABOUT_Y);
    return expt;
  }

  /// A plate-shaped can, so a turn shows, and a fixed component.
  void addEnvironment(ExperimentInfo &expt) {
    auto kit =
        std::make_unique<SampleEnvironment>("kit", std::make_shared<Container>(ShapeFactory().createShape(PLATE_XML)));
    kit->add(ShapeFactory().createShape(ComponentCreationHelper::sphereXML(0.05, V3D(0, 0, 0), "shield")));
    expt.mutableSample().setEnvironment(std::move(kit));
  }
};
