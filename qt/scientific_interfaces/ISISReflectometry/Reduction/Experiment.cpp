// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Experiment.h"
#include "LookupTable.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include <boost/optional.hpp>
#include <cmath>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

Experiment::Experiment()
    : m_analysisMode(AnalysisMode::PointDetector), m_reductionType(ReductionType::Normal),
      m_summationType(SummationType::SumInLambda), m_includePartialBins(false), m_debug(false),
      m_backgroundSubtraction(BackgroundSubtraction()),
      m_polarizationCorrections(PolarizationCorrections(PolarizationCorrectionType::None)),
      m_floodCorrections(FloodCorrections(FloodCorrectionType::Workspace)), m_transmissionStitchOptions(),
      m_stitchParameters(std::map<std::string, std::string>()),
      m_lookupTable(LookupTable({LookupRow(boost::none, std::nullopt, TransmissionRunPair(), boost::none, RangeInQ(),
                                           boost::none, ProcessingInstructions(), boost::none, boost::none)})) {}

Experiment::Experiment(AnalysisMode analysisMode, ReductionType reductionType, SummationType summationType,
                       bool includePartialBins, bool debug, BackgroundSubtraction backgroundSubtraction,
                       PolarizationCorrections polarizationCorrections, FloodCorrections floodCorrections,
                       TransmissionStitchOptions transmissionStitchOptions,

                       std::map<std::string, std::string> stitchParameters,

                       LookupTable lookupTable)
    : m_analysisMode(analysisMode), m_reductionType(reductionType), m_summationType(summationType),
      m_includePartialBins(includePartialBins), m_debug(debug), m_backgroundSubtraction(backgroundSubtraction),
      m_polarizationCorrections(polarizationCorrections), m_floodCorrections(std::move(floodCorrections)),
      m_transmissionStitchOptions(std::move(transmissionStitchOptions)),
      m_stitchParameters(std::move(stitchParameters)), m_lookupTable(std::move(lookupTable)) {}

AnalysisMode Experiment::analysisMode() const { return m_analysisMode; }
ReductionType Experiment::reductionType() const { return m_reductionType; }
SummationType Experiment::summationType() const { return m_summationType; }
bool Experiment::includePartialBins() const { return m_includePartialBins; }
bool Experiment::debug() const { return m_debug; }

BackgroundSubtraction const &Experiment::backgroundSubtraction() const { return m_backgroundSubtraction; }

PolarizationCorrections const &Experiment::polarizationCorrections() const { return m_polarizationCorrections; }
FloodCorrections const &Experiment::floodCorrections() const { return m_floodCorrections; }

TransmissionStitchOptions Experiment::transmissionStitchOptions() const { return m_transmissionStitchOptions; }

std::map<std::string, std::string> Experiment::stitchParameters() const { return m_stitchParameters; }

std::string Experiment::stitchParametersString() const {
  return MantidQt::MantidWidgets::optionsToString(m_stitchParameters);
}

std::vector<LookupRow> const &Experiment::lookupTableRows() const { return m_lookupTable.rows(); }

std::vector<LookupRow::ValueArray> Experiment::lookupTableToArray() const { return m_lookupTable.toValueArray(); }

boost::optional<LookupRow> Experiment::findLookupRow(Row const &row, double tolerance) const {
  return m_lookupTable.findLookupRow(row, tolerance);
}

boost::optional<LookupRow> Experiment::findLookupRow(PreviewRow const &previewRow, double tolerance) const {
  return m_lookupTable.findLookupRow(previewRow, tolerance);
}

boost::optional<LookupRow> Experiment::findWildcardLookupRow() const { return m_lookupTable.findWildcardLookupRow(); }

void Experiment::updateLookupRow(LookupRow lookupRow, double tolerance) {
  m_lookupTable.updateLookupRow(std::move(lookupRow), tolerance);
}

std::optional<size_t> Experiment::getLookupRowIndexFromRow(Row const &row, double tolerance) const {
  if (auto const lookupRow = m_lookupTable.findLookupRow(row, tolerance)) {
    return m_lookupTable.getIndex(lookupRow.get());
  }
  return std::nullopt;
}

bool operator!=(Experiment const &lhs, Experiment const &rhs) { return !operator==(lhs, rhs); }

bool operator==(Experiment const &lhs, Experiment const &rhs) {
  return lhs.analysisMode() == rhs.analysisMode() && lhs.reductionType() == rhs.reductionType() &&
         lhs.summationType() == rhs.summationType() && lhs.includePartialBins() == rhs.includePartialBins() &&
         lhs.debug() == rhs.debug() && lhs.backgroundSubtraction() == rhs.backgroundSubtraction() &&
         lhs.polarizationCorrections() == rhs.polarizationCorrections() &&
         lhs.floodCorrections() == rhs.floodCorrections() &&
         lhs.transmissionStitchOptions() == rhs.transmissionStitchOptions() &&
         lhs.stitchParameters() == rhs.stitchParameters() && lhs.m_lookupTable == rhs.m_lookupTable;
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
