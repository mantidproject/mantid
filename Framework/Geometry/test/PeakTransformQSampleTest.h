// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/PeakTransformQSample.h"
#include "MockObjects.h"
#include <cxxtest/TestSuite.h>
#include <memory>

using namespace Mantid::Geometry;
using namespace Mantid;
using Mantid::Kernel::V3D;
using namespace testing;

namespace boost {
template <class CharType, class CharTrait>
std::basic_ostream<CharType, CharTrait> &operator<<(std::basic_ostream<CharType, CharTrait> &out,
                                                    std::optional<double> const &maybe) {
  if (maybe)
    out << maybe;
  return out;
}
} // namespace boost

class PeakTransformQSampleTest : public CxxTest::TestSuite {

public:
  void test_throws_with_unknown_xLabel() {
    TS_ASSERT_THROWS(PeakTransformQSample("?", "Q_sample_y"), PeakTransformException &);
  }

  void test_throws_with_unknown_yLabel() {
    TS_ASSERT_THROWS(PeakTransformQSample("Q_sample_x", "?"), PeakTransformException &);
  }

  void test_default_transform() {
    PeakTransformQSample transform; // Should be equivalent to constructing
                                    // transform("Q_sample_x", "Q_sample_y")
    V3D original(0, 1, 2);
    V3D transformed = transform.transform(original);
    TS_ASSERT_EQUALS(transformed.X(), original.X());
    TS_ASSERT_EQUALS(transformed.Y(), original.Y());
    TS_ASSERT_EQUALS(transformed.Z(), original.Z());

    TSM_ASSERT("Wrong free peak axis.", boost::regex_match("Q_sample_z", transform.getFreePeakAxisRegex()));
  }

