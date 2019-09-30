// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_NOTEBOOKBUILDERTEST_H_
#define MANTID_NOTEBOOKBUILDERTEST_H_

#include <algorithm>
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
    ~SubAlgorithm() override {}
    const std::string name() const override { return "SubAlgorithm"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Cat;Leopard;Mink"; }
    const std::string summary() const override { return "SubAlgorithm"; }
    const std::string workspaceMethodName() const override {
      return "methodname";
    }
    const std::string workspaceMethodOnTypes() const override {
      return "MatrixWorkspace;ITableWorkspace";
    }
    const std::string workspaceMethodInputProperty() const override {
      return "InputWorkspace";
    }

    void init() override {
      declareProperty("PropertyA", "Hello");
      declareProperty("PropertyB", "World");
    }
    void exec() override {
      // nothing to do!
    }
  };

  // basic algorithm. This acts as a child called for other
  // DataProcessorAlgorithms
  class BasicAlgorithm : public Algorithm {
  public:
    BasicAlgorithm() : Algorithm() {}
    ~BasicAlgorithm() override {}
    const std::string name() const override { return "BasicAlgorithm"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Cat;Leopard;Mink"; }
    const std::string summary() const override { return "BasicAlgorithm"; }
    const std::string workspaceMethodName() const override {
      return "methodname";
    }
    const std::string workspaceMethodOnTypes() const override {
      return "MatrixWorkspace;ITableWorkspace";
    }
    const std::string workspaceMethodInputProperty() const override {
      return "InputWorkspace";
    }

    void init() override {
      declareProperty("PropertyA", "Hello");
      declareProperty("PropertyB", "World");
      declareProperty("PropertyC", "", Direction::Output);
    }
    void exec() override {
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
    ~NestedAlgorithm() override {}
    const std::string name() const override { return "NestedAlgorithm"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Cat;Leopard;Mink"; }
    const std::string summary() const override { return "NestedAlgorithm"; }
    const std::string workspaceMethodName() const override {
      return "methodname";
    }
    const std::string workspaceMethodOnTypes() const override {
      return "MatrixWorkspace;ITableWorkspace";
    }
    const std::string workspaceMethodInputProperty() const override {
      return "InputWorkspace";
    }

    void init() override {
      declareProperty("PropertyA", 13);
      declareProperty("PropertyB", 42);
    }

    void exec() override {
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
    ~TopLevelAlgorithm() override {}
    const std::string name() const override { return "TopLevelAlgorithm"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Cat;Leopard;Mink"; }
    const std::string summary() const override { return "TopLevelAlgorithm"; }
    const std::string workspaceMethodName() const override {
      return "methodname";
    }
    const std::string workspaceMethodOnTypes() const override {
      return "Workspace;MatrixWorkspace;ITableWorkspace";
    }
    const std::string workspaceMethodInputProperty() const override {
      return "InputWorkspace";
    }

    void init() override {
      declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace", "", Direction::Input));
      declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "OutputWorkspace", "", Direction::Output));
    }
    void exec() override {
      auto alg = createChildAlgorithm("NestedAlgorithm");
      alg->initialize();
      alg->execute();

      alg = createChildAlgorithm("NestedAlgorithm");
      alg->initialize();
      alg->execute();

      boost::shared_ptr<MatrixWorkspace> output =
          boost::make_shared<WorkspaceTester>();
      setProperty("OutputWorkspace", output);
    }
  };

private:
public:
  void setUp() override {
    Mantid::API::AlgorithmFactory::Instance().subscribe<TopLevelAlgorithm>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<NestedAlgorithm>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<BasicAlgorithm>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<SubAlgorithm>();
  }

  void tearDown() override {
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
    boost::shared_ptr<WorkspaceTester> input =
        boost::make_shared<WorkspaceTester>();
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

    // Check that the expected line does appear in the output
    TS_ASSERT(std::find(notebookLines.cbegin(), notebookLines.cend(), result) !=
              notebookLines.cend())

    AnalysisDataService::Instance().remove("test_output_workspace");
    AnalysisDataService::Instance().remove("test_input_workspace");
  }

  void test_Build_Unrolled() {
    std::string result_markdown =
        R"(               "source" : "Child algorithms of TopLevelAlgorithm")";
    std::string result_code =
        "               \"input\" : \"BasicAlgorithm(PropertyA='FirstOne')\",";

    boost::shared_ptr<WorkspaceTester> input =
        boost::make_shared<WorkspaceTester>();
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
        builder.build(ws->getName(), ws->getTitle(), ws->getComment());

    std::vector<std::string> notebookLines;
    std::string line;
    std::istringstream buffer(notebookText);
    while (std::getline(buffer, line))
      notebookLines.push_back(line);

    // Check that the expected lines do appear in the output
    TS_ASSERT(std::find(notebookLines.cbegin(), notebookLines.cend(),
                        result_markdown) != notebookLines.cend())
    TS_ASSERT(std::find(notebookLines.cbegin(), notebookLines.cend(),
                        result_code) != notebookLines.cend())

    AnalysisDataService::Instance().remove("test_output_workspace");
    AnalysisDataService::Instance().remove("test_input_workspace");
  }

  void test_Partially_Unrolled() {
    std::string result_markdown =
        R"(               "source" : "Child algorithms of TopLevelAlgorithm")";
    std::string result_code =
        "               \"input\" : \"BasicAlgorithm(PropertyA='FirstOne')\",";

    boost::shared_ptr<WorkspaceTester> input =
        boost::make_shared<WorkspaceTester>();
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
        builder.build(ws->getName(), ws->getTitle(), ws->getComment());

    std::vector<std::string> notebookLines;
    std::string line;
    std::istringstream buffer(notebookText);
    while (std::getline(buffer, line))
      notebookLines.push_back(line);

    // Check that the expected lines do appear in the output
    TS_ASSERT(std::find(notebookLines.cbegin(), notebookLines.cend(),
                        result_markdown) != notebookLines.cend())
    TS_ASSERT(std::find(notebookLines.cbegin(), notebookLines.cend(),
                        result_code) != notebookLines.cend())

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
    boost::shared_ptr<WorkspaceTester> input =
        boost::make_shared<WorkspaceTester>();
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
        builder.build(ws->getName(), ws->getTitle(), ws->getComment());

    std::vector<std::string> notebookLines;
    std::string line;
    std::istringstream buffer(notebookText);
    while (std::getline(buffer, line))
      notebookLines.push_back(line);

    // Check that the expected line does appear in the output
    TS_ASSERT(std::find(notebookLines.cbegin(), notebookLines.cend(), result) !=
              notebookLines.cend())

    AnalysisDataService::Instance().remove("test_output_workspace");
    AnalysisDataService::Instance().remove("test_inp\\ut_workspace");
  }
};

#endif // MANTID_NOTEBOOKBUILDERTEST_H_
