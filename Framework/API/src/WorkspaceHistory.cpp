// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/HistoryView.h"
#include "MantidKernel/EnvironmentHistory.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/Strings.h"
#include "MantidTypes/Core/DateAndTime.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/functional/hash.hpp>
#if BOOST_VERSION == 106900
#ifndef BOOST_PENDING_INTEGER_LOG2_HPP
#define BOOST_PENDING_INTEGER_LOG2_HPP
#include <boost/integer/integer_log2.hpp>
#endif /* BOOST_PENDING_INTEGER_LOG2_HPP */
#endif /* BOOST_VERSION */
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "Poco/DateTime.h"
#include <Poco/DateTimeParser.h>

using boost::algorithm::split;
using Mantid::Kernel::EnvironmentHistory;

namespace Mantid::API {
namespace {
/// static logger object
Kernel::Logger g_log("WorkspaceHistory");
struct AlgorithmHistorySearch {
  bool operator()(const AlgorithmHistory_sptr &lhs, const AlgorithmHistory_sptr &rhs) { return (*lhs) < (*rhs); }
};
struct AlgorithmHistoryHasher {
  size_t operator()(const AlgorithmHistory_sptr &x) const {
    std::size_t nameAsSeed = std::hash<std::string>{}(x->name());
    boost::hash_combine(nameAsSeed, x->executionDate().totalNanoseconds());
    return nameAsSeed;
  }
};
struct AlgorithmHistoryComparator {
  bool operator()(const AlgorithmHistory_sptr &a, const AlgorithmHistory_sptr &b) const {
    return a->uuid() == b->uuid();
  }
};
} // namespace

/// Default Constructor
WorkspaceHistory::WorkspaceHistory() : m_environment() {}

/// Returns a const reference to the algorithmHistory
const Mantid::API::AlgorithmHistories &WorkspaceHistory::getAlgorithmHistories() const { return m_algorithms; }
/// Returns a const reference to the EnvironmentHistory
const Kernel::EnvironmentHistory &WorkspaceHistory::getEnvironmentHistory() const { return m_environment; }

/// Append the algorithm history from another WorkspaceHistory into this one
void WorkspaceHistory::addHistory(const WorkspaceHistory &otherHistory) {
  // Don't copy one's own history onto oneself
  if (this == &otherHistory) {
    return;
  }

  // Merge the histories
  const AlgorithmHistories &otherAlgorithms = otherHistory.getAlgorithmHistories();

  for (const auto &algHistory : otherAlgorithms) {
    this->addHistory(algHistory);
  }

  using UniqueAlgorithmHistories =
      std::unordered_set<AlgorithmHistory_sptr, AlgorithmHistoryHasher, AlgorithmHistoryComparator>;
  // It is faster to default construct a set/unordered_set and insert the
  // elements than use the range-based constructor directly.
  // See https://stackoverflow.com/a/24477023:
  //   "the constructor actually construct a new node for every element, before
  //   checking its value to determine if it should actually be inserted."
  UniqueAlgorithmHistories uniqueHistories;
  for (const auto &algorithmHistory : m_algorithms) {
    uniqueHistories.insert(algorithmHistory);
  }
  m_algorithms.assign(std::begin(uniqueHistories), std::end(uniqueHistories));
  std::sort(std::begin(m_algorithms), std::end(m_algorithms), AlgorithmHistorySearch());
}

/// Append an AlgorithmHistory to this WorkspaceHistory
void WorkspaceHistory::addHistory(AlgorithmHistory_sptr algHistory) {
  // Assume it is always sorted as algorithm history should only be inserted in
  // the correct order
  m_algorithms.emplace_back(std::move(algHistory));
}

/*
 Return the history length
 */
size_t WorkspaceHistory::size() const { return m_algorithms.size(); }

/**
 * Query if the history is empty or not
 * @returns True if the list is empty, false otherwise
 */
bool WorkspaceHistory::empty() const { return m_algorithms.empty(); }

/**
 * Empty the list of algorithm history objects.
 */
void WorkspaceHistory::clearHistory() { m_algorithms.clear(); }

/**
 * Retrieve an algorithm history by index
 * @param index ::  An index within the workspace history
 * @returns A pointer to an AlgorithmHistory object
 * @throws std::out_of_range error if the index is invalid
 */
AlgorithmHistory_const_sptr WorkspaceHistory::getAlgorithmHistory(const size_t index) const {
  if (index >= this->size()) {
    throw std::out_of_range("WorkspaceHistory::getAlgorithmHistory() - Index out of range");
  }
  return *std::next(m_algorithms.cbegin(), index);
}

/**
 * Index operator[] access to a workspace history
 * @param index ::  An index within the workspace history
 * @returns A pointer to an AlgorithmHistory object
 * @throws std::out_of_range error if the index is invalid
 */
AlgorithmHistory_const_sptr WorkspaceHistory::operator[](const size_t index) const {
  return getAlgorithmHistory(index);
}

/**
 *  Create an algorithm from a history record at a given index
 * @param index ::  An index within the workspace history
 * @returns A shared pointer to an algorithm object
 */
std::shared_ptr<IAlgorithm> WorkspaceHistory::getAlgorithm(const size_t index) const {
  return Algorithm::fromHistory(*(this->getAlgorithmHistory(index)));
}

/**
 * Convenience function for retrieving the last algorithm
 * @returns A shared pointer to the algorithm
 */
std::shared_ptr<IAlgorithm> WorkspaceHistory::lastAlgorithm() const {
  if (m_algorithms.empty()) {
    throw std::out_of_range("WorkspaceHistory::lastAlgorithm() - History contains no algorithms.");
  }
  return this->getAlgorithm(this->size() - 1);
}

/** Prints a text representation of itself
 *  @param os :: The ouput stream to write to
 *  @param indent :: an indentation value to make pretty printing of object and
 * sub-objects
 */
void WorkspaceHistory::printSelf(std::ostream &os, const int indent) const {
  os << std::string(indent, ' ') << m_environment << '\n';
  os << std::string(indent, ' ') << "Histories:\n";
  for (const auto &algorithm : m_algorithms) {
    os << '\n';
    algorithm->printSelf(os, indent + 2);
  }
}

//------------------------------------------------------------------------------------------------
/** Saves all of the workspace history to a "process" field
 * in an open NXS file.
 * Code taken from SaveNexusProcessedHelper.cpp on May 14, 2012.
 *
 * @param file :: previously opened NXS file.
 */
void WorkspaceHistory::saveNexus(::NeXus::File *file) const {
  file->makeGroup("process", "NXprocess", true);
  std::stringstream output;

  // Environment history
  EnvironmentHistory envHist;
  output << envHist;
  char buffer[25];
  time_t now;
  time(&now);
  strftime(buffer, 25, "%Y-%b-%d %H:%M:%S", localtime(&now));
  file->makeGroup("MantidEnvironment", "NXnote", true);
  file->writeData("author", "mantid");
  file->openData("author");
  file->putAttr("date", std::string(buffer));
  file->closeData();
  file->writeData("description", "Mantid Environment data");
  file->writeData("data", output.str());
  file->closeGroup();

  // Algorithm History
  int algCount = 0;
  for (const auto &algorithm : m_algorithms) {
    algorithm->saveNexus(file, algCount);
  }

  // close process group
  file->closeGroup();
}

//-------------------------------------------------------------------------------------------------
/** If the first string contains exactly three words separated by spaces
 *  these words will be copied into each of the following strings that were
 * passed
 *  @param[in] words3 a string with 3 words separated by spaces
 *  @param[out] w1 the first word in the input string
 *  @param[out] w2 the second word in the input string
 *  @param[out] w3 the third word in the input string
 *  @throw out_of_range if there aren't exaltly three strings in the word
 */
void getWordsInString(const std::string &words3, std::string &w1, std::string &w2, std::string &w3) {
  Mantid::Kernel::StringTokenizer data(words3, " ", Mantid::Kernel::StringTokenizer::TOK_TRIM);
  if (data.count() != 3)
    throw std::out_of_range("Algorithm list line " + words3 + " is not of the correct format\n");

  w1 = data[0];
  w2 = data[1];
  w3 = data[2];
}

//-------------------------------------------------------------------------------------------------
/** If the first string contains exactly four words separated by spaces
 *  these words will be copied into each of the following strings that were
 * passed
 *  @param[in] words4 a string with 4 words separated by spaces
 *  @param[out] w1 the first word in the input string
 *  @param[out] w2 the second word in the input string
 *  @param[out] w3 the third word in the input string
 *  @param[out] w4 the fourth word in the input string
 *  @throw out_of_range if there aren't exaltly four strings in the word
 */
void getWordsInString(const std::string &words4, std::string &w1, std::string &w2, std::string &w3, std::string &w4) {
  Mantid::Kernel::StringTokenizer data(words4, " ", Mantid::Kernel::StringTokenizer::TOK_TRIM);
  if (data.count() != 4)
    throw std::out_of_range("Algorithm list line " + words4 + " is not of the correct format\n");

  w1 = data[0];
  w2 = data[1];
  w3 = data[2];
  w4 = data[3];
}

//------------------------------------------------------------------------------------------------
/** Opens a group called "process" and loads the workspace history from
 * it.
 *
 * @param file :: previously opened NXS file.
 */
void WorkspaceHistory::loadNexus(::NeXus::File *file) {
  // Warn but continue if the group does not exist.
  try {
    file->openGroup("process", "NXprocess");
  } catch (std::exception &) {
    g_log.warning() << "Error opening the algorithm history field 'process'. "
                       "Workspace will have no history."
                    << "\n";
    return;
  }

  loadNestedHistory(file);
  file->closeGroup();
}

/** Load every algorithm history object at this point in the hierarchy.
 * This method will recurse over every algorithm entry in the nexus file and
 * load both the record and its children.
 *
 * @param file :: The handle to the nexus file
 * @param parent :: Pointer to the parent AlgorithmHistory object. If null then
 *loaded histories are added to
 * the workspace history.
 */
void WorkspaceHistory::loadNestedHistory(::NeXus::File *file, const AlgorithmHistory_sptr &parent) {
  // historyNumbers should be sorted by number
  std::set<int> historyNumbers = findHistoryEntries(file);
  for (auto historyNumber : historyNumbers) {
    std::string entryName = "MantidAlgorithm_" + Kernel::Strings::toString(historyNumber);
    std::string rawData;
    file->openGroup(entryName, "NXnote");
    file->readData("data", rawData);

    try {
      AlgorithmHistory_sptr history = parseAlgorithmHistory(rawData);
      loadNestedHistory(file, history);
      if (parent) {
        parent->addChildHistory(history);
      } else {
        // if not parent point is supplied, assume we're at the top
        // and attach the history to the workspace
        this->addHistory(history);
      }
    } catch (std::runtime_error &e) {
      // just log the exception as a warning and continue parsing history
      g_log.warning() << e.what() << "\n";
    }

    file->closeGroup();
  }
}

/** Find all the algorithm entries at a particular point the nexus file
 * @param file :: The handle to the nexus file
 * @returns set of integers. One for each algorithm at the level in the file.
 */
std::set<int> WorkspaceHistory::findHistoryEntries(::NeXus::File *file) {
  std::set<int> historyNumbers;
  std::map<std::string, std::string> entries;
  file->getEntries(entries);

  // Histories are numbered MantidAlgorithm_0, ..., MantidAlgorithm_10, etc.
  // Find all the unique numbers
  for (auto &entry : entries) {
    std::string entryName = entry.first;
    if (entryName.find("MantidAlgorithm_") != std::string::npos) {
      // Just get the number
      entryName = entryName.substr(16, entryName.size() - 16);
      int num = -1;
      if (Kernel::Strings::convert(entryName, num))
        historyNumbers.insert(num);
    }
  }

  return historyNumbers;
}

/** Parse an algorithm history entry loaded from file.
 * @param rawData :: The string containing the history entry loaded from file
 * @returns a pointer to the loaded algorithm history object
 * @throws std::runtime_error if the loaded data could not be parsed
 */
AlgorithmHistory_sptr WorkspaceHistory::parseAlgorithmHistory(const std::string &rawData) {
  /// specifies the order that algorithm data is listed in workspaces' histories
  enum AlgorithmHist {
    NAME = 0,      //< algorithms name
    EXEC_TIME = 1, //< when the algorithm was run
    EXEC_DUR = 2,  //< execution time for the algorithm
    UUID = 3,      //< the universal unique id of the algorithm
    PARAMS = 4     //< the algorithm's parameters
  };

  std::vector<std::string> info;
  boost::split(info, rawData, boost::is_any_of("\n"));

  const size_t nlines = info.size();
  if (nlines < 4) { // ignore badly formed history entries still at 4 so that
                    // legacy files can be loaded, 5 is ideal for newer files
    throw std::runtime_error("Malformed history record: Incorrect record size.");
  }

  std::string algName, dummy, temp;
  // get the name and version of the algorithm
  getWordsInString(info[NAME], dummy, algName, temp);

  // Chop of the v from the version string
  size_t numStart = temp.find('v');
  // this doesn't abort if the version string doesn't contain a v
  numStart = numStart != 1 ? 1 : 0;
  temp = std::string(temp.begin() + numStart, temp.end());
  const auto version = boost::lexical_cast<int>(temp);

  // Get the execution date/time
  std::string date, time;
  getWordsInString(info[EXEC_TIME], dummy, dummy, date, time);
  Mantid::Types::Core::DateAndTime utc_start;
  // If not legacy version construct normally else Parse in the legacy data
  if (std::isdigit(static_cast<unsigned char>(date[6]))) {
    Mantid::Types::Core::DateAndTime timeConstruction(date + "T" + time);
    utc_start = timeConstruction;
  } else {
    Poco::DateTime start_timedate;
    // This is needed by the Poco parsing function
    int tzdiff(-1);
    if (!Poco::DateTimeParser::tryParse("%Y-%b-%d %H:%M:%S", date + " " + time, start_timedate, tzdiff)) {
      g_log.warning() << "Error parsing start time in algorithm history entry."
                      << "\n";
      utc_start = Types::Core::DateAndTime::defaultTime();
    }
    utc_start.set_from_time_t(start_timedate.timestamp().epochTime());
  }

  // Get the duration
  getWordsInString(info[EXEC_DUR], dummy, dummy, temp, dummy);
  auto dur = boost::lexical_cast<double>(temp);
  if (dur < -1.0) {
    g_log.warning() << "Error parsing duration in algorithm history entry."
                    << "\n";
    dur = -1.0;
  }

  /// To allow legacy files we must check if it is parameters and set the
  /// variables accordingly. If legacy generate a new UUID for it.
  std::string uuid;
  size_t paramNum;
  if (info[3] != "Parameters:") {
    uuid = info[UUID];
    uuid.erase(uuid.find("UUID: "), 6);
    paramNum = PARAMS;
  } else {
    uuid = boost::uuids::to_string(boost::uuids::random_generator()());
    paramNum = 3;
  }

  // Create the algorithm history
  API::AlgorithmHistory alg_hist(algName, version, uuid, utc_start, dur, Algorithm::g_execCount);
  // Simulate running an algorithm
  ++Algorithm::g_execCount;

  // Add property information
  for (size_t index = static_cast<size_t>(paramNum) + 1; index < nlines; ++index) {
    const std::string line = info[index];
    std::string::size_type colon = line.find(':');
    std::string::size_type comma = line.find(',');
    // Each colon has a space after it
    std::string prop_name = line.substr(colon + 2, comma - colon - 2);
    colon = line.find(':', comma);
    comma = line.find(", Default?", colon);
    std::string prop_value = line.substr(colon + 2, comma - colon - 2);
    colon = line.find(':', comma);
    comma = line.find(", Direction", colon);
    std::string is_def = line.substr(colon + 2, comma - colon - 2);
    colon = line.find(':', comma);
    comma = line.find(',', colon);
    std::string direction = line.substr(colon + 2, comma - colon - 2);
    unsigned int direc(Mantid::Kernel::Direction::asEnum(direction));
    alg_hist.addProperty(prop_name, prop_value, (is_def[0] == 'Y'), direc);
  }

  AlgorithmHistory_sptr history = std::make_shared<AlgorithmHistory>(alg_hist);
  return history;
}

//-------------------------------------------------------------------------------------------------
/** Create a flat view of the workspaces algorithm history
 */
std::shared_ptr<HistoryView> WorkspaceHistory::createView() const { return std::make_shared<HistoryView>(*this); }

//------------------------------------------------------------------------------------------------
/** Prints a text representation
 * @param os :: The ouput stream to write to
 * @param WH :: The WorkspaceHistory to output
 * @returns The ouput stream
 */
std::ostream &operator<<(std::ostream &os, const WorkspaceHistory &WH) {
  WH.printSelf(os);
  return os;
}

bool WorkspaceHistory::operator==(const WorkspaceHistory &otherHistory) const {
  return m_algorithms == otherHistory.m_algorithms;
}

} // namespace Mantid::API
