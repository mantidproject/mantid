// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/IDTypes.h"
#include <map>

namespace Mantid {
namespace Algorithms {

/** PDCalibration : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL PDCalibration final : public API::Algorithm {
public:
  PDCalibration();
  ~PDCalibration();

  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  class FittedPeaks; // forward declare of private inner class

  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;
  API::MatrixWorkspace_sptr loadAndBin();
  API::MatrixWorkspace_sptr rebin(API::MatrixWorkspace_sptr wksp);
  API::MatrixWorkspace_sptr load(const std::string &filename);
  std::set<detid_t> detIdsForTable();
  void createCalTableHeader();
  void createCalTableFromExisting();
  void createCalTableNew();
  void createInformationWorkspaces();
  std::tuple<double, double, double> getDSpacingToTof(const std::set<detid_t> &detIds);
  std::vector<double> dSpacingWindows(const std::vector<double> &centres, const std::vector<double> &widthMax);
  std::vector<double> getTOFminmax(const double difc, const double difa, const double tzero);
  void setCalibrationValues(const detid_t detid, const double difc, const double difa, const double tzero);
  void fitDIFCtZeroDIFA_LM(const std::vector<double> &d, const std::vector<double> &tof,
                           const std::vector<double> &height2, double &difc, double &t0, double &difa);
  API::MatrixWorkspace_sptr calculateResolutionTable();

  /// NEW: convert peak positions in dSpacing to peak centers workspace
  std::pair<API::MatrixWorkspace_sptr, API::MatrixWorkspace_sptr>
  createTOFPeakCenterFitWindowWorkspaces(const API::MatrixWorkspace_sptr &dataws,
                                         const std::vector<double> &peakWindowMaxInDSpacing);

  API::ITableWorkspace_sptr sortTableWorkspace(API::ITableWorkspace_sptr &table);

  API::MatrixWorkspace_sptr m_uncalibratedWS{nullptr};
  API::ITableWorkspace_sptr m_calibrationTable{nullptr};
  API::ITableWorkspace_sptr m_peakPositionTable{nullptr};
  API::ITableWorkspace_sptr m_peakWidthTable{nullptr};
  API::ITableWorkspace_sptr m_peakHeightTable{nullptr};
  std::vector<double> m_peaksInDspacing;
  std::map<detid_t, size_t> m_detidToRow;
  int m_startWorkspaceIndex; ///< start index
  int m_stopWorkspaceIndex;  ///< stop index (workspace index of the last spectrum included)
  double m_tofMin{0.};       ///< first bin boundary when rebinning in TOF (user input)
  double m_tofMax{0.};       ///< last bin boundary when rebinning in TOF (user input)
  double m_tzeroMin{0.};
  double m_tzeroMax{0.};
  double m_difaMin{0.};
  double m_difaMax{0.};
  bool m_hasDasIds{false};
  size_t m_numberMaxParams{0};
};

} // namespace Algorithms
} // namespace Mantid
