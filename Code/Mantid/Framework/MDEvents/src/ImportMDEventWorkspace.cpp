/*WIKI*



Creates an MDEventWorkspace from a plain ASCII file. Uses a simple format for the file described below. This algorithm is suitable for importing small volumes of data only. This algorithm does not scale well for large input workspaces. The purpose of this algorithm is to allow users to quickly import data from existing applications for purposes of comparison.

== Format ==

The file must contain a '''DIMENSIONS''' section listing the dimensionality of the workspace. Each input line is taken as a new dimension after the '''DIMENSIONS''' flag, and before the '''MDEVENTS''' flag. Input arguments must be separated by a space or tab. They are provided in the order Name, ID, Units, NumberOfBins. See the Usage examples below.

The file must contain a '''MDEVENTS''' flag after the '''DIMENSIONS''' flag. Again, inputs are separated by a space or tab, each line represents a unique MDEvent. There must be either NDims + 2 or NDims + 4 columns in this section. If there are NDims + 2, columns, then ''Lean'' MDEvents will be used (Signal, Error and Dimensionality only). If there are NDims + 4 columns, then ''Full'' MDEvents will be used (Signal, Error, RunNo, DetectorId and Dimensionality). If you have provided NDims + 2 columns, then each row is interpreted as follows:

 Signal Error {Dimensionality}

where the Dimensionality is an array of coordinates in each of the dimensions listed in the '''DIMENSIONS''' section, and in that order. IF there are NDims + 4 columns, then ''Full'' MDEvents will be used. Each row is interpreted as follows:

 Signal Error RunNumber DetectorId {Dimensionality}

The usage example below shows demo files with both of these formats.

Comments are denoted by lines starting with '''#'''. There is no multi-line comment. 

== Usage ==

The following example creates a 2D MDEventWorkspace called ''demo'' with 10 * 2 bins.


 ImportMDEventWorkspace(Filename=r'demo.txt',OutputWorkspace='demo')

demo.txt looks like:

    # MANDATORY BLOCK. Dimensions are written in the format Id, Name, Units, number of bins
    DIMENSIONS
    a A U 10
    b B U 2
    # MANDATORY BLOCK. Events are written in the format Signal, Error, DetectorId, RunId, coord1, coord2, ... to end of coords
    # or  Signal, Error, coord1, coord2, ... to end of coords
    MDEVENTS
    1.0	2.90	-1.0	-1
    1.1	2.80	-0.9	-1
    1.2	2.70	-0.8	-1
    1.3	2.60	-0.7	-1
    1.4	2.50	-0.6	-1
    1.5	2.40	-0.5	-1
    1.6	2.30	-0.4	-1
    1.7	2.20	-0.3	-1
    1.8	2.10	-0.2	-1
    1.9	2.00	-0.1	-1
    2.0	1.80	-1.0	1
    2.1	1.70	-0.9	1
    2.2	1.60	-0.8	1
    2.3	1.50	-0.7	1
    2.4	1.40	-0.6	1
    2.5	1.30	-0.5	1
    2.6	1.20	-0.4	1
    2.7	1.10	-0.3	1
    2.8	1.00	-0.2	1
    2.9	0.90	-0.1	1

The equivalent with run numbers and detector ids specified is:


    # MANDATORY BLOCK. Dimensions are written in the format Id, Name, Units, number of bins
    DIMENSIONS
    a A U 10
    b B U 2
    # MANDATORY BLOCK. Events are written in the format Signal, Error, DetectorId, RunId, coord1, coord2, ... to end of coords
    # or  Signal, Error, RunNumber, DetectorId, coord1, coord2, ... to end of coords
    MDEVENTS
    1.0	2.90	1	1	-1.0	-1
    1.1	2.80	1	2	-0.9	-1
    1.2	2.70	1	3	-0.8	-1
    1.3	2.60	1	4	-0.7	-1
    1.4	2.50	1	5	-0.6	-1
    1.5	2.40	1	6	-0.5	-1
    1.6	2.30	1	7	-0.4	-1
    1.7	2.20	1	8	-0.3	-1
    1.8	2.10	1	9	-0.2	-1
    1.9	2.00	1	10	-0.1	-1
    2.0	1.80	1	12	-1.0	1
    2.1	1.70	1	13	-0.9	1
    2.2	1.60	1	14	-0.8	1
    2.3	1.50	1	15	-0.7	1
    2.4	1.40	1	16	-0.6	1
    2.5	1.30	1	17	-0.5	1
    2.6	1.20	1	18	-0.4	1
    2.7	1.10	1	19	-0.3	1
    2.8	1.00	1	20	-0.2	1
    2.9	0.90	1	20	-0.1	1

== Alternatives ==
Other alternatives to importing/creating MDWorkspaces are [[ImportMDHistoWorkspace]], [[CreateMDHistoWorkspace]] and [[CreateMDWorkspace]]


*WIKI*/

