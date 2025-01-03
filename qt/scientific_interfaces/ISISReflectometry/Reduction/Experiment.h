// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "AnalysisMode.h"
#include "BackgroundSubtraction.h"
#include "Common/DllConfig.h"
#include "FloodCorrections.h"
#include "LookupRow.h"
#include "LookupTable.h"
#include "PolarizationCorrections.h"
#include "ReductionType.h"
#include "SummationType.h"
#include "TransmissionStitchOptions.h"
#include <boost/optional.hpp>
#include <map>
#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

class Row;
class PreviewRow;

/** @class Experiment

    The Experiment model holds all settings relating to the Experiment Settings
    tab on the GUI
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL Experiment {
public:
  Experiment();
  Experiment(AnalysisMode analysisMode, ReductionType reductionType, SummationType summationType,
             bool includePartialBins, bool debug, BackgroundSubtraction backgroundSubtraction,
             PolarizationCorrections polarizationCorrections, FloodCorrections floodCorrections,
             TransmissionStitchOptions transmissionStitchOptions, std::map<std::string, std::string> stitchParameters,
             LookupTable lookupTable);

  AnalysisMode analysisMode() const;
  ReductionType reductionType() const;
  SummationType summationType() const;
  bool includePartialBins() const;
  bool debug() const;
  BackgroundSubtraction const &backgroundSubtraction() const;
  PolarizationCorrections const &polarizationCorrections() const;
  FloodCorrections const &floodCorrections() const;
  TransmissionStitchOptions transmissionStitchOptions() const;
  std::map<std::string, std::string> stitchParameters() const;
  std::string stitchParametersString() const;
  std::vector<LookupRow> const &lookupTableRows() const;
  std::vector<LookupRow::ValueArray> lookupTableToArray() const;

  boost::optional<LookupRow> findLookupRow(Row const &row, double tolerance) const;
  boost::optional<LookupRow> findLookupRow(PreviewRow const &previewRow, double tolerance) const;
  boost::optional<LookupRow> findWildcardLookupRow() const;

  void updateLookupRow(LookupRow lookupRow, double tolerance);

  std::optional<size_t> getLookupRowIndexFromRow(Row const &row, double tolerance) const;

private:
  AnalysisMode m_analysisMode;
  ReductionType m_reductionType;
  SummationType m_summationType;
  bool m_includePartialBins;
  bool m_debug;

  BackgroundSubtraction m_backgroundSubtraction;
  PolarizationCorrections m_polarizationCorrections;
  FloodCorrections m_floodCorrections;
  TransmissionStitchOptions m_transmissionStitchOptions;

  std::map<std::string, std::string> m_stitchParameters;
  LookupTable m_lookupTable;

  friend bool operator==(Experiment const &lhs, Experiment const &rhs);
  friend bool operator!=(Experiment const &lhs, Experiment const &rhs);
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
