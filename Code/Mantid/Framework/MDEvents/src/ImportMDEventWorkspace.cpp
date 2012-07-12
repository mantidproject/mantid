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

  void ImportMDEventWorkspace::quickFileCheck()
  {
    // Does it have the mandatory blocks?
    if(!fileDoesContain("DIMENSIONS"))
    {
      throw std::invalid_argument("No DIMENSIONS block in file");
    }
    if(!fileDoesContain("MDEVENTS"))
    {
      throw std::invalid_argument("No MDEVENTS block in file");
    }

    // Are the dimensionality entries {string, string, string, number}?

    // Is there at least one MDEvent entry?

    // according to the number of dimensionality entries, there should either be n + 2 or n + 4 columns in the event data section.
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