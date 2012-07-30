#ifndef MDRESOLUTIONCONVOLUTIONFACTORYTEST_H_
#define MDRESOLUTIONCONVOLUTIONFACTORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolutionFactory.h"
#include "MDFittingTestHelpers.h"

using Mantid::MDAlgorithms::MDResolutionConvolutionFactory;

class MDResolutionConvolutionFactoryTest : public CxxTest::TestSuite
{
  public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDResolutionConvolutionFactoryTest *createSuite() { return new MDResolutionConvolutionFactoryTest(); }
  static void destroySuite( MDResolutionConvolutionFactoryTest *suite ) { delete suite; }

  void test_Factory_Throws_When_Given_Invalid_Name()
  {
    using namespace Mantid::Kernel;
    FakeMDFunction fakeFunction;
    TS_ASSERT_THROWS(
        MDResolutionConvolutionFactory::Instance().createConvolution("__NOT_VALID", "FakeForeground", fakeFunction), Exception::NotFoundError);
  }

  void xtest_Factory_Creates_New_Convolution_Object_When_Name_And_ForegroundModel_Are_Valid()
  {
    using Mantid::MDAlgorithms::MDResolutionConvolution;
    FakeMDFunction fakeFunction;
    registerFakeTypes();

    MDResolutionConvolution *convolution(NULL);
    TS_ASSERT_THROWS_NOTHING(convolution = MDResolutionConvolutionFactory::Instance().createConvolution("FakeConvolution", "FakeForegroundModel", fakeFunction));
    TS_ASSERT(convolution != NULL);
    if(convolution)
    {
      TS_ASSERT_EQUALS(convolution->nAttributes(), 2);
      TS_ASSERT_EQUALS(convolution->getAttributeNames()[0], "ConvAtt0");
      TS_ASSERT_EQUALS(convolution->getAttributeNames()[1], "ConvAtt1");
    }


    deregisterFakeTypes();
  }

  private:
    void registerFakeTypes()
    {
      using Mantid::MDAlgorithms::ForegroundModelFactory;
      ForegroundModelFactory::Instance().subscribe<FakeForegroundModel>("FakeForegroundModel");
      MDResolutionConvolutionFactory::Instance().subscribe<FakeMDResolutionConvolution>("FakeConvolution");
    }

    void deregisterFakeTypes()
    {
      using Mantid::MDAlgorithms::ForegroundModelFactory;
      ForegroundModelFactory::Instance().unsubscribe("FakeForegroundModel");
      MDResolutionConvolutionFactory::Instance().unsubscribe("FakeConvolution");

    }

};

#endif // MDRESOLUTIONCONVOLUTIONFACTORYTEST_H_