  void test_maps_to_q_sample_on_ipeak() {
    // Create a peak.
    MockIPeak mockPeak;
    EXPECT_CALL(mockPeak, getQSampleFrame()).WillOnce(Return(V3D())); // Should RUN getQSampleFrame!

    // Use the transform on the peak.
    PeakTransformQSample transform("Q_sample_x", "Q_sample_y");
    transform.transformPeak(mockPeak);

    // Check that the transform read the right coordinates off the peak object.
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPeak));
  }

  void test_transformQxQyQz() {
    PeakTransformQSample transform("Q_sample_x", "Q_sample_y");
    V3D original(0, 1, 2);
    V3D transformed = transform.transform(original);
    TS_ASSERT_EQUALS(transformed.X(), original.X());
    TS_ASSERT_EQUALS(transformed.Y(), original.Y());
    TS_ASSERT_EQUALS(transformed.Z(), original.Z());

    TSM_ASSERT("Wrong free peak axis.", boost::regex_match("Q_sample_z", transform.getFreePeakAxisRegex()));
  }

  void test_transformQxQzQy() {
    PeakTransformQSample transform("Q_sample_x", "Q_sample_z");
    V3D original(0, 1, 2);
    V3D transformed = transform.transform(original);
    TS_ASSERT_EQUALS(transformed.X(), original.X()); // X -> Q_sample_x
    TS_ASSERT_EQUALS(transformed.Y(), original.Z()); // Y -> Q_sample_z
    TS_ASSERT_EQUALS(transformed.Z(), original.Y()); // Z -> Q_sample_y

    TSM_ASSERT("Wrong free peak axis.", boost::regex_match("Q_sample_y", transform.getFreePeakAxisRegex()));
  }

  void test_transformQzQyQx() {
    PeakTransformQSample transform("Q_sample_z", "Q_sample_y");
    V3D original(0, 1, 2);
    V3D transformed = transform.transform(original);
    TS_ASSERT_EQUALS(transformed.X(), original.Z()); // X -> Q_sample_z
    TS_ASSERT_EQUALS(transformed.Y(), original.Y()); // Y -> Q_sample_y
    TS_ASSERT_EQUALS(transformed.Z(), original.X()); // Z -> Q_sample_x

    TSM_ASSERT("Wrong free peak axis.", boost::regex_match("Q_sample_x", transform.getFreePeakAxisRegex()));
  }

  void test_transformQzQxQy() {
    PeakTransformQSample transform("Q_sample_z", "Q_sample_x");
    V3D original(0, 1, 2);
    V3D transformed = transform.transform(original);
    TS_ASSERT_EQUALS(transformed.X(), original.Z()); // X -> Q_sample_z
    TS_ASSERT_EQUALS(transformed.Y(), original.X()); // Y -> Q_sample_x
    TS_ASSERT_EQUALS(transformed.Z(), original.Y()); // Z -> Q_sample_y

    TSM_ASSERT("Wrong free peak axis.", boost::regex_match("Q_sample_y", transform.getFreePeakAxisRegex()));
  }

  void test_transformQyQzQx() {
    PeakTransformQSample transform("Q_sample_y", "Q_sample_z");
    V3D original(0, 1, 2);
    V3D transformed = transform.transform(original);
    TS_ASSERT_EQUALS(transformed.X(), original.Y()); // X -> K
    TS_ASSERT_EQUALS(transformed.Y(), original.Z()); // Y -> L
    TS_ASSERT_EQUALS(transformed.Z(), original.X()); // Z -> H

    TSM_ASSERT("Wrong free peak axis.", boost::regex_match("Q_sample_x", transform.getFreePeakAxisRegex()));
  }

  void test_transformQyQxQz() {
    PeakTransformQSample transform("Q_sample_y", "Q_sample_x");
    V3D original(0, 1, 2);
    V3D transformed = transform.transform(original);
    TS_ASSERT_EQUALS(transformed.X(), original.Y()); // X -> K
    TS_ASSERT_EQUALS(transformed.Y(), original.X()); // Y -> H
    TS_ASSERT_EQUALS(transformed.Z(), original.Z()); // Z -> L

    TSM_ASSERT("Wrong free peak axis.", boost::regex_match("Q_sample_z", transform.getFreePeakAxisRegex()));
  }

  void test_copy_construction() {
    PeakTransformQSample A("Q_sample_x", "Q_sample_z");
    PeakTransformQSample B(A);

    // Test indirectly via what the transformations produce.
    V3D productA = A.transform(V3D(0, 1, 2));
    V3D productB = B.transform(V3D(0, 1, 2));
    TS_ASSERT_EQUALS(productA, productB);
    // Test indirectly via the free regex.
    boost::regex regexA = A.getFreePeakAxisRegex();
    boost::regex regexB = B.getFreePeakAxisRegex();
    TS_ASSERT_EQUALS(regexA, regexB);
  }

  void test_assigment() {
    PeakTransformQSample A("Q_sample_x", "Q_sample_z");
    PeakTransformQSample B("Q_sample_y", "Q_sample_x");
    A = B;

    // Test indirectly via what the transformations produce.
    V3D productA = A.transform(V3D(0, 1, 2));
    V3D productB = B.transform(V3D(0, 1, 2));
    TS_ASSERT_EQUALS(productA, productB);
    // Test indirectly via the free regex.
    boost::regex regexA = A.getFreePeakAxisRegex();
    boost::regex regexB = B.getFreePeakAxisRegex();
    TS_ASSERT_EQUALS(regexA, regexB);
  }

  void test_clone() {
    PeakTransformQSample A("Q_sample_x", "Q_sample_z");
    PeakTransform_sptr clone = A.clone();

    TSM_ASSERT("Clone product is the wrong type.", std::dynamic_pointer_cast<PeakTransformQSample>(clone) != nullptr);

    // Test indirectly via what the transformations produce.
    V3D productA = A.transform(V3D(0, 1, 2));
    V3D productB = clone->transform(V3D(0, 1, 2));
    TS_ASSERT_EQUALS(productA, productB);
    // Test indirectly via the free regex.
    boost::regex regexA = A.getFreePeakAxisRegex();
    boost::regex regexB = clone->getFreePeakAxisRegex();
    TS_ASSERT_EQUALS(regexA, regexB);
  }

  // Test the factory generated about this type.
  void test_factory() {
    // Create the benchmark.
    PeakTransform_sptr expectedProduct = std::make_shared<PeakTransformQSample>("Q_sample_x", "Q_sample_y");

    // Use the factory to create a product.
    PeakTransformQSampleFactory factory;
    PeakTransform_sptr product = factory.createDefaultTransform();

    // Check the type of the output product object.
    TSM_ASSERT("Factory product is the wrong type.",
               std::dynamic_pointer_cast<PeakTransformQSample>(product) != nullptr);

    // Now test that the benchmark and the factory product are equivalent.
    // Test indirectly via what the transformations produce.
    V3D productA = expectedProduct->transform(V3D(0, 1, 2));
    V3D productB = product->transform(V3D(0, 1, 2));
    TS_ASSERT_EQUALS(productA, productB);
    // Test indirectly via the free regex.
    boost::regex regexA = expectedProduct->getFreePeakAxisRegex();
    boost::regex regexB = product->getFreePeakAxisRegex();
    TS_ASSERT_EQUALS(regexA, regexB);
  }

  void test_getFriendlyName() {
    PeakTransformQSample transform;
    TS_ASSERT_EQUALS(PeakTransformQSample::name(), transform.getFriendlyName());
    TS_ASSERT_EQUALS("Q (sample frame)", transform.getFriendlyName());
  }

  void test_getCoordinateSystem() {
    PeakTransformQSample transform;
    TS_ASSERT_EQUALS(Mantid::Kernel::QSample, transform.getCoordinateSystem())
  }
};
