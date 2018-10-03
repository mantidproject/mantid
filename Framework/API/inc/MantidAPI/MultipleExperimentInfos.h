// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_MULTIPLEEXPERIMENTINFOS_H_
#define MANTID_API_MULTIPLEEXPERIMENTINFOS_H_

#include "MantidAPI/ExperimentInfo.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace API {

/** Small class that allows a MDEventWorkspace or a MDHistoWorkspace
  to hold several ExperimentInfo classes.

  @date 2011-11-28
*/
class DLLExport MultipleExperimentInfos {
public:
  MultipleExperimentInfos() = default;
  MultipleExperimentInfos(const MultipleExperimentInfos &other);
  virtual ~MultipleExperimentInfos() = default;

  ExperimentInfo_sptr getExperimentInfo(const uint16_t runIndex);

  ExperimentInfo_const_sptr getExperimentInfo(const uint16_t runIndex) const;

  uint16_t addExperimentInfo(ExperimentInfo_sptr ei);

  void setExperimentInfo(const uint16_t runIndex, ExperimentInfo_sptr ei);

  uint16_t getNumExperimentInfo() const;

  void copyExperimentInfos(const MultipleExperimentInfos &other);

  // Check if this class has an oriented lattice on any sample object
  bool hasOrientedLattice() const;

protected:
  /// Returns a string description of the object
  const std::string toString() const;

private:
  /// Vector for each ExperimentInfo class
  std::vector<ExperimentInfo_sptr> m_expInfos;
};

using MultipleExperimentInfos_sptr = boost::shared_ptr<MultipleExperimentInfos>;
using MultipleExperimentInfos_const_sptr =
    boost::shared_ptr<const MultipleExperimentInfos>;

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_MULTIPLEEXPERIMENTINFOS_H_ */
