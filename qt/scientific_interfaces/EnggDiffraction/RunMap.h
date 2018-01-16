#ifndef MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAP_H_
#define MANTIDQTCUSTOMINTERFACES_ENGGDIFFRACTION_RUNMAP_H_

#include <array>
#include <unordered_map>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

template <size_t NumBanks, typename T> class RunMap {

public:
  void add(const int runNumber, const size_t bank, const T itemToAdd) {
    if (bank < 1 || bank > NumBanks) {
      throw std::invalid_argument("Tried to access invalid bank: " +
                                  std::to_string(bank));
    }
    m_map[bank - 1][runNumber] = itemToAdd;
  }

  bool contains(const int runNumber, const size_t bank) const {
    return bank > 0 && bank <= NumBanks &&
           m_map[bank - 1].find(runNumber) != m_map[bank - 1].end();
  }

  const T &get(const int runNumber, const size_t bank) const {
    if (bank < 1 || bank > NumBanks) {
      throw std::invalid_argument("Tried to access invalid bank: " +
                                  std::to_string(bank));
    }
    if (!contains(runNumber, bank)) {
      throw std::invalid_argument("Tried to access invalid run number " +
                                  std::to_string(runNumber) + " for bank " +
                                  std::to_string(bank));
    }
    return m_map[bank - 1].at(runNumber);
  }

  void remove(const int runNumber, const size_t bank){
    m_map[bank - 1].erase(runNumber);
  }

  std::vector<std::pair<int, size_t>> getRunNumbersAndBankIDs(){
    throw std::runtime_error("Not yet implemented");
  }

private:
  std::array<std::unordered_map<int, T>, NumBanks> m_map;
};

} // CustomInterfaces
} // MantidQt

#endif