#include "MantidMDEvents/ImportMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventInserter.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include <iostream>
#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace
{
  /**
  Helper method that takes a string and performs a cast to the specified type.
  Throws an invalid_argument exception if the argument cannot be cast to the target type.
  @param str : input string
  @return strongly typed value typed to the specfied type argument.
  */
  template< typename T >
  T convert(const std::string& str)
  {
    std::istringstream iss(str);
    T obj;
    iss >> std::ws >> obj >> std::ws;
    if(!iss.eof())
    {
      throw std::invalid_argument("Wrong type destination. Cannot convert " + str);
    }
    return obj; 
  }
}

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ImportMDEventWorkspace)

  /**
  Static method returning the flag for the dimensions block.
  Allows a single definition for the flag to be used both within and outside of this class.
  @return flag.
  */
  const std::string ImportMDEventWorkspace::DimensionBlockFlag()
  {
    return "DIMENSIONS";
  }
    
  /**
  Static method returning the flag for the mdevents block.
  Allows a single definition for the flag to be used both within and outside of this class.
  @return flag.
  */
  const std::string ImportMDEventWorkspace::MDEventBlockFlag()
  {
    return "MDEVENTS";
  }

  /**
  Static method returning the flag for the comment line.
  Allows a single definition for the flag to be used both within and outside of this class.
  @return flag.
  */
  const std::string ImportMDEventWorkspace::CommentLineStartFlag()
  {
    return "#";
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ImportMDEventWorkspace::ImportMDEventWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ImportMDEventWorkspace::~ImportMDEventWorkspace()
  {

  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string ImportMDEventWorkspace::name() const { return "ImportMDEventWorkspace";};
  
  /// Algorithm's version for identification. @see Algorithm::version
  int ImportMDEventWorkspace::version() const { return 1;};
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string ImportMDEventWorkspace::category() const { return "General";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void ImportMDEventWorkspace::initDocs()
  {
    this->setWikiSummary("Reads an ASCII file containing MDEvent data and constructs an MDEventWorkspace.");
    this->setOptionalMessage("Reads an ASCII file containing MDEvent data and constructs an MDEventWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ImportMDEventWorkspace::init()
  {
    std::vector<std::string> fileExtensions(1);
    fileExtensions[0]=".txt";
    declareProperty(new API::FileProperty("Filename","", API::FileProperty::Load,fileExtensions), "File of type txt");
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }

  /**
  Extracts mdevent information from the file data and directs the creation of new MDEvents on the workspace.
  @param ws: Workspace to add the events to.
  */
  template<typename MDE, size_t nd>
  void ImportMDEventWorkspace::addEventsData(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    /// Creates a new instance of the MDEventInserter.
    MDEventInserter<typename MDEventWorkspace<MDE, nd>::sptr> inserter(ws);
    DataCollectionType::iterator mdEventEntriesIterator = m_posMDEventStart;
    for(size_t i = 0; i < m_nMDEvents; ++i)
    {
      float signal = convert<float>(*(++mdEventEntriesIterator));
      float error = convert<float>(*(++mdEventEntriesIterator));
      uint16_t run_no = 0;
      int32_t detector_no = 0;
      if(m_IsFullMDEvents)
      {
        run_no = convert<uint16_t>(*(++mdEventEntriesIterator));
        detector_no = convert<int32_t>(*(++mdEventEntriesIterator));
      }
      Mantid::coord_t centers[nd];
      for(size_t j = 0; j < m_nDimensions; ++j)
      {
        centers[j] = convert<Mantid::coord_t>(*(++mdEventEntriesIterator));
      }
      // Actually add the mdevent.
      inserter.insertMDEvent(signal, error*error, run_no, detector_no, centers);
    }
  }

  /**
  Iterate through the file data looking for the specified flag and returning TRUE if found.
  @param flag : The flag to look for.
  @return TRUE if found.
  */
  bool ImportMDEventWorkspace::fileDoesContain(const std::string& flag)
  {
    return m_file_data.end() != std::find(m_file_data.begin(), m_file_data.end(), flag);
  }

  /// Check the file contents against the file format.
  void ImportMDEventWorkspace::quickFileCheck()
  {
    // Does it have the mandatory blocks?
    if(!fileDoesContain(DimensionBlockFlag()))
    {
      std::string message = DimensionBlockFlag() + " missing from file";
      throw std::invalid_argument(message);
    }
    if(!fileDoesContain(MDEventBlockFlag()))
    {
      std::string message = MDEventBlockFlag() + " missing from file";
      throw std::invalid_argument(message);
    }
    // Are the mandatory block in the correct order.
    DataCollectionType::iterator posDimStart = std::find(m_file_data.begin(), m_file_data.end(), DimensionBlockFlag());
    DataCollectionType::iterator posMDEventStart = std::find(m_file_data.begin(), m_file_data.end(), MDEventBlockFlag());
    int posDiffDims = static_cast<int>(std::distance(posDimStart, posMDEventStart));
    if(posDiffDims < 1)
    {
      std::string message = DimensionBlockFlag() + " must be specified in file before "  + MDEventBlockFlag();
      throw std::invalid_argument(message);
    }
    // Do we have the expected number of dimension entries.
    if((posDiffDims - 1) % 4 != 0)
    {
      throw std::invalid_argument("Dimensions in the file should be specified id, name, units, nbins");
    }
    const size_t nDimensions = (posDiffDims - 1) / 4;
    // Are the dimension entries all of the correct type.
    DataCollectionType::iterator dimEntriesIterator = posDimStart;
    for(size_t i = 0; i < nDimensions; ++i)
    {
      convert<std::string>(*(++dimEntriesIterator));
      convert<std::string>(*(++dimEntriesIterator));
      convert<std::string>(*(++dimEntriesIterator));
      convert<int>(*(++dimEntriesIterator));
    }
    // Do we have the expected number of mdevent entries
    int posDiffMDEvent = static_cast<int>(std::distance(posMDEventStart, m_file_data.end()));
    const size_t columnsForFullEvents = nDimensions + 4; // signal, error, run_no, detector_no
    const size_t columnsForLeanEvents = nDimensions + 2; // signal, error
    size_t nActualColumns = 0;
    if((posDiffMDEvent - 1) % columnsForFullEvents != 0) 
    {
      if((posDiffMDEvent - 1) % columnsForLeanEvents != 0)
      {
        std::stringstream stream;
        stream << "With the dimenionality found to be " << nDimensions << ". Should either have " << columnsForLeanEvents << " or " << columnsForFullEvents << " in each row";
        throw std::invalid_argument(stream.str());
      }
    }
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ImportMDEventWorkspace::exec()
  {
    std::string filename = getProperty("Filename");

    std::ifstream file;
    try
    {
      file.open(filename.c_str(), std::ios::in);
    }
    catch (std::ifstream::failure e) 
    {
      g_log.error() << "Cannot open file: " << filename;
      throw e;
    }

    // Extract data from the file, excluding comment lines.
    std::string line;
    std::vector<std::string> myLines;
    while (std::getline(file, line))
    {
      if(std::string::npos == line.find_first_of(CommentLineStartFlag()))
      {
        std::stringstream buffer(line);
        std::copy
          ( std::istream_iterator <std::string> ( buffer ),
          std::istream_iterator <std::string> (),
          std::back_inserter( m_file_data ) );
      }
    }

    file.close();

    // Check the file format. 
    quickFileCheck();

    // Extract some well used posisions
    m_posDimStart = std::find(m_file_data.begin(), m_file_data.end(), DimensionBlockFlag());
    m_posMDEventStart = std::find(m_file_data.begin(), m_file_data.end(), MDEventBlockFlag());

    // Calculate the dimensionality
    int posDiffDims = static_cast<int>(std::distance(m_posDimStart, m_posMDEventStart));
    m_nDimensions = (posDiffDims - 1) / 4;

    // Calculate the actual number of columns in the MDEvent data.
    int posDiffMDEvent = static_cast<int>(std::distance(m_posMDEventStart, m_file_data.end()));
    const size_t columnsForFullEvents = m_nDimensions + 4; // signal, error, run_no, detector_no
    const size_t columnsForLeanEvents = m_nDimensions + 2; // signal, error
    size_t nActualColumns = 0;
    if((posDiffMDEvent - 1) % columnsForFullEvents != 0) 
    {
      nActualColumns = columnsForLeanEvents;
    }
    else
    {
      nActualColumns = columnsForFullEvents;
    }
    m_IsFullMDEvents = (nActualColumns == columnsForFullEvents);
    m_nMDEvents = posDiffMDEvent / nActualColumns;

    // Get the min and max extents in each dimension.
    std::vector<double> extentMins(m_nDimensions);
    std::vector<double> extentMaxs(m_nDimensions);
    DataCollectionType::iterator mdEventEntriesIterator = m_posMDEventStart;
    for(size_t i = 0; i < m_nMDEvents; ++i)
    {
      mdEventEntriesIterator += 2;
      if(m_IsFullMDEvents)
      {
        mdEventEntriesIterator += 2;
      }
      for(size_t j = 0; j < m_nDimensions; ++j)
      {
        double coord = convert<double>(*(++mdEventEntriesIterator));
        extentMins[j] = coord < extentMins[j] ? coord : extentMins[j];
        extentMaxs[j] = coord > extentMaxs[j] ? coord : extentMaxs[j];
      }
    }

    // Create a target output workspace.
    IMDEventWorkspace_sptr outWs = MDEventFactory::CreateMDWorkspace(m_nDimensions, m_IsFullMDEvents ? "MDEvent" : "MDLeanEvent");

    // Extract Dimensions and add to the output workspace.
    DataCollectionType::iterator dimEntriesIterator = m_posDimStart;
    for(size_t i = 0; i < m_nDimensions; ++i)
    { 
      std::string id = convert<std::string>(*(++dimEntriesIterator));
      std::string name = convert<std::string>(*(++dimEntriesIterator));
      std::string units = convert<std::string>(*(++dimEntriesIterator));
      int nbins = convert<int>(*(++dimEntriesIterator));

      outWs->addDimension(MDHistoDimension_sptr(new MDHistoDimension(id, name, units, static_cast<coord_t>(extentMins[i]), static_cast<coord_t>(extentMaxs[i]), nbins)));
    }

    CALL_MDEVENT_FUNCTION(this->addEventsData, outWs)

    // set output
    this->setProperty("OutputWorkspace", outWs);

  }



} // namespace Mantid
} // namespace MDEvents