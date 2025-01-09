// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"

namespace Mantid {
namespace API {

/** Small class that allows a MDEventWorkspace or a MDHistoWorkspace
  to hold several ExperimentInfo classes.

  @date 2011-11-28
*/
class MANTID_API_DLL MultipleExperimentInfos {
public:
  MultipleExperimentInfos() = default;
  MultipleExperimentInfos(const MultipleExperimentInfos &other);
  MultipleExperimentInfos &operator=(const MultipleExperimentInfos &other);
  virtual ~MultipleExperimentInfos() = default;

  ExperimentInfo_sptr getExperimentInfo(const uint16_t expInfoIndex);

  ExperimentInfo_const_sptr getExperimentInfo(const uint16_t expInfoIndex) const;

  uint16_t addExperimentInfo(const ExperimentInfo_sptr &ei);

  void setExperimentInfo(const uint16_t expInfoIndex, ExperimentInfo_sptr ei);

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

using MultipleExperimentInfos_sptr = std::shared_ptr<MultipleExperimentInfos>;
using MultipleExperimentInfos_const_sptr = std::shared_ptr<const MultipleExperimentInfos>;

} // namespace API
} // namespace Mantid
