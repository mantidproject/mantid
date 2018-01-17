#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAP_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAP_H_

#include <array>
#include <unordered_map>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

template <size_t NumBanks, typename T> class RunMap {

public:
  void add(const int runNumber, const size_t bank, const T itemToAdd);

  bool contains(const int runNumber, const size_t bank) const;

  const T &get(const int runNumber, const size_t bank) const;

  void remove(const int runNumber, const size_t bank);

  std::vector<std::pair<int, size_t>> getRunNumbersAndBankIDs() const;

private:
  std::vector<int> getAllRunNumbers() const;

  std::array<std::unordered_map<int, T>, NumBanks> m_map;
};

} // CustomInterfaces
} // MantidQt

#include "RunMap.tpp"

#endif
