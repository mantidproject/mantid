// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_PLOTTEST_H
#define MPLCPP_PLOTTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidQtWidgets/MplCpp/Plot.h"

using namespace MantidQt::Widgets::Common;
using namespace MantidQt::Widgets::MplCpp;
using namespace Mantid::API;
using namespace Mantid::PythonInterface;

class PlotTest : public CxxTest::TestSuite {
public:
  static PlotTest *createSuite() { return new PlotTest; }
  static void destroySuite(PlotTest *suite) { delete suite; }

  void setUp() override { createTestWorkspaceInADS(m_testws_name); }

  void testPlottingWorksWithWorkspaceIndex() {
    const std::vector<std::string> workspaces = {m_testws_name};
    const std::vector<int> index = {1};
    TS_ASSERT_THROWS_NOTHING(plot(workspaces, boost::none, index))
  }

  void testPlottingWorksQStrings() {
    const QStringList workspaces = {m_testws_name};
    const std::vector<int> index = {1};
    TS_ASSERT_THROWS_NOTHING(plot(workspaces, boost::none, index))
  }

  void testPlottingWorksWithSpecNum() {
    const std::vector<std::string> workspaces = {m_testws_name};
    const std::vector<int> index = {1};
    TS_ASSERT_THROWS_NOTHING(plot(workspaces, index, boost::none))
  }

  void testPlottingThrowsWithSpecNumAndWorkspaceIndex() {
    const std::vector<std::string> workspaces = {m_testws_name};
    const std::vector<int> index = {1};
    TS_ASSERT_THROWS(plot(workspaces, index, index),
                     const std::invalid_argument &)
  }

  void testPlottingWithPlotKwargs() {
    const std::vector<std::string> workspaces = {m_testws_name};
    const std::vector<int> index = {1};
    QHash<QString, QVariant> hash;
    hash.insert(QString("linewidth"), QVariant(10));
    TS_ASSERT_THROWS_NOTHING(
        plot(workspaces, index, boost::none, boost::none, hash))
  }

  void testPlottingWorksWhenPlottingABin() {
    const std::vector<std::string> workspaces = {m_testws_name};
    const std::vector<int> index = {1};
    QHash<QString, QVariant> hash;
    hash["axis"] = static_cast<int>(MantidAxType::Bin);

    TS_ASSERT_THROWS_NOTHING(
        plot(workspaces, index, boost::none, boost::none, hash))
  }

  void testPlottingWithIncorrectPlotKwargsThrows() {
    const std::vector<std::string> workspaces = {m_testws_name};
    const std::vector<int> index = {1};
    QHash<QString, QVariant> hash;
    hash.insert(QString("asdasdasdasdasd"), QVariant(1));
    TS_ASSERT_THROWS(plot(workspaces, index, boost::none, boost::none, hash),
                     const PythonException &)
  }

  void testPlottingWithAxProperties() {
    const std::vector<std::string> workspaces = {m_testws_name};
    const std::vector<int> index = {1};
    QHash<QString, QVariant> hash;
    hash.insert(QString("xscale"), QVariant("log"));
    TS_ASSERT_THROWS_NOTHING(
        plot(workspaces, index, boost::none, boost::none, boost::none, hash))
  }

  void testPlottingWithIncorrectAxPropertiesThrows() {
    const std::vector<std::string> workspaces = {m_testws_name};
    const std::vector<int> index = {1};
    QHash<QString, QVariant> hash;
    hash.insert(QString("asdasdasdasdasd"), QVariant(QString(1)));
    TS_ASSERT_THROWS(
        plot(workspaces, index, boost::none, boost::none, boost::none, hash),
        const PythonException &)
  }

  void testPlottingWithWindowTitle() {
    const std::vector<std::string> workspaces = {m_testws_name};
    const std::vector<int> index = {1};
    const std::string window_title = "window_title";
    TS_ASSERT_THROWS_NOTHING(plot(workspaces, boost::none, index, boost::none,
                                  boost::none, boost::none, window_title))
  }

  void testPlottingWithErrors() {
    const std::vector<std::string> workspaces = {m_testws_name};
    const std::vector<int> index = {1};
    TS_ASSERT_THROWS_NOTHING(plot(workspaces, boost::none, index, boost::none,
                                  boost::none, boost::none, boost::none, true))
  }

  void testPlottingWithOverplotAndMultipleWorkspaces() {
    createTestWorkspaceInADS("ws1");
    createTestWorkspaceInADS("ws2");
    const std::vector<std::string> workspaces = {m_testws_name, "ws1", "ws2"};
    const std::vector<int> index = {1, 1, 1};
    TS_ASSERT_THROWS_NOTHING(plot(workspaces, boost::none, index, boost::none,
                                  boost::none, boost::none, boost::none, false,
                                  true))
  }

  void testPlottingWithoutOverplotButWithMultipleWorkspace() {
    createTestWorkspaceInADS("ws1");
    createTestWorkspaceInADS("ws2");
    const std::vector<std::string> workspaces = {"ws", "ws1", "ws2"};
    const std::vector<int> index = {1, 1, 1};
    TS_ASSERT_THROWS_NOTHING(plot(workspaces, boost::none, index, boost::none,
                                  boost::none, boost::none, boost::none, false,
                                  false))
  }

  void testPcolormesh() {
    const QStringList workspaces = {m_testws_name};
    TS_ASSERT_THROWS_NOTHING(pcolormesh(workspaces));
  }

private:
  void createTestWorkspaceInADS(const std::string &name) {
    constexpr int nhist{2};
    constexpr int ny{2};
    auto testWS =
        WorkspaceFactory::Instance().create("Workspace2D", nhist, ny, ny);
    AnalysisDataService::Instance().addOrReplace(name, testWS);
  }

  constexpr static auto m_testws_name = "ws";
};

#endif // MPLCPP_PLOTTEST_H
