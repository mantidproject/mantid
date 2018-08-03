#include <algorithm>

namespace MantidQt {
namespace CustomInterfaces {

template <size_t NumBanks, typename T>
void RunMap<NumBanks, T>::add(const RunLabel &runLabel, const T &itemToAdd) {
  validateBankID(runLabel.bank);
  m_map[runLabel.bank][runLabel.runNumber] = itemToAdd;
}

template <size_t NumBanks, typename T>
bool RunMap<NumBanks, T>::contains(const RunLabel &runLabel) const {
  return runLabel.bank >= 0 && runLabel.bank < NumBanks &&
         m_map[runLabel.bank].find(runLabel.runNumber) !=
             m_map[runLabel.bank].end();
}

template <size_t NumBanks, typename T>
const T &RunMap<NumBanks, T>::get(const RunLabel &runLabel) const {
  validateBankID(runLabel.bank);
  if (!contains(runLabel)) {
    throw std::invalid_argument("Tried to access invalid run number " +
                                std::to_string(runLabel.runNumber) +
                                " for bank " + std::to_string(runLabel.bank));
  }
  return m_map[runLabel.bank].at(runLabel.runNumber);
}

template <size_t NumBanks, typename T>
void RunMap<NumBanks, T>::remove(const RunLabel &runLabel) {
  validateBankID(runLabel.bank);
  m_map[runLabel.bank].erase(runLabel.runNumber);
}

template <size_t NumBanks, typename T>
std::vector<RunLabel> RunMap<NumBanks, T>::getRunLabels() const {
  std::vector<RunLabel> pairs;

  const auto runNumbers = getAllRunNumbers();
  for (const auto runNumber : runNumbers) {
    for (size_t i = 0; i < NumBanks; ++i) {
      if (m_map[i].find(runNumber) != m_map[i].end()) {
        pairs.emplace_back(runNumber, i);
      }
    }
  }
  return pairs;
}

template <size_t NumBanks, typename T>
std::set<int> RunMap<NumBanks, T>::getAllRunNumbers() const {
  std::set<int> runNumbers;

  for (const auto &bank : m_map) {
    for (const auto &runLabel : bank) {
      runNumbers.insert(runLabel.first);
    }
  }

  return runNumbers;
}

template <size_t NumBanks, typename T>
size_t RunMap<NumBanks, T>::size() const {
  size_t numElements = 0;

  for (const auto &bank : m_map) {
    numElements += bank.size();
  }
  return numElements;
}

template <size_t NumBanks, typename T>
void RunMap<NumBanks, T>::validateBankID(const size_t bank) const {
  if (bank < 0 || bank >= NumBanks) {
    throw std::invalid_argument("Tried to access invalid bank: " +
                                std::to_string(bank));
  }
}

} // CustomInterfaces
} // MantidQt
