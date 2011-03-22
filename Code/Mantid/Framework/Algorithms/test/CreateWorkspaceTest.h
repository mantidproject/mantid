#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CreateWorkspace.h"

#include "MantidAPI/MatrixWorkspace.h"

#include "MantidAPI/TextAxis.h"

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

    std::vector<double> dataEYX;
    
    for ( int i = 0; i < 3; i++ )
    {
      dataEYX.push_back(i*1.234);
    }

    std::vector<std::string> qvals;
    qvals.push_back("9.876");

    TS_ASSERT_THROWS_NOTHING(alg.setProperty<int>("NSpec", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty<std::vector<double> >("DataX", dataEYX));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty<std::vector<double> >("DataY", dataEYX));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty<std::vector<double> >("DataE", dataEYX));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("UnitX", "Wavelength"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("VerticalAxisUnit", "MomentumTransfer"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty<std::vector<std::string> >("VerticalAxisValues", qvals));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "test_CreateWorkspace"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(alg.isExecuted());
    Mantid::API::MatrixWorkspace_const_sptr ws;

    TS_ASSERT_THROWS_NOTHING(ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("test_CreateWorkspace")));

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);

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

    TS_ASSERT_EQUALS(ws->getAxis(1)->unit()->unitID(), "MomentumTransfer");
    TS_ASSERT_EQUALS(ws->getAxis(1)->unit()->caption(), "q");
    
    double axisVal;
    
    TS_ASSERT_THROWS_NOTHING(axisVal = boost::lexical_cast<double>(ws->getAxis(1)->label(0)));

    TS_ASSERT_DELTA(axisVal, 9.876, 0.001);

    // Remove the created workspace
    TS_ASSERT_THROWS_NOTHING(Mantid::API::AnalysisDataService::Instance().remove("test_CreateWorkspace"));
  }

  void testCreateTextAxis()
  {
    Mantid::Algorithms::CreateWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "test_CreateWorkspace");
    alg.setPropertyValue("UnitX", "Wavelength");
    alg.setPropertyValue("VerticalAxisUnit", "Text");

    std::vector<std::string> textAxis;
    textAxis.push_back("I've Got");
    textAxis.push_back("A Lovely");
    textAxis.push_back("Bunch Of");
    textAxis.push_back("Coconuts");

    alg.setProperty<std::vector<std::string> >("VerticalAxisValues", textAxis);
    alg.setProperty<int>("NSpec", 4);

    std::vector<double> values;
    values.push_back(1.0);
    values.push_back(2.0);
    values.push_back(3.0);
    values.push_back(4.0);

    alg.setProperty<std::vector<double> >("DataX", values);
    alg.setProperty<std::vector<double> >("DataY", values);
    alg.setProperty<std::vector<double> >("DataE", values);

    alg.execute();

    TS_ASSERT( alg.isExecuted() );

    // Get hold of the output workspace
    Mantid::API::MatrixWorkspace_sptr workspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("test_CreateWorkspace"));

    TS_ASSERT( ! workspace->isHistogramData() );

    TS_ASSERT_EQUALS( workspace->getNumberHistograms(), 4 );

    Mantid::API::TextAxis* axis = dynamic_cast<Mantid::API::TextAxis*>(workspace->getAxis(1));

    TS_ASSERT_EQUALS( axis->label(0), "I've Got" );
    TS_ASSERT_EQUALS( axis->label(1), "A Lovely" );
    TS_ASSERT_EQUALS( axis->label(2), "Bunch Of" );
    TS_ASSERT_EQUALS( axis->label(3), "Coconuts" );

    // Remove workspace
    Mantid::API::AnalysisDataService::Instance().remove("test_CreateWorkspace");
  }
};