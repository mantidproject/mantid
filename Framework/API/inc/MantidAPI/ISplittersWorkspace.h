// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/TimeSplitter.h"

namespace Mantid {
namespace API {

/** ISplittersWorkspace : Workspace to contain splitters for event filtering.
 * It inherits from ITableWorkspace

  @date 2012-04-03
*/
class MANTID_API_DLL ISplittersWorkspace {
public:
  /*
   * Constructor
   */
  ISplittersWorkspace() = default;
  ISplittersWorkspace &operator=(const ISplittersWorkspace &other) = delete;
  /*
   * Destructor
   */
  virtual ~ISplittersWorkspace() = default;

  /// Returns a clone of the workspace
  std::unique_ptr<ISplittersWorkspace> clone() const { return std::unique_ptr<ISplittersWorkspace>(doClone()); }

  /*
   * Add a time splitter to table workspace
   */
  virtual void addSplitter(Kernel::SplittingInterval splitter) = 0;

  /*
   * Get the corresponding workspace index of a time
   * Input time must the total nanoseconds of the absolute time from 1990.00.00
   */
  virtual Kernel::SplittingInterval getSplitter(size_t index) = 0;

  /*
   * Get number of splitters
   */
  virtual size_t getNumberSplitters() const = 0;

  /*
   * Remove one entry of a splitter
   */
  virtual bool removeSplitter(size_t splitterindex) = 0;

protected:
  /// Protected copy constructor. May be used by childs for cloning.
  ISplittersWorkspace(const ISplittersWorkspace &) = default;

private:
  virtual ISplittersWorkspace *doClone() const = 0;
};

/// Typedef for a shared pointer to \c TableWorkspace
using ISplittersWorkspace_sptr = std::shared_ptr<ISplittersWorkspace>;
/// Typedef for a shared pointer to \c const \c TableWorkspace
using ISplittersWorkspace_const_sptr = std::shared_ptr<const ISplittersWorkspace>;

} // namespace API
} // namespace Mantid
