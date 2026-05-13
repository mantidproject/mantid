// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Common/PlotOptionsProvider.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class PlotOptionsProviderTest : public CxxTest::TestSuite {
public:
  void testAvailableTypesContainsReflectivityCurveForInstrument() {
    auto provider = PlotOptionsProvider{};

    auto const availableTypes = provider.availableTypes("INTER");

    TS_ASSERT_EQUALS(availableTypes.size(), 1);
    TS_ASSERT_EQUALS(availableTypes[0], PlotOutputType::ReflectivityCurve);
  }

  void testReflectivityCurveOptionsAreConfiguredAsCurrentRunsTabPlot() {
    auto provider = PlotOptionsProvider{};

    auto const options = provider.optionsFor(PlotOutputType::ReflectivityCurve, PlotLayout::Individual);

    TS_ASSERT_EQUALS(options.outputType, PlotOutputType::ReflectivityCurve);
    TS_ASSERT_EQUALS(options.plotStyle, PlotStyle::Line);
    TS_ASSERT_EQUALS(options.layout, PlotLayout::Individual);
    TS_ASSERT_EQUALS(options.xAxis.scale, AxisScale::Log);
    TS_ASSERT_EQUALS(options.yAxis.scale, AxisScale::Log);
    TS_ASSERT(options.showErrors);
  }

  void testStitchedReflectivityCurveOptionsPreserveRequestedLayout() {
    auto provider = PlotOptionsProvider{};

    auto const options = provider.optionsFor(PlotOutputType::StitchedReflectivityCurve, PlotLayout::Tiled);

    TS_ASSERT_EQUALS(options.outputType, PlotOutputType::StitchedReflectivityCurve);
    TS_ASSERT_EQUALS(options.layout, PlotLayout::Tiled);
    TS_ASSERT_EQUALS(options.plotStyle, PlotStyle::Line);
  }
};
