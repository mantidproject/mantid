#ifndef MANTID_NOTEBOOKBUILDERTEST_H_
#define MANTID_NOTEBOOKBUILDERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/NotebookBuilder.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class NotebookBuilderTest : public CxxTest::TestSuite {
  /// Use a fake algorithm object instead of a dependency on a real one.
  class SubAlgorithm : public Algorithm {
  public:
    SubAlgorithm() : Algorithm() {}
    virtual ~SubAlgorithm() {}
    const std::string name() const { return "SubAlgorithm"; }
    int version() const { return 1; }
    const std::string category() const { return "Cat;Leopard;Mink"; }
    const std::string summary() const { return "SubAlgorithm"; }
    const std::string workspaceMethodName() const { return "methodname"; }
    const std::string workspaceMethodOnTypes() const {
      return "MatrixWorkspace;ITableWorkspace";
    }
    const std::string workspaceMethodInputProperty() const {
      return "InputWorkspace";
    }

    void init() {
      declareProperty("PropertyA", "Hello");
      declareProperty("PropertyB", "World");
    }
    void exec() {
      // nothing to do!
    }
  };

  // basic algorithm. This acts as a child called for other
  // DataProcessorAlgorithms
  class BasicAlgorithm : public Algorithm {
  public:
    BasicAlgorithm() : Algorithm() {}
    virtual ~BasicAlgorithm() {}
    const std::string name() const { return "BasicAlgorithm"; }
    int version() const { return 1; }
    const std::string category() const { return "Cat;Leopard;Mink"; }
    const std::string summary() const { return "BasicAlgorithm"; }
    const std::string workspaceMethodName() const { return "methodname"; }
    const std::string workspaceMethodOnTypes() const {
      return "MatrixWorkspace;ITableWorkspace";
    }
    const std::string workspaceMethodInputProperty() const {
      return "InputWorkspace";
    }

    void init() {
      declareProperty("PropertyA", "Hello");
      declareProperty("PropertyB", "World");
      declareProperty("PropertyC", "", Direction::Output);
    }
    void exec() {
      // the history from this should never be stored
      auto alg = createChildAlgorithm("SubAlgorithm");
      alg->initialize();
      alg->setProperty("PropertyA", "I Don't exist!");
      alg->execute();
      setProperty("PropertyC", "I have been set!");
    }
  };

  // middle layer algorithm executed by a top level algorithm
  class NestedAlgorithm : public DataProcessorAlgorithm {
  public:
    NestedAlgorithm() : DataProcessorAlgorithm() {}
    virtual ~NestedAlgorithm() {}
    const std::string name() const { return "NestedAlgorithm"; }
    int version() const { return 1; }
    const std::string category() const { return "Cat;Leopard;Mink"; }
    const std::string summary() const { return "NestedAlgorithm"; }
    const std::string workspaceMethodName() const { return "methodname"; }
    const std::string workspaceMethodOnTypes() const {
      return "MatrixWorkspace;ITableWorkspace";
    }
    const std::string workspaceMethodInputProperty() const {
      return "InputWorkspace";
    }

    void init() {
      declareProperty("PropertyA", 13);
      declareProperty("PropertyB", 42);
    }

    void exec() {
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

  // top level algorithm which executes -> NestedAlgorithm which executes ->
  // BasicAlgorithm
  class TopLevelAlgorithm : public DataProcessorAlgorithm {
  public:
    TopLevelAlgorithm() : DataProcessorAlgorithm() {}
    virtual ~TopLevelAlgorithm() {}
    const std::string name() const { return "TopLevelAlgorithm"; }
    int version() const { return 1; }
    const std::string category() const { return "Cat;Leopard;Mink"; }
    const std::string summary() const { return "TopLevelAlgorithm"; }
    const std::string workspaceMethodName() const { return "methodname"; }
    const std::string workspaceMethodOnTypes() const {
      return "Workspace;MatrixWorkspace;ITableWorkspace";
    }
    const std::string workspaceMethodInputProperty() const {
      return "InputWorkspace";
    }

    void init() {
      declareProperty(new WorkspaceProperty<MatrixWorkspace>(
          "InputWorkspace", "", Direction::Input));
      declareProperty(new WorkspaceProperty<MatrixWorkspace>(
          "OutputWorkspace", "", Direction::Output));
    }
    void exec() {
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
  void setUp() {
    Mantid::API::AlgorithmFactory::Instance().subscribe<TopLevelAlgorithm>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<NestedAlgorithm>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<BasicAlgorithm>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<SubAlgorithm>();
  }

  void tearDown() {
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("TopLevelAlgorithm",
                                                          1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("NestedAlgorithm", 1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("BasicAlgorithm", 1);
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("SubAlgorithm", 1);
  }

  void test_Build_Simple() {
    std::string result = "               \"input\" : "
                         "\"TopLevelAlgorithm(InputWorkspace='test_input_"
                         "workspace', "
                         "OutputWorkspace='test_output_workspace')\",";
    boost::shared_ptr<WorkspaceTester> input(new WorkspaceTester());
    AnalysisDataService::Instance().addOrReplace("test_input_workspace", input);

    auto alg = AlgorithmFactory::Instance().create("TopLevelAlgorithm", 1);
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", input);
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "test_output_workspace");
    auto wsHist = ws->getHistory();

    NotebookBuilder builder(wsHist.createView());
    std::string notebookText =
        builder.build("Workspace Name", "Workspace Title", "Workspace Comment");

    std::vector<std::string> notebookLines;
    std::string line;
    std::istringstream buffer(notebookText);
    while (std::getline(buffer, line))
      notebookLines.push_back(line);

    // Compare line with expected result
    TS_ASSERT_EQUALS(notebookLines[64], result)

    AnalysisDataService::Instance().remove("test_output_workspace");
    AnalysisDataService::Instance().remove("test_input_workspace");
  }

  void test_Build_Unrolled() {
    std::string result_markdown =
        "               \"source\" : \"Child algorithms of TopLevelAlgorithm\"";
    std::string result_code =
        "               \"input\" : \"BasicAlgorithm(PropertyA='FirstOne')\",";

    boost::shared_ptr<WorkspaceTester> input(new WorkspaceTester());
    AnalysisDataService::Instance().addOrReplace("test_input_workspace", input);

    auto alg = AlgorithmFactory::Instance().create("TopLevelAlgorithm", 1);
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", input);
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "test_output_workspace");
    auto wsHist = ws->getHistory();
    auto view = wsHist.createView();

    view->unrollAll();
    NotebookBuilder builder(view);
    std::string notebookText =
        builder.build(ws->name(), ws->getTitle(), ws->getComment());

    std::vector<std::string> notebookLines;
    std::string line;
    std::istringstream buffer(notebookText);
    while (std::getline(buffer, line))
      notebookLines.push_back(line);

    TS_ASSERT_EQUALS(notebookLines[64], result_markdown)
    TS_ASSERT_EQUALS(notebookLines[100], result_code)

    AnalysisDataService::Instance().remove("test_output_workspace");
    AnalysisDataService::Instance().remove("test_input_workspace");
  }

  void test_Partially_Unrolled() {
    std::string result_markdown =
        "               \"source\" : \"Child algorithms of TopLevelAlgorithm\"";
    std::string result_code =
        "               \"input\" : \"BasicAlgorithm(PropertyA='FirstOne')\",";

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

    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "test_output_workspace");
    auto wsHist = ws->getHistory();
    auto view = wsHist.createView();

    view->unroll(0);
    view->unroll(1);
    view->unroll(5);

    NotebookBuilder builder(view);
    std::string notebookText =
        builder.build(ws->name(), ws->getTitle(), ws->getComment());

    std::vector<std::string> notebookLines;
    std::string line;
    std::istringstream buffer(notebookText);
    while (std::getline(buffer, line))
      notebookLines.push_back(line);

    TS_ASSERT_EQUALS(notebookLines[64], result_markdown)
    TS_ASSERT_EQUALS(notebookLines[74], result_code)

    AnalysisDataService::Instance().remove("test_output_workspace");
    AnalysisDataService::Instance().remove("test_input_workspace");
  }

  void test_Build_Simple_with_backslash() {
    // checks that property values with \ get prefixed with r, eg.
    // filename=r'c:\test\data.txt'
    std::string result = "               \"input\" : "
                         "\"TopLevelAlgorithm(InputWorkspace=r'test_inp\\\\ut_"
                         "workspace', "
                         "OutputWorkspace='test_output_workspace')\",";
    boost::shared_ptr<WorkspaceTester> input(new WorkspaceTester());
    AnalysisDataService::Instance().addOrReplace("test_inp\\ut_workspace",
                                                 input);

    auto alg = AlgorithmFactory::Instance().create("TopLevelAlgorithm", 1);
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", input);
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "test_output_workspace");
    auto wsHist = ws->getHistory();

    NotebookBuilder builder(wsHist.createView());
    std::string notebookText =
        builder.build(ws->name(), ws->getTitle(), ws->getComment());

    std::vector<std::string> notebookLines;
    std::string line;
    std::istringstream buffer(notebookText);
    while (std::getline(buffer, line))
      notebookLines.push_back(line);

    TS_ASSERT_EQUALS(notebookLines[64], result)

    AnalysisDataService::Instance().remove("test_output_workspace");
    AnalysisDataService::Instance().remove("test_inp\\ut_workspace");
  }
};

#endif // MANTID_NOTEBOOKBUILDERTEST_H_