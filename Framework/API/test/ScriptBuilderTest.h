// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SCRIPTBUILDERTEST_H_
#define MANTID_SCRIPTBUILDERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/ScriptBuilder.h"
#include "MantidKernel/PropertyManagerProperty.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {
std::string getAlgTimestamp(Mantid::API::HistoryView &historyView,
                            size_t index) {
  auto algList = historyView.getAlgorithmsList();
  TS_ASSERT(algList.size() >= index + 1);
  auto executionTime = algList[index].getAlgorithmHistory()->executionDate();
  return executionTime.toISO8601String();
}
} // namespace

class ScriptBuilderTest : public CxxTest::TestSuite {
public:
  static ScriptBuilderTest *createSuite() { return new ScriptBuilderTest; }

  static void destroySuite(ScriptBuilderTest *suite) { return delete suite; }

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

  // Algorithm has an input in the form of a PropertyManager
  class PropertyManagerInputAlgorithm : public Algorithm {
  public:
    const std::string name() const override {
      return "PropertyManagerInputAlgorithm";
    }
    int version() const override { return 1; }
    const std::string category() const override { return "Cat;Leopard;Mink"; }
    const std::string summary() const override {
      return "PropertyManagerInputAlgorithm";
    }
    void init() override {
      declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace", "", Direction::Input));
      declareProperty(
          std::make_unique<PropertyManagerProperty>("Dict", Direction::Input));
      declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "OutputWorkspace", "", Direction::Output));
    }
    void exec() override {
      setProperty("OutputWorkspace", boost::make_shared<WorkspaceTester>());
    }
  };

  class NewlineAlgorithm : public Algorithm {
  public:
    NewlineAlgorithm() : Algorithm() {}
    ~NewlineAlgorithm() override {}
    const std::string name() const override { return "Foo\n\rBar"; }
    const std::string summary() const override { return "Test"; }
    int version() const override { return 1; }
    const std::string category() const override { return "Cat;Leopard;Mink"; }

    void afterPropertySet(const std::string &name) override {
      if (name == "InputWorkspace")
        declareProperty("DynamicInputProperty", "");
    }

    void init() override {
      declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace", "", Direction::Input));
      declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "OutputWorkspace", "", Direction::Output));
      declareProperty("PropertyA", "Hello");
      declareProperty("PropertyB", "World");
    }
    void exec() override {
      declareProperty("DynamicProperty1", "value", Direction::Output);
      setPropertyValue("DynamicProperty1", "outputValue");

      boost::shared_ptr<MatrixWorkspace> output =
          boost::make_shared<WorkspaceTester>();
      setProperty("OutputWorkspace", output);
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
      declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace", "", Direction::Input));
      declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
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

  class AlgorithmWithDynamicProperty : public Algorithm {
  public:
    AlgorithmWithDynamicProperty() : Algorithm() {}
    ~AlgorithmWithDynamicProperty() override {}
    const std::string name() const override {
      return "AlgorithmWithDynamicProperty";
    }
    int version() const override { return 1; }
    const std::string category() const override { return "Cat;Leopard;Mink"; }
    const std::string summary() const override {
      return "AlgorithmWithDynamicProperty";
    }
    void afterPropertySet(const std::string &name) override {
      if (name == "InputWorkspace")
        declareProperty("DynamicInputProperty", "");
    }

    void init() override {
      declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace", "", Direction::Input));
      declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "OutputWorkspace", "", Direction::Output));
      declareProperty("PropertyA", "Hello");
      declareProperty("PropertyB", "World");
    }
    void exec() override {
      declareProperty("DynamicProperty1", "value", Direction::Output);
      setPropertyValue("DynamicProperty1", "outputValue");

      boost::shared_ptr<MatrixWorkspace> output =
          boost::make_shared<WorkspaceTester>();
      setProperty("OutputWorkspace", output);
    }
  };

  ScriptBuilderTest()
      : m_algFactory(AlgorithmFactory::Instance()),
        m_ads(AnalysisDataService::Instance()),
        m_testWS(boost::make_shared<WorkspaceTester>()) {
    m_algFactory.subscribe<TopLevelAlgorithm>();
    m_algFactory.subscribe<NestedAlgorithm>();
    m_algFactory.subscribe<BasicAlgorithm>();
    m_algFactory.subscribe<PropertyManagerInputAlgorithm>();
    m_algFactory.subscribe<SubAlgorithm>();
    m_algFactory.subscribe<NewlineAlgorithm>();
    m_algFactory.subscribe<AlgorithmWithDynamicProperty>();
    m_ads.addOrReplace("test_input_workspace", m_testWS);
  }
  ~ScriptBuilderTest() {
    m_algFactory.unsubscribe("TopLevelAlgorithm", 1);
    m_algFactory.unsubscribe("NestedAlgorithm", 1);
    m_algFactory.unsubscribe("BasicAlgorithm", 1);
    m_algFactory.unsubscribe("PropertyManagerInputAlgorithm", 1);
    m_algFactory.unsubscribe("SubAlgorithm", 1);
    m_algFactory.unsubscribe("AlgorithmWithDynamicProperty", 1);
    m_algFactory.unsubscribe("Foo\n\rBar", 1);
  }

  void test_Build_Simple() {
    std::string result[] = {"TopLevelAlgorithm(InputWorkspace='test_input_"
                            "workspace', "
                            "OutputWorkspace='test_output_workspace')",
                            ""};
    auto alg = m_algFactory.create("TopLevelAlgorithm", 1);
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", m_testWS);
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    auto ws = m_ads.retrieveWS<MatrixWorkspace>("test_output_workspace");
    auto wsHist = ws->getHistory();

    ScriptBuilder builder(wsHist.createView());
    std::string scriptText = builder.build();

    std::vector<std::string> scriptLines;
    boost::split(scriptLines, scriptText, boost::is_any_of("\n"));

    int i = 0;
    for (auto it = scriptLines.begin(); it != scriptLines.end(); ++it, ++i) {
      TS_ASSERT_EQUALS(*it, result[i])
    }

    m_ads.remove("test_output_workspace");
  }

  void test_Build_With_PropertyManagerProperty() {
    auto alg = m_algFactory.create("PropertyManagerInputAlgorithm", 1);
    auto propMgr = boost::make_shared<PropertyManager>();
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", m_testWS);
    propMgr->declareProperty("Int", 1);
    propMgr->declareProperty("String", "option");
    alg->setProperty("Dict", propMgr);
    const std::string outputName("test_Build_With_PropertyManagerProperty_Out");
    alg->setProperty("OutputWorkspace", outputName);
    alg->execute();

    auto outputWS = m_ads.retrieve(outputName);
    const auto &wsHist = outputWS->history();
    ScriptBuilder builder(wsHist.createView());
    const auto generatedText = builder.build();
    const auto expectedText =
        "PropertyManagerInputAlgorithm(InputWorkspace='test_input_workspace', "
        "Dict='{\"Int\":1,\"String\":\"option\"}', "
        "OutputWorkspace='test_Build_With_PropertyManagerProperty_Out')\n";

    TS_ASSERT_EQUALS(expectedText, generatedText)

    m_ads.remove(outputName);
  }

  void test_newline_chars_removed() {
    // Check that any newline chars are removed
    std::string result[] = {"FooBar(InputWorkspace='test_input_workspace',"
                            " OutputWorkspace='test_output_workspace')",
                            ""};

    auto alg = m_algFactory.create("Foo\n\rBar", 1);
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", m_testWS);
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    auto ws = m_ads.retrieveWS<MatrixWorkspace>("test_output_workspace");
    auto wsHist = ws->getHistory();

    ScriptBuilder builder(wsHist.createView());
    std::string scriptText = builder.build();

    std::vector<std::string> scriptLines;
    boost::split(scriptLines, scriptText, boost::is_any_of("\n"));

    int i = 0;
    for (auto it = scriptLines.begin(); it != scriptLines.end(); ++it, ++i) {
      TS_ASSERT_EQUALS(*it, result[i])
    }

    m_ads.remove("test_output_workspace");
  }

  void test_Build_Simple_Timestamped() {
    auto alg = m_algFactory.create("TopLevelAlgorithm", 1);
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", m_testWS);
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    auto ws = m_ads.retrieveWS<MatrixWorkspace>("test_output_workspace");

    const auto wsHistView = ws->getHistory().createView();
    auto executionTime = getAlgTimestamp(*wsHistView, 0);

    std::string algTimestamp{" # " + executionTime};

    std::string result[] = {"TopLevelAlgorithm(InputWorkspace='test_input_"
                            "workspace', "
                            "OutputWorkspace='test_output_workspace')" +
                                algTimestamp,
                            ""};

    const bool appendTimestamp = true;
    ScriptBuilder builder(wsHistView, "old", appendTimestamp);
    std::string scriptText = builder.build();

    std::vector<std::string> scriptLines;
    boost::split(scriptLines, scriptText, boost::is_any_of("\n"));

    int i = 0;
    for (auto it = scriptLines.begin(); it != scriptLines.end(); ++it, ++i) {
      TS_ASSERT_EQUALS(*it, result[i])
    }

    m_ads.remove("test_output_workspace");
  }

  void test_Build_Unrolled() {
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
    auto alg = m_algFactory.create("TopLevelAlgorithm", 1);
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", m_testWS);
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    auto ws = m_ads.retrieveWS<MatrixWorkspace>("test_output_workspace");
    auto wsHist = ws->getHistory();
    auto view = wsHist.createView();

    view->unrollAll();
    ScriptBuilder builder(view);
    std::string scriptText = builder.build();

    std::vector<std::string> scriptLines;
    boost::split(scriptLines, scriptText, boost::is_any_of("\n"));

    int i = 0;
    for (auto it = scriptLines.begin(); it != scriptLines.end(); ++it, ++i) {
      TS_ASSERT_EQUALS(*it, result[i])
    }

    m_ads.remove("test_output_workspace");
  }

  void test_Partially_Unrolled() {
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
    auto alg = m_algFactory.create("TopLevelAlgorithm", 1);
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", m_testWS);
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", "test_output_workspace");
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    auto ws = m_ads.retrieveWS<MatrixWorkspace>("test_output_workspace");
    auto wsHist = ws->getHistory();
    auto view = wsHist.createView();

    view->unroll(0);
    view->unroll(1);
    view->unroll(5);

    ScriptBuilder builder(view);
    std::string scriptText = builder.build();

    std::vector<std::string> scriptLines;
    boost::split(scriptLines, scriptText, boost::is_any_of("\n"));

    int i = 0;
    for (auto it = scriptLines.begin(); it != scriptLines.end(); ++it, ++i) {
      TS_ASSERT_EQUALS(*it, result[i])
    }

    m_ads.remove("test_output_workspace");
  }

  void test_Build_Simple_with_backslash() {
    // checks that property values with \ get prefixed with r, eg.
    // filename=r'c:\test\data.txt'
    std::string result[] = {"TopLevelAlgorithm(InputWorkspace=r'test_inp\\ut_"
                            "workspace', "
                            "OutputWorkspace='test_output_workspace')",
                            ""};
    boost::shared_ptr<WorkspaceTester> backSlashName =
        boost::make_shared<WorkspaceTester>();
    m_ads.addOrReplace("test_inp\\ut_workspace", backSlashName);

    auto alg = m_algFactory.create("TopLevelAlgorithm", 1);
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", backSlashName);
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    auto ws = m_ads.retrieveWS<MatrixWorkspace>("test_output_workspace");
    auto wsHist = ws->getHistory();

    ScriptBuilder builder(wsHist.createView());
    std::string scriptText = builder.build();

    std::vector<std::string> scriptLines;
    boost::split(scriptLines, scriptText, boost::is_any_of("\n"));

    int i = 0;
    for (auto it = scriptLines.begin(); it != scriptLines.end(); ++it, ++i) {
      TS_ASSERT_EQUALS(*it, result[i])
    }

    m_ads.remove("test_output_workspace");
    m_ads.remove("test_inp\\ut_workspace");
  }

  void test_Build_Dynamic_Property() {
    // importantly the Dynamic Property should not be written into the script
    std::string result =
        "AlgorithmWithDynamicProperty(InputWorkspace='test_input_workspace', "
        "OutputWorkspace='test_output_workspace', PropertyA='A', "
        "PropertyB='B', DynamicInputProperty='C')\n";

    auto alg = m_algFactory.create("AlgorithmWithDynamicProperty", 1);
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", m_testWS);
    alg->setProperty("PropertyA", "A");
    alg->setProperty("PropertyB", "B");
    alg->setProperty("DynamicInputProperty", "C");
    alg->setPropertyValue("OutputWorkspace", "test_output_workspace");
    alg->execute();

    auto ws = m_ads.retrieveWS<MatrixWorkspace>("test_output_workspace");
    auto wsHist = ws->getHistory();

    // check the dynamic property is in the history records
    const auto &hist_props = wsHist.getAlgorithmHistory(0)->getProperties();
    bool foundDynamicProperty = false;
    for (const auto &hist_prop : hist_props) {
      if (hist_prop->name() == "DynamicProperty1") {
        foundDynamicProperty = true;
      }
    }
    TSM_ASSERT("Could not find the dynamic property in the algorithm history.",
               foundDynamicProperty);

    ScriptBuilder builder(wsHist.createView());
    std::string scriptText = builder.build();

    // The dynamic property should not be in the script.
    TS_ASSERT_EQUALS(scriptText, result);

    m_ads.remove("test_output_workspace");
  }

private:
  AlgorithmFactoryImpl &m_algFactory;
  AnalysisDataServiceImpl &m_ads;
  MatrixWorkspace_sptr m_testWS;
};

#endif // MANTID_SCRIPTBUILDERTEST_H_
