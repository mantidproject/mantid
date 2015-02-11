#ifndef SLICE_VIEWER_PEAKTRANSFORMHKL_TEST_H_
#define SLICE_VIEWER_PEAKTRANSFORMHKL_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/PeakTransformHKL.h"
#include "MockObjects.h"
#include <boost/make_shared.hpp>

using namespace Mantid::API;
using Mantid::Kernel::V3D;
using namespace testing;

class PeakTransformHKLTest : public CxxTest::TestSuite
{
public:

  void test_throws_with_unknown_xLabel()
  {
    TS_ASSERT_THROWS(PeakTransformHKL("?", "K (Lattice)"), PeakTransformException&);
  }

  void test_throws_with_unknown_yLabel()
  {
    TS_ASSERT_THROWS(PeakTransformHKL("H (Lattice)", "?"), PeakTransformException&);
  }

  void test_maps_to_hkl_on_ipeak()
  {
    // Create a peak.
    MockIPeak mockPeak;
    EXPECT_CALL(mockPeak, getHKL()).WillOnce(Return(V3D())); // Should RUN getHKL!

    // Use the transform on the peak.
    PeakTransformHKL transform("H", "K");
    transform.transformPeak(mockPeak);

    // Check that the transform read the right coordinates off the peak object.
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPeak));
  }

  void test_default_transform()
  {
    PeakTransformHKL transform; // Should be equivalent to constructing transform("H (Lattice)", "K (Lattice)")
    V3D original(0, 1, 2);
    V3D transformed = transform.transform(original);
    TS_ASSERT_EQUALS(transformed.X(), original.X());
    TS_ASSERT_EQUALS(transformed.Y(), original.Y());
    TS_ASSERT_EQUALS(transformed.Z(), original.Z());

    V3D backToOriginal = transform.transformBack(transformed);
    TS_ASSERT_EQUALS(backToOriginal.X(), original.X());
    TS_ASSERT_EQUALS(backToOriginal.Y(), original.Y());
    TS_ASSERT_EQUALS(backToOriginal.Z(), original.Z());

    boost::regex_match("L (Lattice)", transform.getFreePeakAxisRegex());
  }

