#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAP_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAP_H_

#include <array>
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
   @param runNumber The run number associated with the item
   @param bank The bank ID associated with the item
   @param itemToAdd The item to add
   */
  void add(const int runNumber, const size_t bank, const T itemToAdd);

  /// Check whether the map contains an entry for this run number and bank ID
  bool contains(const int runNumber, const size_t bank) const;

  /// Get the value stored at a given run number and bank ID
  const T &get(const int runNumber, const size_t bank) const;

  /// Remove an item from the map
  void remove(const int runNumber, const size_t bank);

  /// Get the associated run number and bank ID of every item stored in the map
  std::vector<std::pair<int, size_t>> getRunNumbersAndBankIDs() const;

  /// Get the number of items stored in the map
  size_t size() const;

private:
  std::vector<int> getAllRunNumbers() const;

  std::array<std::unordered_map<int, T>, NumBanks> m_map;
};

} // CustomInterfaces
} // MantidQt

#include "RunMap.tpp"

#endif
