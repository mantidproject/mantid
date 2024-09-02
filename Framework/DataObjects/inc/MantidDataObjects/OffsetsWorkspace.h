// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace DataObjects {

/** An OffsetsWorkspace is a specialized Workspace2D where
 * the Y value at each pixel is the offset to be used for correcting
 * calculations.
 *
 * @author Janik Zikovsky
 * @date 2011-05-09
 */
class MANTID_DATAOBJECTS_DLL OffsetsWorkspace : public SpecialWorkspace2D {
public:
  OffsetsWorkspace() = default;
  OffsetsWorkspace(const Geometry::Instrument_const_sptr &inst);

  /// Returns a clone of the workspace
  std::unique_ptr<OffsetsWorkspace> clone() const { return std::unique_ptr<OffsetsWorkspace>(doClone()); }
  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<OffsetsWorkspace> cloneEmpty() const { return std::unique_ptr<OffsetsWorkspace>(doCloneEmpty()); }
  OffsetsWorkspace &operator=(const OffsetsWorkspace &) = delete;
  /** Gets the name of the workspace type
  @return Standard string name  */
  const std::string id() const override { return "OffsetsWorkspace"; }

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  OffsetsWorkspace(const OffsetsWorkspace &) = default;

private:
  OffsetsWorkspace *doClone() const override { return new OffsetsWorkspace(*this); }
  OffsetsWorkspace *doCloneEmpty() const override { return new OffsetsWorkspace(); }
};

/// shared pointer to the OffsetsWorkspace class
using OffsetsWorkspace_sptr = std::shared_ptr<OffsetsWorkspace>;

/// shared pointer to a const OffsetsWorkspace
using OffsetsWorkspace_const_sptr = std::shared_ptr<const OffsetsWorkspace>;

} // namespace DataObjects
} // namespace Mantid
