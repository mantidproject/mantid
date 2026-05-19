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

  void testAvailableTypesIncludesInstrumentSpecificPlotTypesForPolrefOffspecAndCrisp() {
    auto provider = PlotOptionsProvider{};
    auto const expected = std::vector<PlotOutputType>{PlotOutputType::ReflectivityCurve, PlotOutputType::DetectorMap,
                                                      PlotOutputType::SpinAsymmetry, PlotOutputType::Alignment};

    TS_ASSERT_EQUALS(provider.availableTypes("POLREF"), expected);
    TS_ASSERT_EQUALS(provider.availableTypes("OFFSPEC"), expected);
    TS_ASSERT_EQUALS(provider.availableTypes("CRISP"), expected);
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

  void testDetectorMapOptionsUseColorfillPlot() {
    auto provider = PlotOptionsProvider{};

    auto const options = provider.optionsFor(PlotOutputType::DetectorMap, PlotLayout::Tiled);

    TS_ASSERT_EQUALS(options.outputType, PlotOutputType::DetectorMap);
    TS_ASSERT_EQUALS(options.plotStyle, PlotStyle::Colorfill);
    TS_ASSERT_EQUALS(options.layout, PlotLayout::Tiled);
    TS_ASSERT_EQUALS(options.xAxis.label, "Time of Flight");
    TS_ASSERT_EQUALS(options.yAxis.label, "Detector Index");
    TS_ASSERT_EQUALS(options.zAxis.label, "Intensity");
  }

  void testDetectorMapOptionsUseSelectedAxisUnits() {
    auto provider = PlotOptionsProvider{};

    auto const options = provider.optionsFor(
        PlotOutputOptions{PlotOutputType::DetectorMap, DetectorMapXAxis::Lambda, DetectorMapYAxis::Theta},
        PlotLayout::Tiled);

    TS_ASSERT_EQUALS(options.xAxis.label, "Lambda");
    TS_ASSERT_EQUALS(options.yAxis.label, "Theta");
  }

  void testSpinAsymmetryOptionsUseQzAndAsymmetryAxes() {
    auto provider = PlotOptionsProvider{};

    auto const options = provider.optionsFor(PlotOutputType::SpinAsymmetry, PlotLayout::Overplot);

    TS_ASSERT_EQUALS(options.outputType, PlotOutputType::SpinAsymmetry);
    TS_ASSERT_EQUALS(options.plotStyle, PlotStyle::Line);
    TS_ASSERT_EQUALS(options.layout, PlotLayout::Overplot);
    TS_ASSERT_EQUALS(options.xAxis.label, "Qz");
    TS_ASSERT_EQUALS(options.yAxis.label, "Spin Asymmetry");
    TS_ASSERT_EQUALS(options.yAxis.unit, "");
    TS_ASSERT(options.showErrors);
    TS_ASSERT(options.horizontalMarker);
    TS_ASSERT_EQUALS(*options.horizontalMarker, 0.0);
  }

  void testAlignmentOptionsUseDetectorIdAndIntegratedIntensityAxes() {
    auto provider = PlotOptionsProvider{};

    auto const options = provider.optionsFor(PlotOutputType::Alignment, PlotLayout::Individual);

    TS_ASSERT_EQUALS(options.outputType, PlotOutputType::Alignment);
    TS_ASSERT_EQUALS(options.plotStyle, PlotStyle::Line);
    TS_ASSERT_EQUALS(options.layout, PlotLayout::Individual);
    TS_ASSERT_EQUALS(options.xAxis.label, "Detector ID");
    TS_ASSERT_EQUALS(options.yAxis.label, "Integrated Intensity");
  }

  void testAlignmentOptionsUseSelectedXAxisUnits() {
    auto provider = PlotOptionsProvider{};

    auto const options =
        provider.optionsFor(PlotOutputOptions{PlotOutputType::Alignment, DetectorMapXAxis::TimeOfFlight,
                                              DetectorMapYAxis::DetectorId, AlignmentXAxis::Theta},
                            PlotLayout::Individual);

    TS_ASSERT_EQUALS(options.xAxis.label, "Theta");
  }
};
