// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/Column.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid {

namespace DataObjects {

/** @class MementoTableWorkspace

  Variation on the TableWorkspace with a set of pre-defined columns used to
  store diffs on Workspaces.

  @author Owen Arnold
  @date 31/08/2011
*/
class DLLExport MementoTableWorkspace : public TableWorkspace {
public:
  MementoTableWorkspace &operator=(const MementoTableWorkspace &) = delete;
  static bool isMementoWorkspace(const Mantid::API::ITableWorkspace &candidate);
  MementoTableWorkspace(int nRows = 0);

  /// Returns a clone of the workspace
  std::unique_ptr<MementoTableWorkspace> clone() const { return std::unique_ptr<MementoTableWorkspace>(doClone()); }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<MementoTableWorkspace> cloneEmpty() const {
    return std::unique_ptr<MementoTableWorkspace>(doCloneEmpty());
  }

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  MementoTableWorkspace(const MementoTableWorkspace &) = default;

private:
  MementoTableWorkspace *doClone() const override { return new MementoTableWorkspace(*this); }

  MementoTableWorkspace *doCloneEmpty() const override { return new MementoTableWorkspace(); }

  static bool expectedColumn(const Mantid::API::Column_const_sptr &expected,
                             const Mantid::API::Column_const_sptr &candidate);
};
} // namespace DataObjects
} // namespace Mantid
