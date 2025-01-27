// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MultipleExperimentInfos.h"

namespace Mantid {
namespace API {

/** Abstract interface to MDHistoWorkspace,
  for use in exposing to Python.

  @date 2011-11-09
*/
class MANTID_API_DLL IMDHistoWorkspace : public IMDWorkspace, public MultipleExperimentInfos {
public:
  IMDHistoWorkspace() = default;
  IMDHistoWorkspace &operator=(const IMDHistoWorkspace &) = delete;
  /// Returns a clone of the workspace
  IMDHistoWorkspace_uptr clone() const { return IMDHistoWorkspace_uptr(doClone()); }
  /// Returns a default-initialized clone of the workspace
  IMDHistoWorkspace_uptr cloneEmpty() const { return IMDHistoWorkspace_uptr(doCloneEmpty()); }
  /// See the MDHistoWorkspace definition for descriptions of these
  virtual coord_t getInverseVolume() const = 0;
  virtual const signal_t *getSignalArray() const = 0;
  virtual const signal_t *getErrorSquaredArray() const = 0;
  virtual const signal_t *getNumEventsArray() const = 0;

  virtual signal_t *mutableSignalArray() = 0;
  virtual signal_t *mutableErrorSquaredArray() = 0;
  virtual signal_t *mutableNumEventsArray() = 0;

  virtual void setTo(signal_t signal, signal_t errorSquared, signal_t numEvents) = 0;
  virtual Mantid::Kernel::VMD getCenter(size_t linearIndex) const = 0;
  virtual void setSignalAt(size_t index, signal_t value) = 0;
  virtual void setErrorSquaredAt(size_t index, signal_t value) = 0;
  virtual signal_t getErrorAt(size_t index) const = 0;
  virtual signal_t getErrorAt(size_t index1, size_t index2) const = 0;
  virtual signal_t getErrorAt(size_t index1, size_t index2, size_t index3) const = 0;
  virtual signal_t getErrorAt(size_t index1, size_t index2, size_t index3, size_t index4) const = 0;
  virtual signal_t getSignalAt(size_t index) const = 0;
  virtual signal_t getSignalAt(size_t index1, size_t index2) const = 0;
  virtual signal_t getSignalAt(size_t index1, size_t index2, size_t index3) const = 0;
  virtual signal_t getSignalAt(size_t index1, size_t index2, size_t index3, size_t index4) const = 0;
  virtual signal_t getSignalNormalizedAt(size_t index) const = 0;
  virtual signal_t getSignalNormalizedAt(size_t index1, size_t index2) const = 0;
  virtual signal_t getSignalNormalizedAt(size_t index1, size_t index2, size_t index3) const = 0;
  virtual signal_t getSignalNormalizedAt(size_t index1, size_t index2, size_t index3, size_t index4) const = 0;
  virtual signal_t getErrorNormalizedAt(size_t index) const = 0;
  virtual signal_t getErrorNormalizedAt(size_t index1, size_t index2) const = 0;
  virtual signal_t getErrorNormalizedAt(size_t index1, size_t index2, size_t index3) const = 0;
  virtual signal_t getErrorNormalizedAt(size_t index1, size_t index2, size_t index3, size_t index4) const = 0;

  virtual signal_t &errorSquaredAt(size_t index) = 0;
  virtual signal_t &signalAt(size_t index) = 0;
  virtual size_t getLinearIndex(size_t index1, size_t index2) const = 0;
  virtual size_t getLinearIndex(size_t index1, size_t index2, size_t index3) const = 0;
  virtual size_t getLinearIndex(size_t index1, size_t index2, size_t index3, size_t index4) const = 0;

  virtual LinePlot getLineData(const Mantid::Kernel::VMD &start, const Mantid::Kernel::VMD &end,
                               Mantid::API::MDNormalization normalize) const = 0;

  virtual double &operator[](const size_t &index) = 0;

  virtual void setCoordinateSystem(const Kernel::SpecialCoordinateSystem coordinateSystem) = 0;

  virtual void setDisplayNormalization(const Mantid::API::MDNormalization &preferredNormalization) = 0;

  // Check if this class has an oriented lattice on any sample object
  virtual bool hasOrientedLattice() const override { return MultipleExperimentInfos::hasOrientedLattice(); }

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  IMDHistoWorkspace(const IMDHistoWorkspace &) = default;

  const std::string toString() const override;

private:
  IMDHistoWorkspace *doClone() const override = 0;
  IMDHistoWorkspace *doCloneEmpty() const override = 0;
};

} // namespace API
} // namespace Mantid
