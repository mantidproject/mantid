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

namespace NeXus {
class File;
}

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
    double value(const detid_t detid) const;

  private:
    std::vector<double> m_calibration;
    detid_t m_detid_offset;
  };

private:
  void init() override;
  void exec() override;

  void initCalibrationConstants(API::MatrixWorkspace_sptr &wksp);

  void loadTOF(std::unique_ptr<std::vector<float>> &data, ::NeXus::File &h5file);
  void loadDetid(std::unique_ptr<std::vector<uint32_t>> &data, ::NeXus::File &h5file);
  void loadPulseTimes(std::unique_ptr<std::vector<double>> &data, ::NeXus::File &h5file);
  void loadEventIndex(std::unique_ptr<std::vector<uint64_t>> &data, ::NeXus::File &h5file);

  std::map<detid_t, double> m_calibration;
  bool is_time_filtered{false};
  size_t pulse_start_index{0};
  size_t pulse_stop_index{std::numeric_limits<size_t>::max()};
  size_t start_event{0};
  size_t stop_event{std::numeric_limits<size_t>::max()};
  /// Index to load start at in the file
  std::vector<int64_t> loadStart;
  /// How much to load in the file
  std::vector<int64_t> loadSize;
};

} // namespace DataHandling
} // namespace Mantid
