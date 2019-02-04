// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_IMASKWORKSPACE_H_
#define MANTID_API_IMASKWORKSPACE_H_

#include "MantidGeometry/IDTypes.h"

#include <boost/shared_ptr.hpp>
#include <memory>
#include <set>
#include <string>

namespace Mantid {
namespace API {

/** This class provides an interface to a MaskWorkspace.
 */
class DLLExport IMaskWorkspace {
public:
  IMaskWorkspace() = default;
  IMaskWorkspace &operator=(const IMaskWorkspace &) = delete;
  virtual ~IMaskWorkspace() = default;
  /// Return the workspace typeID
  virtual const std::string id() const { return "IMaskWorkspace"; }
  /// Total number of masked pixels
  virtual std::size_t getNumberMasked() const = 0;
  /// Check if a detector is masked
  virtual bool isMasked(const detid_t detectorID) const = 0;
  /// Check if all detectors in a set are masked
  virtual bool isMasked(const std::set<detid_t> &detectorIDs) const = 0;
  /// Set / remove mask of a detector
  virtual void setMasked(const detid_t detectorID, const bool mask = true) = 0;
  /// Set / remove masks of all detectors in a set
  virtual void setMasked(const std::set<detid_t> &detectorIDs,
                         const bool mask = true) = 0;
  /// Returns a clone of the workspace
  std::unique_ptr<IMaskWorkspace> clone() const {
    return std::unique_ptr<IMaskWorkspace>(doInterfaceClone());
  }

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  IMaskWorkspace(const IMaskWorkspace &other) { (void)other; }

  /// returns a clone of the workspace as the interface
  virtual IMaskWorkspace *doInterfaceClone() const = 0;
};

/// shared pointer to the matrix workspace base class
using IMaskWorkspace_sptr = boost::shared_ptr<IMaskWorkspace>;
/// shared pointer to the matrix workspace base class (const version)
using IMaskWorkspace_const_sptr = boost::shared_ptr<const IMaskWorkspace>;
} // namespace API
} // namespace Mantid

#endif // MANTID_API_IMASKWORKSPACE_H_
