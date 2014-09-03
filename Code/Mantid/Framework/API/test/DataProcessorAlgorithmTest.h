#ifndef MANTID_API_DATAPROCESSORALGORITHMTEST_H_
#define MANTID_API_DATAPROCESSORALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::API;


class DataProcessorAlgorithmTest : public CxxTest::TestSuite
{

  //top level algorithm which executes -> NestedAlgorithm which executes -> BasicAlgorithm
  class SubAlgorithm : public Algorithm
  {
  public:
    SubAlgorithm() : Algorithm() {}
    virtual ~SubAlgorithm() {}
    const std::string name() const { return "SubAlgorithm";}
    int version() const  { return 1;}
    const std::string category() const { return "Cat;Leopard;Mink";}
    const std::string summary() const { return "SubAlgorithm"; }
    const std::string workspaceMethodName() const { return "methodname"; }
    const std::string workspaceMethodOnTypes() const { return "MatrixWorkspace;ITableWorkspace"; }
    const std::string workspaceMethodInputProperty() const { return "InputWorkspace"; }
    
    void init()
    {
      declareProperty("PropertyA", "Hello");
      declareProperty("PropertyB", "World");
    }
    void exec()
    {
      //nothing to do!
    }
  };

  // basic algorithm. This acts as a child called for other DataProcessorAlgorithms
  class BasicAlgorithm : public Algorithm
  {
  public:
    BasicAlgorithm() : Algorithm() {}
    virtual ~BasicAlgorithm() {}
    const std::string name() const { return "BasicAlgorithm";}
    int version() const  { return 1;}
    const std::string category() const { return "Cat;Leopard;Mink";}
    const std::string summary() const { return "BasicAlgorithm"; }
    const std::string workspaceMethodName() const { return "methodname"; }
    const std::string workspaceMethodOnTypes() const { return "MatrixWorkspace;ITableWorkspace"; }
    const std::string workspaceMethodInputProperty() const { return "InputWorkspace"; }
    
    void init()
    {
      declareProperty("PropertyA", "Hello");
      declareProperty("PropertyB", "World");
    }
    void exec()
    {
      // the history from this should never be stored
      auto alg = createChildAlgorithm("SubAlgorithm");
      alg->initialize();
      alg->setProperty("PropertyA", "I Don't exist!");
      alg->execute();
    }
  };

  //middle layer algorithm executed by a top level algorithm
  class NestedAlgorithm : public DataProcessorAlgorithm
  {
  public:
    NestedAlgorithm() : DataProcessorAlgorithm() {}
    virtual ~NestedAlgorithm() {}
    const std::string name() const { return "NestedAlgorithm";}
    int version() const  { return 1;}
    const std::string category() const { return "Cat;Leopard;Mink";}
    const std::string summary() const { return "NestedAlgorithm"; }
    const std::string workspaceMethodName() const { return "methodname"; }
    const std::string workspaceMethodOnTypes() const { return "MatrixWorkspace;ITableWorkspace"; }
    const std::string workspaceMethodInputProperty() const { return "InputWorkspace"; }
    
    void init()
    {
      declareProperty("PropertyA", 13);
      declareProperty("PropertyB", 42);
    }

    void exec()
    {
      auto alg = createChildAlgorithm("BasicAlgorithm");
      alg->initialize();
      alg->setProperty("PropertyA", "Same!");
      alg->execute();
    }
  };

  class TopLevelAlgorithm : public DataProcessorAlgorithm
  {
  public:
    TopLevelAlgorithm() : DataProcessorAlgorithm() {}
    virtual ~TopLevelAlgorithm() {}
    const std::string name() const { return "TopLevelAlgorithm";}
    int version() const  { return 1;}
    const std::string category() const { return "Cat;Leopard;Mink";}
    const std::string summary() const { return "TopLevelAlgorithm"; }
    const std::string workspaceMethodName() const { return "methodname"; }
    const std::string workspaceMethodOnTypes() const { return "Workspace;MatrixWorkspace;ITableWorkspace"; }
    const std::string workspaceMethodInputProperty() const { return "InputWorkspace"; }
    
