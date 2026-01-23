// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/SplittingInterval.h"

#ifdef _MSC_VER
#pragma warning(disable : 4250) // Disable warning regarding inheritance via dominance
#endif

namespace Mantid {
namespace DataObjects {

/** SplittersWorkspace : A TableWorkspace to contain TimeSplitters.
  It will be used as an input for FilterEvents, which is the ultimate method for
  event filtering.
  There can be various algorithms to generate an object like this.

  A SplittersWorkspace contains 3 columns as int64, int64 and int32 to denote
  (1) splitter start time (2) splitter end time and (3) group workspace index

  @date 2012-04-03
*/
class MANTID_DATAOBJECTS_DLL SplittersWorkspace : public DataObjects::TableWorkspace, public API::ISplittersWorkspace {
public:
  SplittersWorkspace();

  /// Returns a clone of the workspace
  std::unique_ptr<SplittersWorkspace> clone() const { return std::unique_ptr<SplittersWorkspace>(doClone()); }

  /// Returns a default-initialized clone of the workspace
  std::unique_ptr<SplittersWorkspace> cloneEmpty() const { return std::unique_ptr<SplittersWorkspace>(doCloneEmpty()); }

  SplittersWorkspace &operator=(const SplittersWorkspace &other) = delete;
  void addSplitter(const Kernel::SplittingInterval &splitter) override;

  Kernel::SplittingInterval getSplitter(size_t index) override;

  size_t getNumberSplitters() const override;

  bool removeSplitter(size_t) override;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  SplittersWorkspace(const SplittersWorkspace &) = default;

private:
  SplittersWorkspace *doClone() const override { return new SplittersWorkspace(*this); }
  SplittersWorkspace *doCloneEmpty() const override { return new SplittersWorkspace(); }
};

using SplittersWorkspace_sptr = std::shared_ptr<SplittersWorkspace>;
using SplittersWorkspace_const_sptr = std::shared_ptr<const SplittersWorkspace>;

} // namespace DataObjects
} // namespace Mantid

#ifndef DataObjects_EXPORTS
#include "MantidAPI/WorkspaceProperty.h"
namespace Mantid::API {
/// @cond
extern template class MANTID_DATAOBJECTS_DLL WorkspaceProperty<DataObjects::SplittersWorkspace>;
/// @endcond
} // namespace Mantid::API
#endif
