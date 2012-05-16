//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/EnvironmentHistory.h"
#include <boost/algorithm/string/split.hpp>
#include "Poco/DateTime.h"
#include <Poco/DateTimeParser.h>

using Mantid::Kernel::EnvironmentHistory;
using boost::algorithm::split;

namespace Mantid
{
namespace API
{

///Default Constructor
WorkspaceHistory::WorkspaceHistory() : m_environment(), m_algorithms(),
    g_log(Kernel::Logger::get("WorkspaceHistory"))
{}

/// Destructor
WorkspaceHistory::~WorkspaceHistory()
{}

/**
  Standard Copy Constructor
  @param A :: WorkspaceHistory Item to copy
 */
WorkspaceHistory::WorkspaceHistory(const WorkspaceHistory& A) :
  m_environment(A.m_environment), m_algorithms(A.m_algorithms),
  g_log(Kernel::Logger::get("WorkspaceHistory"))
{}

/// Returns a const reference to the algorithmHistory
const WorkspaceHistory::AlgorithmHistories & WorkspaceHistory::getAlgorithmHistories() const
{
  return m_algorithms;
}
/// Returns a const reference to the EnvironmentHistory
const Kernel::EnvironmentHistory& WorkspaceHistory::getEnvironmentHistory() const
{
  return m_environment;
}

/// Append the algorithm history from another WorkspaceHistory into this one
void WorkspaceHistory::addHistory(const WorkspaceHistory& otherHistory)
{
  // Don't copy one's own history onto oneself
  if (this == &otherHistory)
  {
    return;
  }

  // Merge the histories
  const AlgorithmHistories & otherAlgorithms = otherHistory.getAlgorithmHistories();
  m_algorithms.insert(otherAlgorithms.begin(), otherAlgorithms.end());

}

/// Append an AlgorithmHistory to this WorkspaceHistory
void WorkspaceHistory::addHistory(const AlgorithmHistory& algHistory)
{
  m_algorithms.insert(algHistory);
}

/*
 Return the history length
 */
size_t WorkspaceHistory::size() const
{
  return m_algorithms.size();
}

/**
 * Query if the history is empty or not
 * @returns True if the list is empty, false otherwise
 */
bool WorkspaceHistory::empty() const
{
  return m_algorithms.empty();
}

/**
 * Retrieve an algorithm history by index
 * @param index ::  An index within the workspace history
 * @returns A reference to a const AlgorithmHistory object
 * @throws std::out_of_range error if the index is invalid
 */
const AlgorithmHistory & WorkspaceHistory::getAlgorithmHistory(const size_t index) const
{
  if( index >= this->size() )
  {
    throw std::out_of_range("WorkspaceHistory::getAlgorithmHistory() - Index out of range");
  }
  AlgorithmHistories::const_iterator start = m_algorithms.begin();
  std::advance(start, index);
  return *start;
}

/**
 * Index operator[] access to a workspace history
 * @param index ::  An index within the workspace history
 * @returns A reference to a const AlgorithmHistory object
 * @throws std::out_of_range error if the index is invalid
 */
const AlgorithmHistory & WorkspaceHistory::operator[](const size_t index) const
{
  return getAlgorithmHistory(index);
}

/**
 *  Create an algorithm from a history record at a given index
 * @param index ::  An index within the workspace history
 * @returns A shared pointer to an algorithm object
 */
boost::shared_ptr<IAlgorithm> WorkspaceHistory::getAlgorithm(const size_t index) const
{
  return Algorithm::fromHistory(this->getAlgorithmHistory(index));
}

/**
 * Convenience function for retrieving the last algorithm
 * @returns A shared pointer to the algorithm
 */
boost::shared_ptr<IAlgorithm> WorkspaceHistory::lastAlgorithm() const
{
  if( m_algorithms.empty() )
  {
    throw std::out_of_range("WorkspaceHistory::lastAlgorithm() - History contains no algorithms.");
  }
  return this->getAlgorithm(this->size() - 1);
}

/** Prints a text representation of itself
 *  @param os :: The ouput stream to write to
 *  @param indent :: an indentation value to make pretty printing of object and sub-objects
 */
void WorkspaceHistory::printSelf(std::ostream& os, const int indent)const
{

  os << std::string(indent,' ')  << m_environment << std::endl;

  AlgorithmHistories::const_iterator it;
  os << std::string(indent,' ') << "Histories:" <<std::endl;

  for (it=m_algorithms.begin();it!=m_algorithms.end();++it)
  {
    os << std::endl;
    it->printSelf( os, indent+2 );
  }
}

//------------------------------------------------------------------------------------------------
/** Saves all of the workspace history to a "process" field
 * in an open NXS file.
 * Code taken from NexusFileIO.cpp on May 14, 2012.
 *
 * @param file :: previously opened NXS file.
 */
void WorkspaceHistory::saveNexus(::NeXus::File * file) const
{
  file->makeGroup("process", "NXprocess", true);
  std::stringstream output,algorithmNumber;

  // Environment history
  EnvironmentHistory envHist;
  output << envHist;
  char buffer [25];
  time_t now;
  time(&now);
  strftime (buffer,25,"%Y-%b-%d %H:%M:%S",localtime(&now));
  file->makeGroup("MantidEnvironment", "NXNote", true);
  file->writeData("author", "mantid");
  file->openData("author");
  file->putAttr("date", std::string(buffer));
  file->closeData();
  file->writeData("description", "Mantid Environment data");
  file->writeData("data", output.str());
  file->closeGroup();

  // Algorithm History
  typedef std::map <std::size_t,std::string> orderedHistMap;
  orderedHistMap ordMap;
  for(std::size_t i=0;i<this->size();i++)
  {
    std::stringstream algNumber,algData;
    const API::AlgorithmHistory & entry = this->getAlgorithmHistory(i);
    entry.printSelf(algData);

    //get execute count
    std::size_t nexecCount=entry.execCount();
    //order by execute count
    ordMap.insert(orderedHistMap::value_type(nexecCount,algData.str()));
  }
  int num=0;
  std::map <std::size_t,std::string>::iterator m_Iter;
  for (m_Iter=ordMap.begin( );m_Iter!=ordMap.end( );++m_Iter)
  {
    ++num;
    std::stringstream algNumber;
    algNumber << "MantidAlgorithm_" << num;

    file->makeGroup(algNumber.str(), "NXNote", true);
    file->writeData("author", std::string("mantid"));
    file->writeData("description", std::string("Mantid Algorithm data"));
    file->writeData("data", m_Iter->second);
    file->closeGroup();
  }
  file->closeGroup();
}

//-------------------------------------------------------------------------------------------------
/** If the first string contains exactly three words separated by spaces
 *  these words will be copied into each of the following strings that were passed
 *  @param[in] words3 a string with 3 words separated by spaces
 *  @param[out] w1 the first word in the input string
 *  @param[out] w2 the second word in the input string
 *  @param[out] w3 the third word in the input string
 *  @throw out_of_range if there aren't exaltly three strings in the word
 */
void getWordsInString(const std::string & words3, std::string & w1, std::string & w2, std::string & w3 )
{
  Poco::StringTokenizer data(words3, " ", Poco::StringTokenizer::TOK_TRIM);
  if (data.count() != 3)
    throw std::out_of_range("Algorithm list line " + words3 + " is not of the correct format\n");

  w1 = data[0];
  w2 = data[1];
  w3 = data[2];
}



//-------------------------------------------------------------------------------------------------
/** If the first string contains exactly four words separated by spaces
 *  these words will be copied into each of the following strings that were passed
 *  @param[in] words4 a string with 4 words separated by spaces
 *  @param[out] w1 the first word in the input string
 *  @param[out] w2 the second word in the input string
 *  @param[out] w3 the third word in the input string
 *  @param[out] w4 the fourth word in the input string
 *  @throw out_of_range if there aren't exaltly four strings in the word
 */
void getWordsInString(const std::string & words4, std::string & w1, std::string & w2, std::string & w3, std::string & w4)
{
  Poco::StringTokenizer data(words4, " ", Poco::StringTokenizer::TOK_TRIM);
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
void WorkspaceHistory::loadNexus(::NeXus::File * file)
{
  /// specifies the order that algorithm data is listed in workspaces' histories
  enum AlgorithmHist
  {
    NAME = 0,                          //< algorithms name
    EXEC_TIME = 1,                     //< when the algorithm was run
    EXEC_DUR = 2,                      //< execution time for the algorithm
    PARAMS = 3                         //< the algorithm's parameters
  };


  file->openGroup("process", "NXprocess");
  std::map<std::string, std::string> entries;
  file->getEntries(entries);
  // Entries should be sorted by name
  for (auto it = entries.begin(); it != entries.end(); ++it)
  {
    std::string entryName = it->first;
    if( entryName.find("MantidAlgorithm") != std::string::npos )
    {
      file->openGroup(entryName, "NXprocess");
      std::string rawData;
      file->readData("data", rawData);
      file->closeData();
      file->closeGroup();

      // Split into separate lines
      std::vector<std::string> info;
      boost::split(info, rawData, boost::is_any_of("\n"));

      const size_t nlines = info.size();
      if( nlines < 4 )
      {// ignore badly formed history entries
        continue;
      }

      std::string algName, dummy, temp;
      // get the name and version of the algorithm
      getWordsInString(info[NAME], dummy, algName, temp);

      //Chop of the v from the version string
      size_t numStart = temp.find('v');
      // this doesn't abort if the version string doesn't contain a v
      numStart = numStart != 1 ? 1 : 0;
      temp = std::string(temp.begin() + numStart, temp.end());
      const int version = boost::lexical_cast<int>(temp);

      //Get the execution date/time
      std::string date, time;
      getWordsInString(info[EXEC_TIME], dummy, dummy, date, time);
      Poco::DateTime start_timedate;
      //This is needed by the Poco parsing function
      int tzdiff(-1);
      if( !Poco::DateTimeParser::tryParse("%Y-%b-%d %H:%M:%S", date + " " + time, start_timedate, tzdiff))
      {
        g_log.warning() << "Error parsing start time in algorithm history entry." << "\n";
        return;
      }
      //Get the duration
      getWordsInString(info[EXEC_DUR], dummy, dummy, temp, dummy);
      double dur = boost::lexical_cast<double>(temp);
      if ( dur < 0.0 )
      {
        g_log.warning() << "Error parsing start time in algorithm history entry." << "\n";
        return;
      }
      //Convert the timestamp to time_t to DateAndTime
      Mantid::Kernel::DateAndTime utc_start;
      utc_start.set_from_time_t( start_timedate.timestamp().epochTime() );
      //Create the algorithm history
      API::AlgorithmHistory alg_hist(algName, version, utc_start, dur,Algorithm::g_execCount);
      // Simulate running an algorithm
      ++Algorithm::g_execCount;

      //Add property information
      for( size_t index = static_cast<size_t>(PARAMS)+1;index < nlines;++index )
      {
        const std::string line = info[index];
        std::string::size_type colon = line.find(":");
        std::string::size_type comma = line.find(",");
        //Each colon has a space after it
        std::string prop_name = line.substr(colon + 2, comma - colon - 2);
        colon = line.find(":", comma);
        comma = line.find(", Default?", colon);
        std::string prop_value = line.substr(colon + 2, comma - colon - 2);
        colon = line.find(":", comma);
        comma = line.find(", Direction", colon);
        std::string is_def = line.substr(colon + 2, comma - colon - 2);
        colon = line.find(":", comma);
        comma = line.find(",", colon);
        std::string direction = line.substr(colon + 2, comma - colon - 2);
        unsigned int direc(Mantid::Kernel::Direction::asEnum(direction));
        alg_hist.addProperty(prop_name, prop_value, (is_def[0] == 'Y'), direc);
      }
      this->addHistory(alg_hist);
    }
  }
}


//------------------------------------------------------------------------------------------------
/** Prints a text representation
 * @param os :: The ouput stream to write to
 * @param WH :: The WorkspaceHistory to output
 * @returns The ouput stream
 */
std::ostream& operator<<(std::ostream& os, const WorkspaceHistory& WH)
{
  WH.printSelf(os);
  return os;
}

} // namespace API
} // namespace Mantid
