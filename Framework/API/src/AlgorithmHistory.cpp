// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/Algorithm.h"

#if BOOST_VERSION == 106900
#ifndef BOOST_PENDING_INTEGER_LOG2_HPP
#define BOOST_PENDING_INTEGER_LOG2_HPP
#include <boost/integer/integer_log2.hpp>
#endif /* BOOST_PENDING_INTEGER_LOG2_HPP */
#endif /* BOOST_VERSION */

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <algorithm>
#include <iterator>
#include <sstream>
#include <utility>

namespace Mantid::API {

using Kernel::Property;
using Kernel::PropertyHistories;
using Kernel::PropertyHistory;
using Kernel::PropertyHistory_const_sptr;
using Kernel::PropertyHistory_sptr;
using Types::Core::DateAndTime;

namespace {
/// The generator for algorithm history UUIDs
static boost::uuids::random_generator uuidGen;
} // namespace

/** Constructor
 *  @param alg ::      A pointer to the algorithm for which the history should
 * be constructed
 *  @param start ::    The start time of the algorithm execution (optional)
 *  @param duration :: The time (in seconds) that it took to run this algorithm
 * (optional)
 *  @param uexeccount :: an  unsigned int for algorithm execution order
 */
AlgorithmHistory::AlgorithmHistory(const Algorithm *const alg, const Types::Core::DateAndTime &start,
                                   const double &duration, std::size_t uexeccount)
    : m_name(alg->name()), m_version(alg->version()), m_executionDate(start), m_executionDuration(duration),
      m_execCount(uexeccount), m_childHistories(), m_storeInADS(alg->getAlwaysStoreInADS()) {
  // Now go through the algorithm's properties and create the PropertyHistory
  // objects.
  setProperties(alg);
  m_uuid = boost::uuids::to_string(uuidGen());
}

/// Default constructor
AlgorithmHistory::AlgorithmHistory() : m_uuid(boost::uuids::to_string(uuidGen())) {}

/**
    Construct AlgorithmHistory by name. Can be used for rstoring the history
   from saved records.
    @param name :: The algorithm name.
    @param vers :: The algorithm version.
    @param uuid :: The universally unique id assigned to this alghistory on
   creation during assignment to history.
    @param start :: The start time of the algorithm execution (optional).
    @param duration :: The time (in seconds) that it took to run this algorithm
   (optional).
   @param uexeccount ::  an  unsigned int for algorithm execution order
 */
AlgorithmHistory::AlgorithmHistory(std::string name, int vers, std::string uuid, const Types::Core::DateAndTime &start,
                                   const double &duration, std::size_t uexeccount)
    : m_name(std::move(name)), m_version(vers), m_executionDate(start), m_executionDuration(duration),
      m_execCount(uexeccount), m_childHistories(), m_uuid(std::move(uuid)) {}

/**
 *  Set the history properties for an algorithm pointer
 *  @param alg :: A pointer to the algorithm for which the history should be
 * constructed
 */
void AlgorithmHistory::setProperties(const Algorithm *const alg) {
  // overwrite any existing properties
  m_properties.clear();
  // Now go through the algorithm's properties and create the PropertyHistory
  // objects.
  const std::vector<Property *> &properties = alg->getProperties();
  std::transform(properties.cbegin(), properties.cend(), std::back_inserter(m_properties),
                 [](const auto &property) { return std::make_shared<PropertyHistory>(property->createHistory()); });
}

/**
 *  Fill the algoirthm history object after it has been created.
 *  @param alg ::      A pointer to the algorithm for which the history should
 * be constructed
 *  @param start ::    The start time of the algorithm execution (optional)
 *  @param duration :: The time (in seconds) that it took to run this algorithm
 * (optional)
 *  @param uexeccount :: an  unsigned int for algorithm execution order
 */
void AlgorithmHistory::fillAlgorithmHistory(const Algorithm *const alg, const Types::Core::DateAndTime &start,
                                            const double &duration, std::size_t uexeccount) {
  m_name = alg->name();
  m_version = alg->version();
  m_executionDate = start;
  m_executionDuration = duration;
  m_execCount = uexeccount;
  m_storeInADS = alg->getAlwaysStoreInADS();
  setProperties(alg);
}

/** Add details of an algorithm's execution to an existing history object
 *  @param start ::    The start time of the algorithm execution
 *  @param duration :: The time (in seconds) that it took to run this algorithm
 */
void AlgorithmHistory::addExecutionInfo(const DateAndTime &start, const double &duration) {
  m_executionDate = start;
  m_executionDuration = duration;
}

/** Add a property to the history.
    @param name :: The name of the property
    @param value :: The value of the property
    @param isdefault :: True if the property is default
    @param direction :: The direction of the property
 */
void AlgorithmHistory::addProperty(const std::string &name, const std::string &value, bool isdefault,
                                   const unsigned int &direction) {
  m_properties.emplace_back(std::make_shared<PropertyHistory>(name, value, "", isdefault, direction));
}

/** Add a child algorithm history to history
 *  @param childHist :: The child history
 */
void AlgorithmHistory::addChildHistory(const AlgorithmHistory_sptr &childHist) {
  // Don't copy one's own history onto oneself
  if (this == &(*childHist)) {
    return;
  }

  m_childHistories.emplace_back(childHist);
}

/*
 Return the child history length
 */
size_t AlgorithmHistory::childHistorySize() const { return m_childHistories.size(); }

/**
 * Retrieve a child algorithm history by index
 * @param index ::  An index within the child algorithm history set
 * @returns A pointer to an AlgorithmHistory object
 * @throws std::out_of_range error if the index is invalid
 */
AlgorithmHistory_sptr AlgorithmHistory::getChildAlgorithmHistory(const size_t index) const {
  if (index >= this->getChildHistories().size()) {
    throw std::out_of_range("AlgorithmHistory::getAlgorithmHistory() - Index out of range");
  }
  return *std::next(m_childHistories.cbegin(), index);
}

/**
 * Index operator[] access to a child algorithm history
 * @param index ::  An index within the algorithm history
 * @returns A pointer to an AlgorithmHistory object
 * @throws std::out_of_range error if the index is invalid
 */
AlgorithmHistory_sptr AlgorithmHistory::operator[](const size_t index) const { return getChildAlgorithmHistory(index); }

/**
 * Gets the value of a specified algorithm property
 * @param name ::  The property to find
 * @returns The string value of the property
 * @throw Exception::NotFoundError if the named property is unknown
 */
const std::string &AlgorithmHistory::getPropertyValue(const std::string &name) const {
  const auto found = std::find_if(m_properties.cbegin(), m_properties.cend(),
                                  [&name](const auto &history) { return history->name() == name; });
  if (found == m_properties.cend()) {
    throw Kernel::Exception::NotFoundError("Could not find the specified property", name);
  }
  return (*found)->value();
}

/**
 *  Create an algorithm from a history record at a given index
 * @param index :: An index within the workspace history
 * @returns A shared pointer to an algorithm object
 */
std::shared_ptr<IAlgorithm> AlgorithmHistory::getChildAlgorithm(const size_t index) const {
  return Algorithm::fromHistory(*(this->getChildAlgorithmHistory(index)));
}

/** Prints a text representation of itself
 *  @param os :: The output stream to write to
 *  @param indent :: an indentation value to make pretty printing of object and
 * sub-objects
 *  @param maxPropertyLength :: the max length for any property value string (0
 * = full length)
 */
void AlgorithmHistory::printSelf(std::ostream &os, const int indent, const size_t maxPropertyLength) const {
  auto execDate = m_executionDate.toISO8601String();
  execDate.replace(execDate.find("T"), 1, " ");
  os << std::string(indent, ' ') << "Algorithm: " << m_name;
  os << std::string(indent, ' ') << " v" << m_version << '\n';

  os << std::string(indent, ' ') << "Execution Date: " << execDate << '\n';
  os << std::string(indent, ' ') << "Execution Duration: " << m_executionDuration << " seconds\n";
  os << std::string(indent, ' ') << "UUID: " << m_uuid << '\n';
  os << std::string(indent, ' ') << "Parameters:\n";

  for (const auto &property : m_properties) {
    property->printSelf(os, indent + 2, maxPropertyLength);
  }
}

/**
 * Create a concrete algorithm based on a history record
 * @returns An algorithm object constructed from this history record
 */
std::shared_ptr<IAlgorithm> AlgorithmHistory::createAlgorithm() const { return Algorithm::fromHistory(*this); }

/**
    Standard Assignment operator
    @param A :: AlgorithmHistory Item to assign to 'this'
 */
AlgorithmHistory &AlgorithmHistory::operator=(const AlgorithmHistory &A) {
  if (this != &A) {
    m_name = A.m_name;
    m_version = A.m_version;
    m_executionDate = A.m_executionDate;
    m_executionDuration = A.m_executionDuration;
    m_properties = A.m_properties;
    m_storeInADS = A.m_storeInADS;
    // required to prevent destruction of descendant if assigning a descendant
    // to an ancestor
    auto temp = A.m_childHistories;
    m_childHistories = temp;
    m_uuid = A.m_uuid;
    m_execCount = A.m_execCount;
  }
  return *this;
}

/** Prints a text representation
 * @param os :: The ouput stream to write to
 * @param AH :: The AlgorithmHistory to output
 * @returns The ouput stream
 */
std::ostream &operator<<(std::ostream &os, const AlgorithmHistory &AH) {
  AH.printSelf(os);
  return os;
}

/** Write out this history record to file.
 * @param file :: The handle to the nexus file to save to
 * @param algCount :: Counter of the number of algorithms written to file.
 */
void AlgorithmHistory::saveNexus(::NeXus::File *file, int &algCount) const {
  std::stringstream algNumber;
  ++algCount;
  algNumber << "MantidAlgorithm_" << algCount; // history entry names start at 1 not 0

  std::stringstream algData;
  printSelf(algData);

  file->makeGroup(algNumber.str(), "NXnote", true);
  file->writeData("author", std::string("mantid"));
  file->writeData("description", std::string("Mantid Algorithm data"));
  file->writeData("data", algData.str());

  // child algorithms
  for (const auto &history : m_childHistories) {
    history->saveNexus(file, algCount);
  }
  file->closeGroup();
}
} // namespace Mantid::API
