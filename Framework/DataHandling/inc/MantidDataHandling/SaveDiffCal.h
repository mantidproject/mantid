// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"

namespace H5 {
class Group;
}

namespace Mantid {
namespace DataHandling {

/** SaveDiffCal : TODO: DESCRIPTION
 */
class MANTID_DATAHANDLING_DLL SaveDiffCal final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"LoadDiffCal"}; }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;

  void writeDoubleFieldZeros(H5::Group &group, const std::string &name);

  void writeDoubleFieldFromTable(H5::Group &group, const std::string &name);
  void writeIntFieldFromTable(H5::Group &group, const std::string &name);
  void writeDetIdsfromSVWS(H5::Group &group, const std::string &name,
                           const DataObjects::SpecialWorkspace2D_const_sptr &ws);
  void writeIntFieldFromSVWS(H5::Group &group, const std::string &name,
                             const DataObjects::SpecialWorkspace2D_const_sptr &ws);
  void generateDetidToIndex();
  void generateDetidToIndex(const DataObjects::SpecialWorkspace2D_const_sptr &ws);
  bool tableHasColumn(const std::string &ColumnName) const;

  // minimum of (CalibrationWorkspace_row_count,
  // GroupingWorkspace_histogram_count, MaskWorkspace_histogram_count)
  std::size_t m_numValues{0};
  API::ITableWorkspace_sptr m_calibrationWS;
  std::map<detid_t, size_t> m_detidToIndex;
};

} // namespace DataHandling
} // namespace Mantid
