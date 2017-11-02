#include "EnggDiffFittingModel.h"

#include <algorithm>
#include <numeric>

namespace {

template<typename T>
void insertInOrder(const T &item, std::vector<T> &vec) {
	vec.insert(std::upper_bound(vec.begin(), vec.end(), item), item);
}

} // anonymous namespace

namespace MantidQT {
namespace CustomInterfaces {

void EnggDiffFittingModel::addWorkspace(const int runNumber, const int bank,
  const Mantid::API::MatrixWorkspace_sptr ws) {
  m_wsMap[bank - 1][runNumber] = ws;
}

API::MatrixWorkspace_sptr EnggDiffFittingModel::getWorkspace(const int runNumber, const int bank){
	if (bank < 0 || bank >= m_wsMap.size()) {
		return nullptr;
	}
	if (m_wsMap[bank - 1].find(runNumber) == m_wsMap[bank - 1].end()) {
		return nullptr;
	}
	return m_wsMap[bank - 1][runNumber];
}

std::vector<int> EnggDiffFittingModel::getAllRunNumbers(){
	std::vector<int> runNumbers;
		
	for (const auto &workspaces : m_wsMap) {
		for (const auto &kvPair : workspaces) {
			const auto runNumber = kvPair.first;
			if (std::find(runNumbers.begin(), runNumbers.end(), runNumber) == runNumbers.end()) {
				insertInOrder(runNumber, runNumbers);
			}
		}
	}

	return runNumbers;
}

} // namespace CustomInterfaces
} // namespace MantidQT