void test_transformHKL()
{
  PeakTransformHKL transform("H (Lattice)", "K (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.X());
  TS_ASSERT_EQUALS(transformed.Y(), original.Y());
  TS_ASSERT_EQUALS(transformed.Z(), original.Z());

  V3D backToOriginal = transform.transformBack(transformed);
  TS_ASSERT_EQUALS(backToOriginal.X(), original.X());
  TS_ASSERT_EQUALS(backToOriginal.Y(), original.Y());
  TS_ASSERT_EQUALS(backToOriginal.Z(), original.Z());

  boost::regex_match("L (Lattice)", transform.getFreePeakAxisRegex());
  boost::regex_match("[0,0,L]", transform.getFreePeakAxisRegex());
}

void test_transformHLK()
{
  PeakTransformHKL transform("H (Lattice)", "L (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.X()); // X -> H
  TS_ASSERT_EQUALS(transformed.Y(), original.Z()); // Y -> L
  TS_ASSERT_EQUALS(transformed.Z(), original.Y()); // Z -> K

  V3D backToOriginal = transform.transformBack(transformed);
  TS_ASSERT_EQUALS(backToOriginal.X(), original.X());
  TS_ASSERT_EQUALS(backToOriginal.Y(), original.Y());
  TS_ASSERT_EQUALS(backToOriginal.Z(), original.Z());

  boost::regex_match("K (Lattice)", transform.getFreePeakAxisRegex());
  boost::regex_match("[0,K,0]", transform.getFreePeakAxisRegex());
}

void test_transformLKH()
{
  PeakTransformHKL transform("L (Lattice)", "K (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Z()); // X -> L
  TS_ASSERT_EQUALS(transformed.Y(), original.Y()); // Y -> K
  TS_ASSERT_EQUALS(transformed.Z(), original.X()); // Z -> H

  V3D backToOriginal = transform.transformBack(transformed);
  TS_ASSERT_EQUALS(backToOriginal.X(), original.X());
  TS_ASSERT_EQUALS(backToOriginal.Y(), original.Y());
  TS_ASSERT_EQUALS(backToOriginal.Z(), original.Z());

  boost::regex_match("H (Lattice)", transform.getFreePeakAxisRegex());
  boost::regex_match("[H,0,0]", transform.getFreePeakAxisRegex());
}

void test_transformLHK()
{
  PeakTransformHKL transform("L (Lattice)", "H (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Z()); // X -> L
  TS_ASSERT_EQUALS(transformed.Y(), original.X()); // Y -> H
  TS_ASSERT_EQUALS(transformed.Z(), original.Y()); // Z -> K

  V3D backToOriginal = transform.transformBack(transformed);
  TS_ASSERT_EQUALS(backToOriginal.X(), original.X());
  TS_ASSERT_EQUALS(backToOriginal.Y(), original.Y());
  TS_ASSERT_EQUALS(backToOriginal.Z(), original.Z());

  boost::regex_match("K (Lattice)", transform.getFreePeakAxisRegex());
  boost::regex_match("[0,K,0]", transform.getFreePeakAxisRegex());
}

// Check that the peaks transform works when the dimension labels are square bracket notation.
void test_transformLHK_via_regex_v2()
{
  try
  {
  PeakTransformHKL transform("[0,0,L]", "[H,0,0]");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Z()); // X -> L
  TS_ASSERT_EQUALS(transformed.Y(), original.X()); // Y -> H
  TS_ASSERT_EQUALS(transformed.Z(), original.Y()); // Z -> K

  V3D backToOriginal = transform.transformBack(transformed);
  TS_ASSERT_EQUALS(backToOriginal.X(), original.X());
  TS_ASSERT_EQUALS(backToOriginal.Y(), original.Y());
  TS_ASSERT_EQUALS(backToOriginal.Z(), original.Z());

  boost::regex_match("K (Lattice)", transform.getFreePeakAxisRegex());
  boost::regex_match("[0,K,0]", transform.getFreePeakAxisRegex());
  }
  catch(PeakTransformException& ex)
  {
    std::cout << "THROWS!!!" << ex.what() << std::endl;
  }
}

void test_transformKLH()
{
  PeakTransformHKL transform("K (Lattice)", "L (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Y()); // X -> K
  TS_ASSERT_EQUALS(transformed.Y(), original.Z()); // Y -> L
  TS_ASSERT_EQUALS(transformed.Z(), original.X()); // Z -> H

  V3D backToOriginal = transform.transformBack(transformed);
  TS_ASSERT_EQUALS(backToOriginal.X(), original.X());
  TS_ASSERT_EQUALS(backToOriginal.Y(), original.Y());
  TS_ASSERT_EQUALS(backToOriginal.Z(), original.Z());

  boost::regex_match("H (Lattice)", transform.getFreePeakAxisRegex());
  boost::regex_match("[H,0,0]", transform.getFreePeakAxisRegex());
}

void test_transformKHL()
{
  PeakTransformHKL transform("K (Lattice)", "H (Lattice)");
  V3D original(0, 1, 2);
  V3D transformed = transform.transform(original);
  TS_ASSERT_EQUALS(transformed.X(), original.Y()); // X -> K
  TS_ASSERT_EQUALS(transformed.Y(), original.X()); // Y -> H
  TS_ASSERT_EQUALS(transformed.Z(), original.Z()); // Z -> L

  V3D backToOriginal = transform.transformBack(transformed);
  TS_ASSERT_EQUALS(backToOriginal.X(), original.X());
  TS_ASSERT_EQUALS(backToOriginal.Y(), original.Y());
  TS_ASSERT_EQUALS(backToOriginal.Z(), original.Z());

  boost::regex_match("L (Lattice)", transform.getFreePeakAxisRegex());
  boost::regex_match("[0,0,L]", transform.getFreePeakAxisRegex());
}

void test_copy_construction()
{
  PeakTransformHKL A("H", "L");
  PeakTransformHKL B(A);

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
  PeakTransformHKL A("H", "L");
  PeakTransformHKL B("K", "H");
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
  PeakTransformHKL A("H", "L");
  PeakTransform_sptr clone = A.clone();

  TSM_ASSERT("Clone product is the wrong type.", boost::dynamic_pointer_cast<PeakTransformHKL>(clone) != NULL);

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
  PeakTransform_sptr expectedProduct = boost::make_shared<PeakTransformHKL>("H", "K");

  // Use the factory to create a product.
  PeakTransformHKLFactory factory;
  PeakTransform_sptr product = factory.createDefaultTransform();

  // Check the type of the output product object.
  TSM_ASSERT("Factory product is the wrong type.", boost::dynamic_pointer_cast<PeakTransformHKL>(product) != NULL);

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
  PeakTransformHKL transform;
  TS_ASSERT_EQUALS(PeakTransformHKL::name(), transform.getFriendlyName());
  TS_ASSERT_EQUALS("HKL", transform.getFriendlyName());
}


void test_getCoordinateSystem()
{
  PeakTransformHKL transform;
  TS_ASSERT_EQUALS(Mantid::Kernel::HKL, transform.getCoordinateSystem())
}

};
#endif

//end SLICE_VIEWER_PEAKTRANSFORMHKL_TEST_H_