    void init()
    {
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input));
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","", Direction::Output));
      declareProperty("RecordHistory", true, Direction::Input);
    }
    void exec()
    {
      const bool recordHistory = static_cast<bool>(getProperty("RecordHistory"));
      auto alg = createChildAlgorithm("NestedAlgorithm");
      alg->enableHistoryRecordingForChild(recordHistory);
      alg->initialize();
      alg->execute();

      boost::shared_ptr<MatrixWorkspace> output(new WorkspaceTester());
      setProperty("OutputWorkspace", output);
    }
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorAlgorithmTest *createSuite() { return new DataProcessorAlgorithmTest(); }
  static void destroySuite( DataProcessorAlgorithmTest *suite ) { delete suite; }

  void setUp()
  {
    Mantid::API::AlgorithmFactory::Instance().subscribe<TopLevelAlgorithm>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<NestedAlgorithm>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<BasicAlgorithm>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<SubAlgorithm>();
  }

  void tearDown()
  {
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("TopLevelAlgorithm",1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("NestedAlgorithm",1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("BasicAlgorithm",1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("SubAlgorithm",1);
  }

  void test_Nested_History()
  {
    boost::shared_ptr<WorkspaceTester> input(new WorkspaceTester());
    AnalysisDataService::Instance().addOrReplace("test_input_workspace", input);

    TopLevelAlgorithm alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", input);
    alg.setPropertyValue("OutputWorkspace", "test_output_workspace");

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );
   
    // check workspace history
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_output_workspace");
    auto wsHist = ws->getHistory();
    TS_ASSERT_EQUALS( wsHist.size(), 1);
    
    // check top level algorithm history
    auto algHist = wsHist.getAlgorithmHistory(0);
    
    TS_ASSERT_EQUALS(algHist->name(), "TopLevelAlgorithm");
    TS_ASSERT_EQUALS(algHist->childHistorySize(), 1);
    
    // check nested algorithm history
    auto childHist = algHist->getChildAlgorithmHistory(0);

    TS_ASSERT_EQUALS(childHist->name(), "NestedAlgorithm");
    TS_ASSERT_EQUALS(childHist->childHistorySize(), 1);

    // check basic algorithm history
    childHist = childHist->getChildAlgorithmHistory(0);
    TS_ASSERT_EQUALS(childHist->name(), "BasicAlgorithm");
    
    //even though BasicAlgorithm calls another algorithm, 
    //it should not store the history.
    TS_ASSERT_EQUALS(childHist->childHistorySize(), 0);

    AnalysisDataService::Instance().remove("test_output_workspace");
    AnalysisDataService::Instance().remove("test_input_workspace");
  }

  void test_Dont_Record_Nested_History()
  {
    boost::shared_ptr<WorkspaceTester> input(new WorkspaceTester());
    AnalysisDataService::Instance().addOrReplace("test_input_workspace", input);

    TopLevelAlgorithm alg;
    alg.initialize();
    alg.setRethrows(true);
    alg.setProperty("InputWorkspace", input);
    alg.setProperty("RecordHistory", false);
    alg.setPropertyValue("OutputWorkspace", "test_output_workspace");

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );
   
    // check workspace history
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_output_workspace");
    auto wsHist = ws->getHistory();
    TS_ASSERT_EQUALS( wsHist.size(), 1 );

    auto algHist = wsHist.getAlgorithmHistory(0);
    TS_ASSERT_EQUALS( algHist->name(), "TopLevelAlgorithm");    
    //algorithm should have no child histories.
    TS_ASSERT_EQUALS( algHist->childHistorySize(), 0 );

    AnalysisDataService::Instance().remove("test_output_workspace");
    AnalysisDataService::Instance().remove("test_input_workspace");
  }

};


#endif /* MANTID_API_DATAPROCESSORALGORITHMTEST_H_ */
