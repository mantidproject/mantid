#ifndef RESOLUTIONCONVOLVEDCROSSSECTIONTEST_H_
#define RESOLUTIONCONVOLVEDCROSSSECTIONTEST_H_

#include "MantidAPI/FunctionDomainMD.h"
#include "MantidAPI/FunctionValues.h"

#include "MantidMDAlgorithms/Quantification/ForegroundModel.h"
#include "MantidMDAlgorithms/Quantification/ForegroundModelFactory.h"
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolution.h"
#include "MantidMDAlgorithms/Quantification/MDResolutionConvolutionFactory.h"
#include "MantidMDAlgorithms/Quantification/ResolutionConvolvedCrossSection.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MDFittingTestHelpers.h"

#include <cxxtest/TestSuite.h>

#include <gmock/gmock.h>

class ResolutionConvolvedCrossSectionTest : public CxxTest::TestSuite
{

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ResolutionConvolvedCrossSectionTest *createSuite() { return new ResolutionConvolvedCrossSectionTest(); }
  static void destroySuite( ResolutionConvolvedCrossSectionTest *suite ) { delete suite; }

  ResolutionConvolvedCrossSectionTest()
  {
    using namespace Mantid::MDAlgorithms;
    ForegroundModelFactory::Instance().subscribe<FakeForegroundModel>("FakeForegroundModel");
    MDResolutionConvolutionFactory::Instance().subscribe<FakeMDResolutionConvolution>("FakeConvolution");
  }

  ~ResolutionConvolvedCrossSectionTest()
  {
    using namespace Mantid::MDAlgorithms;
    ForegroundModelFactory::Instance().unsubscribe("FakeForegroundModel");
    MDResolutionConvolutionFactory::Instance().unsubscribe("FakeConvolution");
  }

  void test_functionMD_Does_Not_Throw_With_Foreground_And_ResolutionModel_Attrs_Set()
  {
    using namespace Mantid::MDAlgorithms;
    using namespace Mantid::API;
    Mantid::API::IMDWorkspace_sptr testWS = createTestMDWorkspace();
    Mantid::API::IMDIterator *box = testWS->createIterator();
    FunctionDomainMD mdDomain(testWS,0,box->getDataSize());
    FunctionValues output;

    IFunction * crossSecResolution = createInitializedTestConvolution();
    crossSecResolution->setWorkspace(testWS);
    // TODO: Needs a better input workspace
    //TS_ASSERT_THROWS_NOTHING(crossSecResolution->function(mdDomain, output));
    delete box;
    delete crossSecResolution;
  }

  void test_Function_Acquires_ForegroundModelParameters_When_ResolutionModel_Is_Set()
  {
    using namespace Mantid::MDAlgorithms;
    using namespace Mantid::API;
    ResolutionConvolvedCrossSection *crossSection = createInitializedTestConvolution();
    FakeForegroundModel fgModel;
    fgModel.initialize();

    TS_ASSERT(fgModel.nParams() > 0);
    TS_ASSERT_EQUALS(crossSection->nParams(), fgModel.nParams());
    //Check values
    TS_ASSERT_EQUALS(crossSection->getParameter("FgA0"), fgModel.start1);
    TS_ASSERT_EQUALS(crossSection->getParameter("FgA1"), fgModel.start2);

    delete crossSection;
  }

  void test_Function_Acquires_Attributes_From_ResolutionType_And_ForegroundModel_When_Set()
  {
    using namespace Mantid::MDAlgorithms;
    using namespace Mantid::API;
    ResolutionConvolvedCrossSection crossSection;
    crossSection.initialize();
    crossSection.setAttributeValue("ForegroundModel", "FakeForegroundModel");

    const size_t startingNAttrs = crossSection.nAttributes();
    crossSection.setAttributeValue("ResolutionFunction", "FakeConvolution");

    TS_ASSERT_EQUALS(crossSection.nAttributes(), startingNAttrs + 5);
  }

  void test_ResolutionConvolution_Attributes_Are_Passed_On_Correctly()
  {
    // How this works -> The fake convolution's signal member is
    // set up to throw an exception if the attribute still has
    // its initial value. This should indicate that the setAttribute
    // call on the ResolutionConvolvedCrossSection object has
    // not passed it on.

    using namespace Mantid::MDAlgorithms;
    using namespace Mantid::API;

    IFunction * crossSection = createInitializedTestConvolution();

    Mantid::API::IMDWorkspace_sptr testWS = createTestMDWorkspace();
    auto mdDomain = boost::shared_ptr<FunctionDomainMD>(new FunctionDomainMD(testWS));
    FunctionValues output(*mdDomain);
    crossSection->setWorkspace(testWS);
    crossSection->setAttributeValue("ConvAtt0", 100.3);

    // Fake function throws if attribute value has not changed
    TS_ASSERT_THROWS_NOTHING(crossSection->function(*mdDomain, output));

    delete crossSection;
  }

private:
  /// Create a test resolution function, putting the results the the provided pointers
  Mantid::MDAlgorithms::ResolutionConvolvedCrossSection *
  createInitializedTestConvolution()
  {
    using namespace Mantid::MDAlgorithms;

    ResolutionConvolvedCrossSection *xSec = new ResolutionConvolvedCrossSection;
    xSec->initialize();
    xSec->setAttributeValue("ForegroundModel", "FakeForegroundModel");
    xSec->setAttributeValue("ResolutionFunction", "FakeConvolution");
    return xSec;
  }

  /**
   * Creates a workspace with 4 dims, 3 boxes and 1 event per box = 81 events
    * @return A pointer to the object
   */
  Mantid::API::IMDWorkspace_sptr createTestMDWorkspace()
  {
    using namespace Mantid::DataObjects;

    // 4 dims, 3 boxes and 1 event per box = 81 events
    boost::shared_ptr<MDEventWorkspace<MDEvent<4>,4> > testWS =
            MDEventsTestHelper::makeMDEWFull<4>(3,0.0,3.,1);

    testWS->addExperimentInfo(boost::make_shared<Mantid::API::ExperimentInfo>());
    return testWS;
  }
};

#endif /* RESOLUTIONCONVOLVEDCROSSSECTIONTEST */
