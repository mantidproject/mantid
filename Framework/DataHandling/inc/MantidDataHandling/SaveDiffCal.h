// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVEDIFFCAL_H_
#define MANTID_DATAHANDLING_SAVEDIFFCAL_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidKernel/System.h"

namespace H5 {
class Group;
}

namespace Mantid {
namespace DataHandling {

/** SaveDiffCal : TODO: DESCRIPTION
 */
class DLLExport SaveDiffCal : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"LoadDiffCal"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;

  void writeDoubleFieldFromTable(H5::Group &group, const std::string &name);
  void writeIntFieldFromTable(H5::Group &group, const std::string &name);
  void writeIntFieldFromSVWS(H5::Group &group, const std::string &name,
                             DataObjects::SpecialWorkspace2D_const_sptr ws);
  void generateDetidToIndex();
  bool tableHasColumn(const std::string &ColumnName) const;

  std::size_t m_numValues{0};
  API::ITableWorkspace_sptr m_calibrationWS;
  std::map<detid_t, size_t> m_detidToIndex;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAVEDIFFCAL_H_ */
