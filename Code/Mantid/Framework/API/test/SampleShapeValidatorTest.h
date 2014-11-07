#ifndef MANTID_API_SAMPLESHAPEVALIDATORTEST_H_
#define MANTID_API_SAMPLESHAPEVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SampleShapeValidator.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/FakeObjects.h"

#include "boost/make_shared.hpp"

using Mantid::API::SampleShapeValidator;

class SampleShapeValidatorTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SampleShapeValidatorTest *createSuite() { return new SampleShapeValidatorTest(); }
  static void destroySuite( SampleShapeValidatorTest *suite ) { delete suite; }

  void test_validator_passes_for_workspace_with_defined_sample_shape()
  {
    auto fakeWS = boost::make_shared<WorkspaceTester>();
    // Add a sample shape
    auto sphere = ComponentCreationHelper::createSphere(1.0, V3D(), "sphere");
    fakeWS->mutableSample().setShape(*sphere);

    auto sampleValidator = boost::make_shared<SampleShapeValidator>();
    TS_ASSERT_EQUALS( sampleValidator->isValid(fakeWS), "" );
  }

  void test_validator_throws_error_for_workspace_without_shape()
  {
    auto fakeWS = boost::make_shared<WorkspaceTester>();

    auto sampleValidator = boost::make_shared<SampleShapeValidator>();
    TS_ASSERT_EQUALS( sampleValidator->isValid(fakeWS),
                      "Invalid or no shape defined for sample" );
  }


};


#endif /* MANTID_API_SAMPLESHAPEVALIDATORTEST_H_ */
