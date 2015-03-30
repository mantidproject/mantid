#include "MantidMDEvents/ImportMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventInserter.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace {
/**
Helper method that takes a string and performs a cast to the specified type.
Throws an invalid_argument exception if the argument cannot be cast to the
target type.
@param str : input string
@return strongly typed value typed to the specfied type argument.
*/
template <typename T> T convert(const std::string &str) {
  std::istringstream iss(str);
  T obj;
  iss >> std::ws >> obj >> std::ws;
  if (!iss.eof()) {
    throw std::invalid_argument("Wrong type destination. Cannot convert " +
                                str);
  }
  return obj;
}
}

namespace Mantid {
namespace MDEvents {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ImportMDEventWorkspace)

/**
Static method returning the flag for the dimensions block.
Allows a single definition for the flag to be used both within and outside of
this class.
@return flag.
*/
const std::string ImportMDEventWorkspace::DimensionBlockFlag() {
  return "DIMENSIONS";
}

/**
Static method returning the flag for the mdevents block.
Allows a single definition for the flag to be used both within and outside of
this class.
@return flag.
*/
const std::string ImportMDEventWorkspace::MDEventBlockFlag() {
  return "MDEVENTS";
}

/**
Static method returning the flag for the comment line.
Allows a single definition for the flag to be used both within and outside of
this class.
@return flag.
*/
const std::string ImportMDEventWorkspace::CommentLineStartFlag() { return "#"; }

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ImportMDEventWorkspace::ImportMDEventWorkspace() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ImportMDEventWorkspace::~ImportMDEventWorkspace() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string ImportMDEventWorkspace::name() const {
  return "ImportMDEventWorkspace";
}

/// Algorithm's version for identification. @see Algorithm::version
int ImportMDEventWorkspace::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ImportMDEventWorkspace::category() const {
  return "MDAlgorithms";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ImportMDEventWorkspace::init() {
  std::vector<std::string> fileExtensions(1);
  fileExtensions[0] = ".txt";
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load,
                                        fileExtensions),
                  "File of type txt");
  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

/**
Extracts mdevent information from the file data and directs the creation of new
MDEvents on the workspace.
@param ws: Workspace to add the events to.
*/
template <typename MDE, size_t nd>
void ImportMDEventWorkspace::addEventsData(
    typename MDEventWorkspace<MDE, nd>::sptr ws) {
  /// Creates a new instance of the MDEventInserter.
  MDEventInserter<typename MDEventWorkspace<MDE, nd>::sptr> inserter(ws);
  DataCollectionType::iterator mdEventEntriesIterator = m_posMDEventStart;
  std::vector<Mantid::coord_t> centers(nd);
  for (size_t i = 0; i < m_nMDEvents; ++i) {
    float signal = convert<float>(*(++mdEventEntriesIterator));
    float error = convert<float>(*(++mdEventEntriesIterator));
    uint16_t run_no = 0;
    int32_t detector_no = 0;
    if (m_IsFullMDEvents) {
      run_no = convert<uint16_t>(*(++mdEventEntriesIterator));
      detector_no = convert<int32_t>(*(++mdEventEntriesIterator));
    }
    for (size_t j = 0; j < m_nDimensions; ++j) {
      centers[j] = convert<Mantid::coord_t>(*(++mdEventEntriesIterator));
    }
    // Actually add the mdevent.
    inserter.insertMDEvent(signal, error * error, run_no, detector_no,
                           centers.data());
  }
}

/**
Iterate through the file data looking for the specified flag and returning TRUE
if found.
@param flag : The flag to look for.
@return TRUE if found.
*/
bool ImportMDEventWorkspace::fileDoesContain(const std::string &flag) {
  return m_file_data.end() !=
         std::find(m_file_data.begin(), m_file_data.end(), flag);
}

/// Check the file contents against the file format.
void ImportMDEventWorkspace::quickFileCheck() {
  // Does it have the mandatory blocks?
  if (!fileDoesContain(DimensionBlockFlag())) {
    std::string message = DimensionBlockFlag() + " missing from file";
    throw std::invalid_argument(message);
  }
  if (!fileDoesContain(MDEventBlockFlag())) {
    std::string message = MDEventBlockFlag() + " missing from file";
    throw std::invalid_argument(message);
  }
  // Are the mandatory block in the correct order.
  DataCollectionType::iterator posDimStart =
      std::find(m_file_data.begin(), m_file_data.end(), DimensionBlockFlag());
  DataCollectionType::iterator posMDEventStart =
      std::find(m_file_data.begin(), m_file_data.end(), MDEventBlockFlag());
  int posDiffDims =
      static_cast<int>(std::distance(posDimStart, posMDEventStart));
  if (posDiffDims < 1) {
    std::string message = DimensionBlockFlag() +
                          " must be specified in file before " +
                          MDEventBlockFlag();
    throw std::invalid_argument(message);
  }
  // Do we have the expected number of dimension entries.
  if ((posDiffDims - 1) % 4 != 0) {
    throw std::invalid_argument(
        "Dimensions in the file should be specified id, name, units, nbins");
  }
  const size_t nDimensions = (posDiffDims - 1) / 4;
  // Are the dimension entries all of the correct type.
  DataCollectionType::iterator dimEntriesIterator = posDimStart;
  for (size_t i = 0; i < nDimensions; ++i) {
    convert<std::string>(*(++dimEntriesIterator));
    convert<std::string>(*(++dimEntriesIterator));
    convert<std::string>(*(++dimEntriesIterator));
    convert<int>(*(++dimEntriesIterator));
  }
  // Do we have the expected number of mdevent entries
  int posDiffMDEvent =
      static_cast<int>(std::distance(posMDEventStart, m_file_data.end()));
  const size_t columnsForFullEvents =
      nDimensions + 4; // signal, error, run_no, detector_no
  const size_t columnsForLeanEvents = nDimensions + 2; // signal, error
  if ((posDiffMDEvent - 1) % columnsForFullEvents != 0) {
    if ((posDiffMDEvent - 1) % columnsForLeanEvents != 0) {
      std::stringstream stream;
      stream << "With the dimenionality found to be " << nDimensions
             << ". Should either have " << columnsForLeanEvents << " or "
             << columnsForFullEvents << " in each row";
      throw std::invalid_argument(stream.str());
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ImportMDEventWorkspace::exec() {
  std::string filename = getProperty("Filename");

  std::ifstream file;
  try {
    file.open(filename.c_str(), std::ios::in);
  } catch (std::ifstream::failure &e) {
    g_log.error() << "Cannot open file: " << filename;
    throw(e);
  }

  // Extract data from the file, excluding comment lines.
  std::string line;
  std::string lastLine;
  size_t nActualColumns = 0;
  while (std::getline(file, line)) {
    boost::algorithm::trim(line);
    if (std::string::npos == line.find_first_of(CommentLineStartFlag())) {
      std::stringstream buffer(line);
      std::copy(std::istream_iterator<std::string>(buffer),
                std::istream_iterator<std::string>(),
                std::back_inserter(m_file_data));

      if (lastLine == MDEventBlockFlag()) {
        std::vector<std::string> strVec;
        boost::algorithm::split(strVec, line, boost::is_any_of("\t "),
                                boost::token_compress_on);
        nActualColumns = strVec.size();
      }
    }
    lastLine = line;
  }

  file.close();

  // Check the file format.
  quickFileCheck();

  // Extract some well used posisions
  m_posDimStart =
      std::find(m_file_data.begin(), m_file_data.end(), DimensionBlockFlag());
  m_posMDEventStart =
      std::find(m_file_data.begin(), m_file_data.end(), MDEventBlockFlag());

  // Calculate the dimensionality
  int posDiffDims =
      static_cast<int>(std::distance(m_posDimStart, m_posMDEventStart));
  m_nDimensions = (posDiffDims - 1) / 4;

  // Calculate the actual number of columns in the MDEvent data.
  int posDiffMDEvent =
      static_cast<int>(std::distance(m_posMDEventStart, m_file_data.end()));
  const size_t columnsForFullEvents =
      m_nDimensions + 4; // signal, error, run_no, detector_no
  m_IsFullMDEvents = (nActualColumns == columnsForFullEvents);

  m_nMDEvents = posDiffMDEvent / nActualColumns;

  // Get the min and max extents in each dimension.
  std::vector<double> extentMins(m_nDimensions);
  std::vector<double> extentMaxs(m_nDimensions);
  DataCollectionType::iterator mdEventEntriesIterator = m_posMDEventStart;
  for (size_t i = 0; i < m_nMDEvents; ++i) {
    mdEventEntriesIterator += 2;
    if (m_IsFullMDEvents) {
      mdEventEntriesIterator += 2;
    }
    for (size_t j = 0; j < m_nDimensions; ++j) {
      double coord = convert<double>(*(++mdEventEntriesIterator));
      extentMins[j] = coord < extentMins[j] ? coord : extentMins[j];
      extentMaxs[j] = coord > extentMaxs[j] ? coord : extentMaxs[j];
    }
  }

  // Create a target output workspace.
  IMDEventWorkspace_sptr outWs = MDEventFactory::CreateMDWorkspace(
      m_nDimensions, m_IsFullMDEvents ? "MDEvent" : "MDLeanEvent");

  // Extract Dimensions and add to the output workspace.
  DataCollectionType::iterator dimEntriesIterator = m_posDimStart;
  for (size_t i = 0; i < m_nDimensions; ++i) {
    std::string id = convert<std::string>(*(++dimEntriesIterator));
    std::string name = convert<std::string>(*(++dimEntriesIterator));
    std::string units = convert<std::string>(*(++dimEntriesIterator));
    int nbins = convert<int>(*(++dimEntriesIterator));

    outWs->addDimension(MDHistoDimension_sptr(new MDHistoDimension(
        id, name, units, static_cast<coord_t>(extentMins[i]),
        static_cast<coord_t>(extentMaxs[i]), nbins)));
  }

  CALL_MDEVENT_FUNCTION(this->addEventsData, outWs)

  // set output
  this->setProperty("OutputWorkspace", outWs);
}

} // namespace Mantid
} // namespace MDEvents
