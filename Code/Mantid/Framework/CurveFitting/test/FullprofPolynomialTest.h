#ifndef MANTID_CURVEFITTING_FULLPROFPOLYNOMIALTEST_H_
#define MANTID_CURVEFITTING_FULLPROFPOLYNOMIALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/FullprofPolynomial.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidDataObjects/Workspace2D.h"


using Mantid::CurveFitting::FullprofPolynomial;

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

class FullprofPolynomialTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FullprofPolynomialTest *createSuite() { return new FullprofPolynomialTest(); }
  static void destroySuite( FullprofPolynomialTest *suite ) { delete suite; }


  void testForCategories()
  {
    FullprofPolynomial forCat;
    const std::vector<std::string> categories = forCat.categories();
    TS_ASSERT( categories.size() == 1 );
    TS_ASSERT( categories[0] == "Background" );
  }

  /** Test function on a Fullprof polynomial function
    */
  void test_FPPolynomial()
  {
    // Create a workspace
    std::string wsName = "TOFPolybackgroundBackgroundTest";
    int histogramNumber = 1;
    int timechannels = 1000;
    DataObjects::Workspace2D_sptr ws2D =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            API::WorkspaceFactory::Instance().create("Workspace2D", histogramNumber, timechannels, timechannels));

    AnalysisDataService::Instance().add(wsName, ws2D);

    double tof0 = 8000.;
    double dtof = 5.;

    for (int i = 0; i < timechannels; i++)
    {
      ws2D->dataX(0)[i] = static_cast<double>(i) * dtof + tof0;
    }

    // Create a function
    IFunction_sptr tofbkgd = boost::dynamic_pointer_cast<IFunction>(boost::make_shared<FullprofPolynomial>());
    TS_ASSERT_THROWS_NOTHING(tofbkgd->setAttributeValue("n", 6));
    TS_ASSERT_THROWS_NOTHING(tofbkgd->setAttributeValue("Bkpos", 10000.));
    TS_ASSERT_THROWS_NOTHING(tofbkgd->setParameter("A0", 0.3));
    TS_ASSERT_THROWS_NOTHING(tofbkgd->setParameter("A1", 1.0));
    TS_ASSERT_THROWS_NOTHING(tofbkgd->setParameter("A2", -0.5));
    TS_ASSERT_THROWS_NOTHING(tofbkgd->setParameter("A3", 0.05));
    TS_ASSERT_THROWS_NOTHING(tofbkgd->setParameter("A4", -0.02));

    // Calculate function
    FunctionDomain1DVector domain(ws2D->readX(0));
    FunctionValues values(domain);
    tofbkgd->function(domain, values);

    // Test result
    TS_ASSERT_DELTA(values[400], 0.3, 1.0E-10); // Y[10000] = B0
    TS_ASSERT_DELTA(values[0], 0.079568, 1.0E-5);
    TS_ASSERT_DELTA(values[605], 0.39730, 1.0E-5);
    TS_ASSERT_DELTA(values[999], 0.55583, 1.0E-5);

    // Set the workspace
    for (size_t i = 0; i < ws2D->readY(0).size(); ++i)
    {
      ws2D->dataY(0)[i] = values[i];
      ws2D->dataE(0)[i] = sqrt(fabs(values[i]));
    }

    // Make function a little bit off
    tofbkgd->setParameter("A0", 0.5);
    tofbkgd->setParameter("A3", 0.0);

    // Set up fit
    CurveFitting::Fit fitalg;
    TS_ASSERT_THROWS_NOTHING(fitalg.initialize());
    TS_ASSERT( fitalg.isInitialized() );

    fitalg.setProperty("Function", tofbkgd);
    fitalg.setPropertyValue("InputWorkspace", wsName);
    fitalg.setPropertyValue("WorkspaceIndex", "0");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(TS_ASSERT( fitalg.execute() ));
    TS_ASSERT( fitalg.isExecuted() );

    // test the output from fit is what you expect
    double chi2 = fitalg.getProperty("OutputChi2overDoF");

    TS_ASSERT_DELTA( chi2, 0.0, 0.1);
    TS_ASSERT_DELTA( tofbkgd->getParameter("A0"), 0.3, 0.01);
    TS_ASSERT_DELTA( tofbkgd->getParameter("A1"), 1.0, 0.0003);
    TS_ASSERT_DELTA( tofbkgd->getParameter("A3"), 0.05, 0.01);

    // Clean
    AnalysisDataService::Instance().remove(wsName);

    return;
  }


};


#endif /* MANTID_CURVEFITTING_FULLPROFPOLYNOMIALTEST_H_ */
