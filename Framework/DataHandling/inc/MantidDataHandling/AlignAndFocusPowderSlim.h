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
#include "MantidKernel/TimeROI.h"

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

/** AlignAndFocusPowderSlim : TODO: DESCRIPTION
 */
class MANTID_DATAHANDLING_DLL AlignAndFocusPowderSlim : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

private:
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;

  API::MatrixWorkspace_sptr createOutputWorkspace();
  API::MatrixWorkspace_sptr editInstrumentGeometry(API::MatrixWorkspace_sptr &wksp, const double l1,
                                                   const std::vector<double> &polars,
                                                   const std::vector<specnum_t> &specids,
                                                   const std::vector<double> &l2s,
                                                   const std::vector<double> &azimuthals);
  API::MatrixWorkspace_sptr convertToTOF(API::MatrixWorkspace_sptr &wksp);
  void initCalibrationConstants(API::MatrixWorkspace_sptr &wksp, const std::vector<double> &difc_focus);
  void loadCalFile(const API::Workspace_sptr &inputWS, const std::string &filename,
                   const std::vector<double> &difc_focus);
  void determinePulseIndices(const API::MatrixWorkspace_sptr &wksp);
  Kernel::TimeROI timeROIFromSplitterWorkspace(const Types::Core::DateAndTime &);

  std::map<detid_t, double> m_calibration; // detid: 1/difc
  std::set<detid_t> m_masked;
  bool is_time_filtered{false};
  /// Index to load start at in the file
  std::vector<int64_t> loadStart;
  /// How much to load in the file
  std::vector<int64_t> loadSize;
  std::vector<std::pair<size_t, size_t>> pulse_indices;
};
} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
