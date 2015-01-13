//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/Algorithm.h"
#include <sstream>

namespace Mantid {
namespace API {

using Kernel::Property;
using Kernel::DateAndTime;
using Kernel::PropertyHistory;
using Kernel::PropertyHistory_sptr;
using Kernel::PropertyHistory_const_sptr;
using Kernel::PropertyHistories;

/** Constructor
 *  @param alg ::      A pointer to the algorithm for which the history should
 * be constructed
 *  @param start ::    The start time of the algorithm execution (optional)
 *  @param duration :: The time (in seconds) that it took to run this algorithm
 * (optional)
 *  @param uexeccount :: an  unsigned int for algorithm execution order
 */
AlgorithmHistory::AlgorithmHistory(const Algorithm *const alg,
                                   const Kernel::DateAndTime &start,
                                   const double &duration,
                                   std::size_t uexeccount)
    : m_name(alg->name()), m_version(alg->version()), m_executionDate(start),
      m_executionDuration(duration), m_execCount(uexeccount),
      m_childHistories(boost::bind(CompareHistory::compare, _1, _2)) {
  // Now go through the algorithm's properties and create the PropertyHistory
  // objects.
  setProperties(alg);
}

/** Private empty constructor for use by Algorithm
 */
AlgorithmHistory::AlgorithmHistory()
    : m_name(), m_version(), m_executionDate(), m_executionDuration(),
      m_execCount(),
      m_childHistories(boost::bind(CompareHistory::compare, _1, _2)) {}

/// Destructor
AlgorithmHistory::~AlgorithmHistory() {}

/**
    Construct AlgorithmHistory by name. Can be used for rstoring the history
   from saved records.
    @param name :: The algorithm name.
    @param vers :: The algorithm version.
    @param start :: The start time of the algorithm execution (optional).
    @param duration :: The time (in seconds) that it took to run this algorithm
   (optional).
   @param uexeccount ::  an  unsigned int for algorithm execution order
 */
AlgorithmHistory::AlgorithmHistory(const std::string &name, int vers,
                                   const Kernel::DateAndTime &start,
                                   const double &duration,
                                   std::size_t uexeccount)
    : m_name(name), m_version(vers), m_executionDate(start),
      m_executionDuration(duration), m_execCount(uexeccount),
      m_childHistories(boost::bind(CompareHistory::compare, _1, _2)) {}

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
  std::vector<Property *>::const_iterator it;
  for (it = properties.begin(); it != properties.end(); ++it) {
    m_properties.push_back(
        boost::make_shared<PropertyHistory>((*it)->createHistory()));
  }
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
void AlgorithmHistory::fillAlgorithmHistory(const Algorithm *const alg,
                                            const Kernel::DateAndTime &start,
                                            const double &duration,
                                            std::size_t uexeccount) {
  m_name = alg->name();
  m_version = alg->version();
  m_executionDate = start;
  m_executionDuration = duration;
  m_execCount = uexeccount;
  setProperties(alg);
}

/**
    Standard Copy Constructor
    @param A :: AlgorithmHistory Item to copy
 */
AlgorithmHistory::AlgorithmHistory(const AlgorithmHistory &A)
    : m_name(A.m_name), m_version(A.m_version),
      m_executionDate(A.m_executionDate),
      m_executionDuration(A.m_executionDuration), m_properties(A.m_properties),
      m_execCount(A.m_execCount),
      m_childHistories(boost::bind(CompareHistory::compare, _1, _2)) {
  m_childHistories = A.m_childHistories;
}

/** Add details of an algorithm's execution to an existing history object
 *  @param start ::    The start time of the algorithm execution
 *  @param duration :: The time (in seconds) that it took to run this algorithm
 */
void AlgorithmHistory::addExecutionInfo(const DateAndTime &start,
                                        const double &duration) {
  m_executionDate = start;
  m_executionDuration = duration;
}

/** Add a property to the history.
    @param name :: The name of the property
    @param value :: The value of the property
    @param isdefault :: True if the property is default
    @param direction :: The direction of the property
 */
void AlgorithmHistory::addProperty(const std::string &name,
                                   const std::string &value, bool isdefault,
                                   const unsigned int &direction) {
  Kernel::PropertyHistory propHist(name, value, "", isdefault, direction);
  m_properties.push_back(boost::make_shared<PropertyHistory>(propHist));
}

/** Add a child algorithm history to history
 *  @param childHist :: The child history
 */
void AlgorithmHistory::addChildHistory(AlgorithmHistory_sptr childHist) {
  // Don't copy one's own history onto oneself
  if (this == &(*childHist)) {
    return;
  }

  m_childHistories.insert(childHist);
}

/*
 Return the child history length
 */
size_t AlgorithmHistory::childHistorySize() const {
  return m_childHistories.size();
}

/**
 * Retrieve a child algorithm history by index
 * @param index ::  An index within the child algorithm history set
 * @returns A pointer to an AlgorithmHistory object
 * @throws std::out_of_range error if the index is invalid
 */
AlgorithmHistory_sptr
AlgorithmHistory::getChildAlgorithmHistory(const size_t index) const {
  if (index >= this->getChildHistories().size()) {
    throw std::out_of_range(
        "AlgorithmHistory::getAlgorithmHistory() - Index out of range");
  }
  AlgorithmHistories::const_iterator start = m_childHistories.begin();
  std::advance(start, index);
  return *start;
}

/**
 * Index operator[] access to a child algorithm history
 * @param index ::  An index within the algorithm history
 * @returns A pointer to an AlgorithmHistory object
 * @throws std::out_of_range error if the index is invalid
 */
AlgorithmHistory_sptr AlgorithmHistory::operator[](const size_t index) const {
  return getChildAlgorithmHistory(index);
}

/**
 *  Create an algorithm from a history record at a given index
 * @param index :: An index within the workspace history
 * @returns A shared pointer to an algorithm object
 */
boost::shared_ptr<IAlgorithm>
AlgorithmHistory::getChildAlgorithm(const size_t index) const {
  return Algorithm::fromHistory(*(this->getChildAlgorithmHistory(index)));
}

/** Prints a text representation of itself
 *  @param os :: The ouput stream to write to
 *  @param indent :: an indentation value to make pretty printing of object and
 * sub-objects
 */
void AlgorithmHistory::printSelf(std::ostream &os, const int indent) const {
  os << std::string(indent, ' ') << "Algorithm: " << m_name;
  os << std::string(indent, ' ') << " v" << m_version << std::endl;

  os << std::string(indent, ' ')
     << "Execution Date: " << m_executionDate.toFormattedString() << std::endl;
  os << std::string(indent, ' ')
     << "Execution Duration: " << m_executionDuration << " seconds"
     << std::endl;

  os << std::string(indent, ' ') << "Parameters:" << std::endl;

  PropertyHistories::const_iterator it;
  for (it = m_properties.begin(); it != m_properties.end(); ++it) {
    (*it)->printSelf(os, indent + 2);
  }
}

/**
 * Create a concrete algorithm based on a history record
 * @returns An algorithm object constructed from this history record
 */
boost::shared_ptr<IAlgorithm> AlgorithmHistory::createAlgorithm() const {
  return Algorithm::fromHistory(*this);
}

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
    // required to prevent destruction of descendant if assigning a descendant
    // to an ancestor
    auto temp = A.m_childHistories;
    m_childHistories = temp;
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
  algNumber << "MantidAlgorithm_"
            << algCount; // history entry names start at 1 not 0

  std::stringstream algData;
  printSelf(algData);

  file->makeGroup(algNumber.str(), "NXnote", true);
  file->writeData("author", std::string("mantid"));
  file->writeData("description", std::string("Mantid Algorithm data"));
  file->writeData("data", algData.str());

  // child algorithms
  AlgorithmHistories::const_iterator histIter = m_childHistories.begin();
  for (; histIter != m_childHistories.end(); ++histIter) {
    (*histIter)->saveNexus(file, algCount);
  }

  file->closeGroup();
}

} // namespace API
} // namespace Mantid
