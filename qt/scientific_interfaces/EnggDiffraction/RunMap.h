// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAP_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAP_H_

#include "RunLabel.h"

#include <array>
#include <set>
#include <unordered_map>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

/**
Templated container class holding information relating to runs, indexed by
run number and bank ID.
*/
template <size_t NumBanks, typename T> class RunMap {

public:
  /**
   Add an item to the map
   @param runLabel Run number and bank ID of the item to add
   @param itemToAdd The item to add
   */
  void add(const RunLabel &runLabel, const T &itemToAdd);

  /// Check whether the map contains an entry for a given run number and bank ID
  bool contains(const RunLabel &runLabel) const;

  /// Get the value stored at a given run number and bank ID
  const T &get(const RunLabel &runLabel) const;

  /// Remove an item from the map
  void remove(const RunLabel &runLabel);

  /// Get the associated run number and bank ID of every item stored in the map
  std::vector<RunLabel> getRunLabels() const;

  /// Get the number of items stored in the map
  size_t size() const;

private:
  std::set<std::string> getAllRunNumbers() const;

  void validateBankID(const size_t bank) const;

  std::array<std::unordered_map<std::string, T>, NumBanks> m_map;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#include "RunMap.tpp"

#endif
