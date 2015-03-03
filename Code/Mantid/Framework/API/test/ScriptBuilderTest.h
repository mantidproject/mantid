#ifndef HISTORYVIEWTEST_H_
#define HISTORYVIEWTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/ScriptBuilder.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;


class ScriptBuilderTest : public CxxTest::TestSuite
{
  /// Use a fake algorithm object instead of a dependency on a real one.
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
      declareProperty("PropertyC", "", Direction::Output);
    }
    void exec()
    {
      // the history from this should never be stored
      auto alg = createChildAlgorithm("SubAlgorithm");
      alg->initialize();
      alg->setProperty("PropertyA", "I Don't exist!");
      alg->execute();
      setProperty("PropertyC", "I have been set!");
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
      alg->setProperty("PropertyA", "FirstOne");
      alg->execute();

      alg = createChildAlgorithm("BasicAlgorithm");
      alg->initialize();
      alg->setProperty("PropertyA", "SecondOne");
      alg->execute();
    }
  };

  //top level algorithm which executes -> NestedAlgorithm which executes -> BasicAlgorithm
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
    }
    void exec()
    {
      auto alg = createChildAlgorithm("NestedAlgorithm");
      alg->initialize();
      alg->execute();

      alg = createChildAlgorithm("NestedAlgorithm");
      alg->initialize();
      alg->execute();

      boost::shared_ptr<MatrixWorkspace> output(new WorkspaceTester());
      setProperty("OutputWorkspace", output);
    }
  };

private:

public:

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

  void test_Build_Simple()
  {
    std::string result[] = {
      "TopLevelAlgorithm(InputWorkspace='test_input_workspace', OutputWorkspace='test_output_workspace')",
      ""
    };
    boost::shared_ptr<WorkspaceTester> input(new WorkspaceTester());
    AnalysisDataService::Instance().addOrReplace("test_input_workspace", input);

    auto alg = AlgorithmFactory::Instance().create("TopLevelAlgorithm", 1);
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", input);
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_output_workspace");
    auto wsHist = ws->getHistory();

    ScriptBuilder builder(wsHist.createView());
    std::string scriptText = builder.build();

    std::vector<std::string> scriptLines;
    boost::split(scriptLines, scriptText, boost::is_any_of("\n"));

    int i=0;
    for (auto it = scriptLines.begin(); it != scriptLines.end(); ++it, ++i)
    {
      TS_ASSERT_EQUALS(*it, result[i])
    }

    AnalysisDataService::Instance().remove("test_output_workspace");
    AnalysisDataService::Instance().remove("test_input_workspace");
  }

  void test_Build_Unrolled()
  {
    std::string result[] = {
      "",
      "# Child algorithms of TopLevelAlgorithm",
      "",
      "## Child algorithms of NestedAlgorithm",
      "BasicAlgorithm(PropertyA='FirstOne')",
      "BasicAlgorithm(PropertyA='SecondOne')",
      "## End of child algorithms of NestedAlgorithm",
      "",
      "## Child algorithms of NestedAlgorithm",
      "BasicAlgorithm(PropertyA='FirstOne')",
      "BasicAlgorithm(PropertyA='SecondOne')",
      "## End of child algorithms of NestedAlgorithm",
      "",
      "# End of child algorithms of TopLevelAlgorithm",
      "",
      "",
    };

    boost::shared_ptr<WorkspaceTester> input(new WorkspaceTester());
    AnalysisDataService::Instance().addOrReplace("test_input_workspace", input);

    auto alg = AlgorithmFactory::Instance().create("TopLevelAlgorithm", 1);
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", input);
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_output_workspace");
    auto wsHist = ws->getHistory();
    auto view = wsHist.createView();

    view->unrollAll();
    ScriptBuilder builder(view);
    std::string scriptText = builder.build();

    std::vector<std::string> scriptLines;
    boost::split(scriptLines, scriptText, boost::is_any_of("\n"));

    int i=0;
    for (auto it = scriptLines.begin(); it != scriptLines.end(); ++it, ++i)
    {
      TS_ASSERT_EQUALS(*it, result[i])
    }

    AnalysisDataService::Instance().remove("test_output_workspace");
    AnalysisDataService::Instance().remove("test_input_workspace");
  }

  void test_Partially_Unrolled()
  {
    std::string result[] = {
      "",
      "# Child algorithms of TopLevelAlgorithm",
      "",
      "## Child algorithms of NestedAlgorithm",
      "BasicAlgorithm(PropertyA='FirstOne')",
      "BasicAlgorithm(PropertyA='SecondOne')",
      "## End of child algorithms of NestedAlgorithm",
      "",
      "NestedAlgorithm()",
      "# End of child algorithms of TopLevelAlgorithm",
      "",
      "# Child algorithms of TopLevelAlgorithm",
      "NestedAlgorithm()",
      "NestedAlgorithm()",
      "# End of child algorithms of TopLevelAlgorithm",
      "",
      "",
    };

    boost::shared_ptr<WorkspaceTester> input(new WorkspaceTester());
    AnalysisDataService::Instance().addOrReplace("test_input_workspace", input);

    auto alg = AlgorithmFactory::Instance().create("TopLevelAlgorithm", 1);
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", input);
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", "test_output_workspace");
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_output_workspace");
    auto wsHist = ws->getHistory();
    auto view = wsHist.createView();

    view->unroll(0);
    view->unroll(1);
    view->unroll(5);

    ScriptBuilder builder(view);
    std::string scriptText = builder.build();

    std::vector<std::string> scriptLines;
    boost::split(scriptLines, scriptText, boost::is_any_of("\n"));

    int i=0;
    for (auto it = scriptLines.begin(); it != scriptLines.end(); ++it, ++i)
    {
      TS_ASSERT_EQUALS(*it, result[i])
    }

    AnalysisDataService::Instance().remove("test_output_workspace");
    AnalysisDataService::Instance().remove("test_input_workspace");
  }


  void test_Build_Simple_with_backslash()
  {
    //checks that property values with \ get prefixed with r, eg. filename=r'c:\test\data.txt'
    std::string result[] = {
      "TopLevelAlgorithm(InputWorkspace=r'test_inp\\ut_workspace', OutputWorkspace='test_output_workspace')",
      ""
    };
    boost::shared_ptr<WorkspaceTester> input(new WorkspaceTester());
    AnalysisDataService::Instance().addOrReplace("test_inp\\ut_workspace", input);

     auto alg = AlgorithmFactory::Instance().create("TopLevelAlgorithm", 1);
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", input);
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("test_output_workspace");
    auto wsHist = ws->getHistory();

    ScriptBuilder builder(wsHist.createView());
    std::string scriptText = builder.build();

    std::vector<std::string> scriptLines;
    boost::split(scriptLines, scriptText, boost::is_any_of("\n"));

    int i=0;
    for (auto it = scriptLines.begin(); it != scriptLines.end(); ++it, ++i)
    {
      TS_ASSERT_EQUALS(*it, result[i])
    }

    AnalysisDataService::Instance().remove("test_output_workspace");
    AnalysisDataService::Instance().remove("test_inp\\ut_workspace");
  }

};

#endif
