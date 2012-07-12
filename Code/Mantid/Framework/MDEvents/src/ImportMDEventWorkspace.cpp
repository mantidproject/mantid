/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidMDEvents/ImportMDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include <iostream>
#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

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
    size_t nActualColumns = 0;
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
    // Copy the data out of the file.
    std::copy(
      std::istream_iterator <std::string> ( file ),
      std::istream_iterator <std::string> (),
      std::back_inserter( m_file_data )
      );

    file.close();

    // Check the file format. 
    quickFileCheck();

    // Extract some well used posisions
    m_posDimStart = std::find(m_file_data.begin(), m_file_data.end(), DimensionBlockFlag());
    m_posMDEventStart = std::find(m_file_data.begin(), m_file_data.end(), MDEventBlockFlag());

    // Calculate the dimensionality
    int posDiffDims = static_cast<int>(std::distance(m_posDimStart, m_posMDEventStart));
    const size_t nDimensions = (posDiffDims - 1) / 4;

    // Calculate the actual number of columns in the MDEvent data.
    int posDiffMDEvent = static_cast<int>(std::distance(m_posMDEventStart, m_file_data.end()));
    const size_t columnsForFullEvents = nDimensions + 4; // signal, error, run_no, detector_no
    const size_t columnsForLeanEvents = nDimensions + 2; // signal, error
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
    const size_t nMDEvents = posDiffMDEvent / nActualColumns;

    // Get the min and max extents in each dimension.
    DataCollectionType::iterator mdEventEntriesIterator = m_posMDEventStart;
    std::vector<double> extentMins(nDimensions);
    std::vector<double> extentMaxs(nDimensions);
    for(size_t i = 0; i < nMDEvents; ++i)
    {
      mdEventEntriesIterator += 2;
      if(m_IsFullMDEvents)
      {
        mdEventEntriesIterator += 2;
      }
      for(size_t j = 0; j < nDimensions; ++j)
      {
        double coord = convert<double>(*(++mdEventEntriesIterator));
        extentMins[j] = coord < extentMins[j] ? coord : extentMins[j];
        extentMaxs[j] = coord > extentMaxs[j] ? coord : extentMaxs[j];
      }
    }

    IMDEventWorkspace_sptr out = MDEventFactory::CreateMDWorkspace(nDimensions, m_IsFullMDEvents ? "MDEvent" : "MDLeanEvent");

    //CALL_MDEVENT_FUNCTION(this->addEventData, in_ws)

    //Extract Dimensions
    DataCollectionType::iterator dimEntriesIterator = m_posDimStart;
    for(size_t i = 0; i < nDimensions; ++i)
    {
      std::string id = convert<std::string>(*(++dimEntriesIterator));
      std::string name = convert<std::string>(*(++dimEntriesIterator));
      std::string units = convert<std::string>(*(++dimEntriesIterator));
      int nbins = convert<int>(*(++dimEntriesIterator));

      out->addDimension(MDHistoDimension_sptr(new MDHistoDimension(id, name, units, static_cast<coord_t>(extentMins[i]), static_cast<coord_t>(extentMaxs[i]), nbins)));
    }

    // Create output
    this->setProperty("OutputWorkspace", out);

  }



} // namespace Mantid
} // namespace MDEvents