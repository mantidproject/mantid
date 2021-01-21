// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IMDNode.h"
#include "MantidKernel/ISaveable.h"

namespace Mantid {
namespace DataObjects {

//===============================================================================================
/** Two classes responsible for implementing methods which automatically
save/load MDBox in conjuction with
DiskBuffer
One class responsible for saving events into nexus and another one -- for
identifying the data positions in a file in conjuction with DB

@date March 15, 2013
*/
class DLLExport MDBoxSaveable : public Kernel::ISaveable {
public:
  MDBoxSaveable(API::IMDNode *const);

  /// Save the data to the place, specified by the object
  void save() const override;

  /// Load the data which are not in memory yet and merge them with the data in
  /// memory;
  void load() override;
  /// Method to flush the data to disk and ensure it is written.
  void flushData() const override;
  /// remove objects data from memory but keep all averages
  void clearDataFromMemory() override { m_MDNode->clearDataFromMemory(); }

  /// @return the amount of memory that the object takes up in the MRU.
  uint64_t getTotalDataSize() const override { return m_MDNode->getTotalDataSize(); }
  /**@return the size of the event vector. ! Note that this is NOT necessarily
  the same as the number of points
  (because it might be cached to disk) or the size on disk (because you might
  have called AddEvents) */
  size_t getDataMemorySize() const override { return m_MDNode->getDataInMemorySize(); }

private:
  API::IMDNode *const m_MDNode;
};
} // namespace DataObjects
} // namespace Mantid
