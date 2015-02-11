#ifndef MANTIDAPI_PEAKTRANSFORMQLAB_TEST_H_
#define MANTIDAPI_PEAKTRANSFORMQLAB_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/PeakTransformQLab.h"
#include "MockObjects.h"
#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid;
using Mantid::Kernel::V3D;
using namespace testing;

class PeakTransformQLabTest : public CxxTest::TestSuite
{

public:

  void test_throws_with_unknown_xLabel()
  {
    TS_ASSERT_THROWS(PeakTransformQLab("?", "Q_lab_y"), PeakTransformException&);
  }

  void test_throws_with_unknown_yLabel()
  {
    TS_ASSERT_THROWS(PeakTransformQLab("Q_lab_x", "?"), PeakTransformException&);
  }

  void test_default_transform()
  {
    PeakTransformQLab transform; // Should be equivalent to constructing transform("Q_lab_x", "Q_lab_y")
    V3D original(0, 1, 2);
    V3D transformed = transform.transform(original);
    TS_ASSERT_EQUALS(transformed.X(), original.X());
    TS_ASSERT_EQUALS(transformed.Y(), original.Y());
    TS_ASSERT_EQUALS(transformed.Z(), original.Z());

    TSM_ASSERT("Wrong free peak axis.", boost::regex_match("Q_lab_z", transform.getFreePeakAxisRegex()));
  }

  void test_maps_to_q_lab_on_ipeak()
  {
    // Create a peak.
    MockIPeak mockPeak;
    EXPECT_CALL(mockPeak, getQLabFrame()).WillOnce(Return(V3D())); // Should RUN getQLabFrame!

    // Use the transform on the peak.
    PeakTransformQLab transform("Q_lab_x", "Q_lab_y");
    transform.transformPeak(mockPeak);

    // Check that the transform read the right coordinates off the peak object.
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPeak));
  }

  void test_transformQxQyQz()
  {
    PeakTransformQLab transform("Q_lab_x", "Q_lab_y");
    V3D original(0, 1, 2);
    V3D transformed = transform.transform(original);
    TS_ASSERT_EQUALS(transformed.X(), original.X());
    TS_ASSERT_EQUALS(transformed.Y(), original.Y());
    TS_ASSERT_EQUALS(transformed.Z(), original.Z());

    TSM_ASSERT("Wrong free peak axis.", boost::regex_match("Q_lab_z", transform.getFreePeakAxisRegex()));
  }

  void test_transformQxQzQy()
  {
    PeakTransformQLab transform("Q_lab_x", "Q_lab_z");
    V3D original(0, 1, 2);
    V3D transformed = transform.transform(original);
    TS_ASSERT_EQUALS(transformed.X(), original.X()); // X -> Q_lab_x
    TS_ASSERT_EQUALS(transformed.Y(), original.Z()); // Y -> Q_lab_z
    TS_ASSERT_EQUALS(transformed.Z(), original.Y()); // Z -> Q_lab_y

   TSM_ASSERT("Wrong free peak axis.",  boost::regex_match("Q_lab_y", transform.getFreePeakAxisRegex()));
  }

  void test_transformQzQyQx()
  {
    PeakTransformQLab transform("Q_lab_z", "Q_lab_y");
    V3D original(0, 1, 2);
    V3D transformed = transform.transform(original);
    TS_ASSERT_EQUALS(transformed.X(), original.Z()); // X -> Q_lab_z
    TS_ASSERT_EQUALS(transformed.Y(), original.Y()); // Y -> Q_lab_y
    TS_ASSERT_EQUALS(transformed.Z(), original.X()); // Z -> Q_lab_x

    TSM_ASSERT("Wrong free peak axis.", boost::regex_match("Q_lab_x", transform.getFreePeakAxisRegex()));
  }

void test_transformQzQxQy()
{
  PeakTransformQLab transform("Q_lab_z", "Q_lab_x");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Z()); // X -> Q_lab_z
  TS_ASSERT_EQUALS(transformed.Y(), original.X()); // Y -> Q_lab_x
  TS_ASSERT_EQUALS(transformed.Z(), original.Y()); // Z -> Q_lab_y

  TSM_ASSERT("Wrong free peak axis.", boost::regex_match("Q_lab_y", transform.getFreePeakAxisRegex()));
}

void test_transformQyQzQx()
{
  PeakTransformQLab transform("Q_lab_y", "Q_lab_z");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Y()); // X -> K
  TS_ASSERT_EQUALS(transformed.Y(), original.Z()); // Y -> L
  TS_ASSERT_EQUALS(transformed.Z(), original.X()); // Z -> H

  TSM_ASSERT("Wrong free peak axis.", boost::regex_match("Q_lab_x", transform.getFreePeakAxisRegex()));
}

void test_transformQyQxQz()
{
  PeakTransformQLab transform("Q_lab_y", "Q_lab_x");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Y()); // X -> K
  TS_ASSERT_EQUALS(transformed.Y(), original.X()); // Y -> H
  TS_ASSERT_EQUALS(transformed.Z(), original.Z()); // Z -> L

  TSM_ASSERT("Wrong free peak axis.", boost::regex_match("Q_lab_z", transform.getFreePeakAxisRegex()));
}

void test_copy_construction()
{
  PeakTransformQLab A("Q_lab_x", "Q_lab_z");
  PeakTransformQLab B(A);

  // Test indirectly via what the transformations produce.
  V3D productA = A.transform(V3D(0, 1, 2));
  V3D productB = B.transform(V3D(0, 1, 2));
  TS_ASSERT_EQUALS(productA, productB);  
  // Test indirectly via the free regex.
  boost::regex regexA = A.getFreePeakAxisRegex();
  boost::regex regexB = B.getFreePeakAxisRegex();
  TS_ASSERT_EQUALS(regexA, regexB);
}


void test_assigment()
{
  PeakTransformQLab A("Q_lab_x", "Q_lab_z");
  PeakTransformQLab B("Q_lab_y", "Q_lab_x");
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

void test_clone()
{
  PeakTransformQLab A("Q_lab_x", "Q_lab_z");
  PeakTransform_sptr clone = A.clone();

  TSM_ASSERT("Clone product is the wrong type.", boost::dynamic_pointer_cast<PeakTransformQLab>(clone) != NULL);

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
void test_factory()
{
  // Create the benchmark.
  PeakTransform_sptr expectedProduct = boost::make_shared<PeakTransformQLab>("Q_lab_x", "Q_lab_y");

  // Use the factory to create a product.
  PeakTransformQLabFactory factory;
  PeakTransform_sptr product = factory.createDefaultTransform();

  // Check the type of the output product object.
  TSM_ASSERT("Factory product is the wrong type.", boost::dynamic_pointer_cast<PeakTransformQLab>(product) != NULL);

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

void test_getFriendlyName()
{
  PeakTransformQLab transform;
  TS_ASSERT_EQUALS(PeakTransformQLab::name(), transform.getFriendlyName());
  TS_ASSERT_EQUALS("Q (lab frame)", transform.getFriendlyName());
}

void test_getCoordinateSystem()
{
  PeakTransformQLab transform;
  TS_ASSERT_EQUALS(Mantid::Kernel::QLab, transform.getCoordinateSystem())
}


};
#endif

//end MANTIDAPI_PEAKTRANSFORMQLAB_TEST_H_
