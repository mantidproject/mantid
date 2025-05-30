// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidGeometry/IDTypes.h"

namespace Mantid {
namespace DataHandling {

/** AlignAndFocusPowderSlim : TODO: DESCRIPTION
 */
class MANTID_DATAHANDLING_DLL AlignAndFocusPowderSlim : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

  class MANTID_DATAHANDLING_DLL BankCalibration {
  public:
    BankCalibration(const detid_t idmin, const detid_t idmax, const std::map<detid_t, double> &calibration_map);
    const double &value(const detid_t detid) const;
    const detid_t &idmin() const;
    detid_t idmax() const;

  private:
    std::vector<double> m_calibration;
    detid_t m_detid_offset;
  };

private:
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;

  API::MatrixWorkspace_sptr createOutputWorkspace(const size_t numHist, const bool linearBins, const double x_delta);
  API::MatrixWorkspace_sptr editInstrumentGeometry(API::MatrixWorkspace_sptr &wksp, const double l1,
                                                   const std::vector<double> &polars,
                                                   const std::vector<specnum_t> &specids,
                                                   const std::vector<double> &l2s,
                                                   const std::vector<double> &azimuthals);
  void initCalibrationConstants(API::MatrixWorkspace_sptr &wksp, const std::vector<double> &difc_focus);
  void loadCalFile(const API::Workspace_sptr &inputWS, const std::string &filename,
                   const std::vector<double> &difc_focus);

  std::map<detid_t, double> m_calibration; // detid: 1/difc
  std::set<detid_t> m_masked;
  bool is_time_filtered{false};
  size_t pulse_start_index{0};
  size_t pulse_stop_index{std::numeric_limits<size_t>::max()};
  /// Index to load start at in the file
  std::vector<int64_t> loadStart;
  /// How much to load in the file
  std::vector<int64_t> loadSize;
};

} // namespace DataHandling
} // namespace Mantid
