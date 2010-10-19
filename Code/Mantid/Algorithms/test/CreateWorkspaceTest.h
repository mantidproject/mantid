#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CreateWorkspace.h"

#include "MantidAPI/MatrixWorkspace.h"

class CreateWorkspaceTest : public CxxTest::TestSuite
{
public:
  void testMeta()
  {
    Mantid::Algorithms::CreateWorkspace alg;
    TS_ASSERT_EQUALS(alg.name(), "CreateWorkspace");
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void testCreate()
  {
    Mantid::Algorithms::CreateWorkspace alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    int nSpec = 1;

    std::vector<double> dataEYX;
    
    for ( int i = 0; i < 3; i++ )
    {
      dataEYX.push_back(i*1.234);
    }

    alg.setProperty<int>("NSpec", nSpec);
    alg.setProperty<std::vector<double> >("DataX", dataEYX);
    alg.setProperty<std::vector<double> >("DataY", dataEYX);
    alg.setProperty<std::vector<double> >("DataE", dataEYX);
    alg.setPropertyValue("UnitX", "Wavelength");
    alg.setPropertyValue("OutputWorkspace", "createWorkspace");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    Mantid::API::MatrixWorkspace_const_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("createWorkspace"));

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), nSpec);

    TS_ASSERT_EQUALS(ws->dataX(0)[0], 0);
    TS_ASSERT_EQUALS(ws->dataX(0)[1], 1.234);
    TS_ASSERT_EQUALS(ws->dataX(0)[2], 2.468);
    TS_ASSERT_EQUALS(ws->dataY(0)[0], 0);
    TS_ASSERT_EQUALS(ws->dataY(0)[1], 1.234);
    TS_ASSERT_EQUALS(ws->dataY(0)[2], 2.468);
    TS_ASSERT_EQUALS(ws->dataE(0)[0], 0);
    TS_ASSERT_EQUALS(ws->dataE(0)[1], 1.234);
    TS_ASSERT_EQUALS(ws->dataE(0)[2], 2.468);
    TS_ASSERT_EQUALS(ws->getAxis(0)->unit()->caption(), "Wavelength");
  }
};