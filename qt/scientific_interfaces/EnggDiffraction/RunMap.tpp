#include <algorithm>

namespace {

template <typename T> void insertInOrder(const T &item, std::vector<T> &vec) {
  vec.insert(std::upper_bound(vec.begin(), vec.end(), item), item);
}
}

namespace MantidQt {
namespace CustomInterfaces {

template <size_t NumBanks, typename T>
void RunMap<NumBanks, T>::add(const int runNumber, const size_t bank,
                              const T itemToAdd) {
  if (bank < 1 || bank > NumBanks) {
    throw std::invalid_argument("Tried to access invalid bank: " +
                                std::to_string(bank));
  }
  m_map[bank - 1][runNumber] = itemToAdd;
}

template <size_t NumBanks, typename T>
bool RunMap<NumBanks, T>::contains(const int runNumber,
                                   const size_t bank) const {
  return bank > 0 && bank <= NumBanks &&
         m_map[bank - 1].find(runNumber) != m_map[bank - 1].end();
}

template <size_t NumBanks, typename T>
const T &RunMap<NumBanks, T>::get(const int runNumber,
                                  const size_t bank) const {
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

template <size_t NumBanks, typename T>
void RunMap<NumBanks, T>::remove(const int runNumber, const size_t bank) {
  m_map[bank - 1].erase(runNumber);
}

template <size_t NumBanks, typename T>
std::vector<std::pair<int, size_t>>
RunMap<NumBanks, T>::getRunNumbersAndBankIDs() const {
  std::vector<std::pair<int, size_t>> pairs;

  const auto runNumbers = getAllRunNumbers();
  for (const auto runNumber : runNumbers) {
    for (size_t i = 0; i < NumBanks; ++i) {
      if (m_map[i].find(runNumber) != m_map[i].end()) {
        pairs.push_back(std::pair<int, size_t>(runNumber, i + 1));
      }
    }
  }
  return pairs;
}

template <size_t NumBanks, typename T>
std::vector<int> RunMap<NumBanks, T>::getAllRunNumbers() const {
  std::vector<int> runNumbers;

  for (const auto &bank: m_map) {
    for (const auto &runBankPair : bank) {
      const auto runNumber = runBankPair.first;
      if (std::find(runNumbers.begin(), runNumbers.end(), runNumber) ==
          runNumbers.end()) {
        insertInOrder(runNumber, runNumbers);
      }
    }
  }

  return runNumbers;
}

} // CustomInterfaces
} // MantidQt
