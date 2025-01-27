// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {

namespace HistogramData {
class HistogramY;
class HistogramE;
} // namespace HistogramData

namespace CurveFitting {
namespace Algorithms {

class MANTID_CURVEFITTING_DLL NormaliseByPeakArea final : public API::Algorithm {
public:
  NormaliseByPeakArea();

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Normalises the input data by the area of of peak defined by the "
           "input mass value.";
  }

  const std::vector<std::string> seeAlso() const override { return {"MonitorEfficiencyCorUser", "Divide"}; }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  /// Check and store appropriate input data
  void retrieveInputs();
  /// Create the output workspaces
  void createOutputWorkspaces(const API::MatrixWorkspace_sptr &yspaceIn);
  /// Set the units meta-data
  void setUnitsToMomentum(const API::MatrixWorkspace_sptr &workspace);

  /// Convert input workspace to Y coordinates for fitting
  API::MatrixWorkspace_sptr convertInputToY();
  /// Fit the mass peak & find the area value
  double fitToMassPeak(const API::MatrixWorkspace_sptr &yspace, const size_t index);
  /// Normalise given TOF spectrum
  void normaliseTOFData(const double area, const size_t index);
  /// Stores/accumulates the results
  void saveToOutput(const API::MatrixWorkspace_sptr &accumWS, const Kernel::cow_ptr<HistogramData::HistogramY> &yValues,
                    const Kernel::cow_ptr<HistogramData::HistogramE> &eValues, const size_t index);
  /// Symmetrises the data in yspace about the origin
  void symmetriseYSpace();

  API::MatrixWorkspace_sptr m_inputWS;
  /// The input mass in AMU
  double m_mass;
  /// Flag to indicate if results are to be summed
  bool m_sumResults;
  /// Normalised output in TOF
  API::MatrixWorkspace_sptr m_normalisedWS;
  /// Input data converted (and possible summed) to Y space
  API::MatrixWorkspace_sptr m_yspaceWS;
  /// Fitted output
  API::MatrixWorkspace_sptr m_fittedWS;
  /// Fitted output
  API::MatrixWorkspace_sptr m_symmetrisedWS;

  /// Reporting
  std::unique_ptr<API::Progress> m_progress;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
