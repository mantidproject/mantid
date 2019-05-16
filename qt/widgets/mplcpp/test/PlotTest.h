// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_PLOTTEST_H
#define MPLCPP_PLOTTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidQtWidgets/MplCpp/Plot.h"

using namespace MantidQt::Widgets::Common;
using namespace MantidQt::Widgets::MplCpp;
using namespace Mantid::PythonInterface;

namespace {
void setMatplotlibBackend() {
  auto mpl = Python::NewRef(PyImport_ImportModule("matplotlib"));
  mpl.attr("use")("Agg");
}
} // namespace

class PlotTest : public CxxTest::TestSuite {
public:
  static PlotTest *createSuite() { return new PlotTest; }
  static void destroySuite(PlotTest *suite) { delete suite; }

  void setUp() override {
    Mantid::API::FrameworkManager::Instance();
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "CreateSampleWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "ws");
    alg->execute();

    setMatplotlibBackend();
  }

  void testPlottingWorksWithWorkspaceIndex() {
    std::vector<std::string> workspaces = {"ws"};
    std::vector<int> index = {1};
    TS_ASSERT_THROWS_NOTHING(plot(workspaces, boost::none, index))
  }

  void testPlottingWorksWithSpecNum() {
    std::vector<std::string> workspaces = {"ws"};
    std::vector<int> index = {1};
    TS_ASSERT_THROWS_NOTHING(plot(workspaces, index, boost::none))
  }

  void testPlottingThrowsWithSpecNumAndWorkspaceIndex() {
    std::vector<std::string> workspaces = {"ws"};
    std::vector<int> index = {1};
    TS_ASSERT_THROWS(plot(workspaces, index, index),
                     const std::invalid_argument &)
  }

  void testPlottingWithPlotKwargs() {
    std::vector<std::string> workspaces = {"ws"};
    std::vector<int> index = {1};
    QHash<QString, QVariant> hash;
    hash.insert(QString("linewidth"), QVariant(10));
    TS_ASSERT_THROWS_NOTHING(
        plot(workspaces, index, boost::none, boost::none, hash))
  }

  void testPlottingWithIncorrectPlotKwargsThrows() {
    std::vector<std::string> workspaces = {"ws"};
    std::vector<int> index = {1};
    QHash<QString, QVariant> hash;
    hash.insert(QString("asdasdasdasdasd"), QVariant(1));
    TS_ASSERT_THROWS(plot(workspaces, index, boost::none, boost::none, hash),
                     const PythonException &)
  }

  void testPlottingWithAxProperties() {
    std::vector<std::string> workspaces = {"ws"};
    std::vector<int> index = {1};
    QHash<QString, QVariant> hash;
    hash.insert(QString("xscale"), QVariant("log"));
    TS_ASSERT_THROWS_NOTHING(
        plot(workspaces, index, boost::none, boost::none, boost::none, hash))
  }

  void testPlottingWithIncorrectAxPropertiesThrows() {
    std::vector<std::string> workspaces = {"ws"};
    std::vector<int> index = {1};
    QHash<QString, QVariant> hash;
    hash.insert(QString("asdasdasdasdasd"), QVariant(QString(1)));
    TS_ASSERT_THROWS(
        plot(workspaces, index, boost::none, boost::none, boost::none, hash),
        const PythonException &)
  }

  void testPlottingWithWindowTitle() {
    std::vector<std::string> workspaces = {"ws"};
    std::vector<int> index = {1};
    std::string window_title = "window_title";
    TS_ASSERT_THROWS_NOTHING(plot(workspaces, boost::none, index, boost::none,
                                  boost::none, boost::none, window_title))
  }

  void testPlottingWithErrors() {
    std::vector<std::string> workspaces = {"ws"};
    std::vector<int> index = {1};
    TS_ASSERT_THROWS_NOTHING(plot(workspaces, boost::none, index, boost::none,
                                  boost::none, boost::none, boost::none, true))
  }

  void testPlottingWithOverplotAndMultipleWorkspaces() {
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "CreateSampleWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "ws1");
    alg->execute();
    alg->setProperty("OutputWorkspace", "ws2");
    alg->execute();
    std::vector<std::string> workspaces = {"ws", "ws1", "ws2"};
    std::vector<int> index = {1, 1, 1};
    TS_ASSERT_THROWS_NOTHING(plot(workspaces, boost::none, index, boost::none,
                                  boost::none, boost::none, boost::none, false,
                                  true))
  }

  void testPlottingWithoutOverplotButWithMultipleWorkspace() {
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "CreateSampleWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "ws1");
    alg->execute();
    alg->setProperty("OutputWorkspace", "ws2");
    alg->execute();

    std::vector<std::string> workspaces = {"ws", "ws1", "ws2"};
    std::vector<int> index = {1, 1, 1};
    TS_ASSERT_THROWS_NOTHING(plot(workspaces, boost::none, index, boost::none,
                                  boost::none, boost::none, boost::none, false,
                                  false))
  }
};

#endif // MPLCPP_PLOTTEST_H