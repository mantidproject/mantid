#include "EnggDiffFittingModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/PropertyWithValue.h"

#include <algorithm>
#include <numeric>

using namespace Mantid;

namespace {

template <typename T> void insertInOrder(const T &item, std::vector<T> &vec) {
  vec.insert(std::upper_bound(vec.begin(), vec.end(), item), item);
}

bool isDigit(const std::string &text) {
  return std::all_of(text.cbegin(), text.cend(), ::isdigit);
}

} // anonymous namespace

namespace MantidQT {
namespace CustomInterfaces {

void EnggDiffFittingModel::addWorkspace(const int runNumber, const size_t bank,
                                        const API::MatrixWorkspace_sptr ws) {
  m_wsMap[bank - 1][runNumber] = ws;
}

API::MatrixWorkspace_sptr
EnggDiffFittingModel::getWorkspace(const int runNumber, const size_t bank) {
  if (bank < 1 || bank > m_wsMap.size()) {
    return nullptr;
  }
  if (m_wsMap[bank - 1].find(runNumber) == m_wsMap[bank - 1].end()) {
    return nullptr;
  }
  return m_wsMap[bank - 1][runNumber];
}

std::vector<int> EnggDiffFittingModel::getAllRunNumbers() const {
  std::vector<int> runNumbers;

  for (const auto &workspaces : m_wsMap) {
    for (const auto &kvPair : workspaces) {
      const auto runNumber = kvPair.first;
      if (std::find(runNumbers.begin(), runNumbers.end(), runNumber) ==
          runNumbers.end()) {
        insertInOrder(runNumber, runNumbers);
      }
    }
  }

  return runNumbers;
}

void EnggDiffFittingModel::loadWorkspaces(const std::string &filename) {
  auto loadAlg = API::AlgorithmManager::Instance().create("Load");
  loadAlg->initialize();

  loadAlg->setPropertyValue("Filename", filename);
  loadAlg->setPropertyValue("OutputWorkspace", FOCUSED_WS_NAME);
  loadAlg->execute();

  API::AnalysisDataServiceImpl &ADS = API::AnalysisDataService::Instance();
  if (filename.find(",") == std::string::npos) { // Only 1 run loaded
    const auto ws = ADS.retrieveWS<API::MatrixWorkspace>(FOCUSED_WS_NAME);
    addWorkspace(ws->getRunNumber(), guessBankID(ws), ws);
  } else {
    const auto group_ws = ADS.retrieveWS<API::WorkspaceGroup>(FOCUSED_WS_NAME);
    for (auto iter = group_ws->begin(); iter != group_ws->end(); ++iter) {
      const auto ws = boost::dynamic_pointer_cast<API::MatrixWorkspace>(*iter);
      addWorkspace(ws->getRunNumber(), guessBankID(ws), ws);
    }
  }
}

std::vector<std::pair<int, size_t>>
EnggDiffFittingModel::getRunNumbersAndBanksIDs() {
  std::vector<std::pair<int, size_t>> pairs;

  const auto runNumbers = getAllRunNumbers();
  for (const auto runNumber : runNumbers) {
    for (size_t i = 0; i < m_wsMap.size(); ++i) {
      if (m_wsMap[i].find(runNumber) != m_wsMap[i].end()) {
        pairs.push_back(std::pair<int, size_t>(runNumber, i + 1));
      }
    }
  }
  return pairs;
}

size_t
EnggDiffFittingModel::guessBankID(API::MatrixWorkspace_const_sptr ws) const {
  if (ws->run().hasProperty("bankid")) {
    const auto log = dynamic_cast<Kernel::PropertyWithValue<int> *>(
        ws->run().getLogData("bankid"));
    return boost::lexical_cast<size_t>(log->value());
  }

  // couldn't get it from sample logs - try using the old naming convention
  auto name = ws->getName();
  std::vector<std::string> chunks;
  boost::split(chunks, name, boost::is_any_of("_"));
  bool isNum = isDigit(chunks.back());
  if (!chunks.empty() && isNum) {
    try {
      return boost::lexical_cast<size_t>(chunks.back());
    } catch (boost::exception &) {
      // If we get a bad cast or something goes wrong then
      // the file is probably not what we were expecting
      // so throw a runtime error
      throw std::runtime_error(
          "Failed to fit file: The data was not what is expected. "
          "Does the file contain a focused workspace?");
    }
  }

  throw std::runtime_error("Could not guess run number from input workspace. "
                           "Are you sure it has been focused correctly?");
}

const std::string EnggDiffFittingModel::FOCUSED_WS_NAME =
    "engggui_fitting_focused_ws";

} // namespace CustomInterfaces
} // namespace MantidQT
