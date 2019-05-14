// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_MANTIDAXESTEST_H
#define MPLCPP_MANTIDAXESTEST_H

#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidQtWidgets/MplCpp/MantidAxes.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace MantidQt::Widgets::MplCpp;

class MantidAxesTest : public CxxTest::TestSuite {
public:
  static MantidAxesTest *createSuite() { return new MantidAxesTest; }
  static void destroySuite(MantidAxesTest *suite) { delete suite; }

public:
  // ----------------- success tests ---------------------
  void testConstructWithPyObjectAxes() {
    TS_ASSERT_THROWS_NOTHING(MantidAxes axes(pyAxes()));
  }

  void testPlotWithWorkspaceReturnsLineForValidWsIndex() {
    using Mantid::DataObjects::create;
    const auto ws = boost::shared_ptr<Workspace2D>(
        create<Workspace2D>(2, Histogram(BinEdges{1, 2, 4})).release());
    MantidAxes axes{pyAxes()};
    auto line = axes.plot(ws, 0, "red", "mylabel");
    TS_ASSERT_EQUALS(1.5, line.pyobj().attr("get_xdata")()[0]);
    TS_ASSERT_EQUALS("red", line.pyobj().attr("get_color")());
  }

  void testReplaceArtist() {
    MantidAxes axes{pyAxes()};
    const auto wsOld = boost::shared_ptr<Workspace2D>(
        create<Workspace2D>(2, Histogram(BinEdges{1, 2, 4})).release());
    // replacement is based on names and the only way to set a name is to
    // add the object to the ADS
    auto &ads = AnalysisDataService::Instance();
    ads.addOrReplace("name", wsOld);
    axes.plot(wsOld, 0, "red", "mylabel");
    const auto wsNew = boost::shared_ptr<Workspace2D>(
        create<Workspace2D>(2, Histogram(BinEdges{2, 3, 5})).release());
    ads.addOrReplace("name", wsNew);
    axes.replaceWorkspaceArtists(wsNew);

    auto newLine = axes.pyobj().attr("lines")[0];
    TS_ASSERT_EQUALS(2.5, newLine.attr("get_xdata")()[0]);
    TS_ASSERT_EQUALS("red", newLine.attr("get_color")());

    ads.remove("name");
  }

  // ----------------- failure tests ----------------------
  void testPlotWithWorkspaceInvalidWsIndexThrows() {
    using Mantid::DataObjects::create;
    const auto ws = boost::shared_ptr<Workspace2D>(
        create<Workspace2D>(2, Histogram(BinEdges{1, 2, 4})).release());
    MantidAxes axes{pyAxes()};
    TS_ASSERT_THROWS(axes.plot(ws, 2, "red", "mylabel"),
                     Python::ErrorAlreadySet);
  }

private:
  Python::Object pyAxes() {
    // An Axes requires a figure and rectangle definition
    // to be constructible
    const Python::Object figureModule{
        Python::NewRef(PyImport_ImportModule("matplotlib.figure"))};
    const Python::Object figure{figureModule.attr("Figure")()};
    const Python::Object rect{
        Python::NewRef(Py_BuildValue("(iiii)", 0, 0, 1, 1))};
    const Python::Object plotsModule{
        Python::NewRef(PyImport_ImportModule("mantid.plots"))};
    return plotsModule.attr("MantidAxes")(figure, rect);
  }
};

#endif // MPLCPP_MANTIDAXESTEST_H
