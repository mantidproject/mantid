/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidMDEvents/ImportMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidMDEvents/MDEventFactory.h"
#include <iostream>
#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ImportMDEventWorkspace)
  
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

  const std::string ImportMDEventWorkspace::DimensionBlockFlag()
  {
    return "DIMENSIONS";
  }
    

  const std::string ImportMDEventWorkspace::MDEventBlockFlag()
  {
    return "MDEVENTS";
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

  template<typename MDE, size_t nd>
  void ImportMDEventWorkspace::addEventData(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
  }

  ImportMDEventWorkspace::MDEventType ImportMDEventWorkspace::readEventFlag()
  {
    return MDEventType::NotSpecified;
  }

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
      std::string id = convert<std::string>(*(++dimEntriesIterator));
      std::string name = convert<std::string>(*(++dimEntriesIterator));
      std::string units = convert<std::string>(*(++dimEntriesIterator));
      int nbins = convert<int>(*(++dimEntriesIterator));
    }
    // Do we have the expected number of mdevent entries
    int posDiffMDEvent = static_cast<int>(std::distance(posMDEventStart, m_file_data.end()));
    const size_t columnsForFullEvents = nDimensions + 4; // signal, error, run_no, detector_no
    const size_t columnsForLeanEvents = nDimensions + 2; // signal, error
    int nActualColumns = 0;
    if((posDiffMDEvent - 1) % columnsForFullEvents != 0) 
    {
      if((posDiffMDEvent - 1) % columnsForLeanEvents != 0)
      {
        std::stringstream stream;
        stream << "With the dimenionality found to be " << nDimensions << ". Should either have " << columnsForLeanEvents << " or " << columnsForFullEvents << " in each row";
        throw std::invalid_argument(stream.str());
      }
      else
      {
        nActualColumns = columnsForLeanEvents;
      }
    }
    else
    {
      nActualColumns = columnsForFullEvents;
    }
    const size_t nMDEvents = posDiffMDEvent / nActualColumns;

    DataCollectionType::iterator mdEventEntriesIterator = posMDEventStart;
    for(size_t i = 0; i < nMDEvents; ++i)
    {
      double signal = convert<double>(*(++mdEventEntriesIterator));
      double error = convert<double>(*(++mdEventEntriesIterator));
      if(nActualColumns == columnsForFullEvents)
      {
        int run_no = convert<int>(*(++mdEventEntriesIterator));
        int detector_no = convert<int>(*(++mdEventEntriesIterator));
      }
      for(size_t j = 0; j < nDimensions; ++j)
      {
        double coord = convert<double>(*(++mdEventEntriesIterator));
      }
    }
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ImportMDEventWorkspace::exec()
  {
    std::string filename = getProperty("Filename");

    //size_t ndims = 2;
    //std::string eventType = "MDLeanEvent";
    //IMDEventWorkspace_sptr ws = MDEventFactory::CreateMDWorkspace(ndims, eventType);
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

    std::copy(
      std::istream_iterator <std::string> ( file ),
      std::istream_iterator <std::string> (),
      std::back_inserter( m_file_data )
      );

    file.close();

    quickFileCheck();

    // look for the event_type flag.
    //MDEventType eventType = readEventFlag();
    

    // read the raw data.

    // interpret the data w.r.t the event_type flag. Throw if invalid or set the flag.

    // once the data has some meaning, use the coord columns to determine the min and max in each dimension.

    // read the dimensions

    // create the empty workspace

    // Fill the empty workspace

    //std::vector<IMDDimension> dimensions = readDimension();



    //IMDEventWorkspace_sptr ws = createEmptyOutputWorkspace(dimensions);
    //std::vector< createEventList();

    //CALL_MDEVENT_FUNCTION(this->addEventData, in_ws)
  

    // Copy each string present in the file stream into a deque.
    //typedef std::deque<std::string> box_collection;
    //box_collection box_elements;
    //std::copy(
    //  std::istream_iterator <std::string> ( file ),
    //  std::istream_iterator <std::string> (),
    //  std::back_inserter( box_elements )
    //  );

  }



} // namespace Mantid
} // namespace MDEvents