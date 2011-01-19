//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <fstream>
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidDataObjects/Workspace2D.h"
#include "Poco/Path.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAlgorithms/ReadGroupsFromFile.h"

// Poco XML Headers for Grouping File
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"


namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(ReadGroupsFromFile)

      using namespace Kernel;
    using API::WorkspaceProperty;
    using API::MatrixWorkspace_sptr;
    using API::MatrixWorkspace;
    using API::FileProperty;

    ReadGroupsFromFile::ReadGroupsFromFile():API::Algorithm(),calibration()
    {
    }
    /** Initialisation method. Declares properties to be used in algorithm.
     *
     */
    void ReadGroupsFromFile::init()
    {

      // The name of the instrument
      declareProperty("InstrumentName","",
		      "The name of the instrument. This needs to be have been loaded during\n"
		      "this Mantid session either by loading a dataset from that instrument\n"
		      "or calling LoadEmptyInstrument" );
      // The calibration file that contains the grouping information
      std::vector<std::string> exts;
      exts.push_back(".cal");
      exts.push_back(".xml");
      declareProperty(new FileProperty("GroupingFileName","", FileProperty::Load, exts),
		      "The CalFile containing the grouping you want to visualize" );
      // Flag to consider unselected detectors in the cal file
      std::vector<std::string> select;
      select.push_back("True");
      select.push_back("False");
      declareProperty("ShowUnselected", "True", new ListValidator(select),
		      "Whether to show detectors that are not in any group (default yes)" );
      // The output worksapce (2D) that will contain the group information
      declareProperty(
	new API::WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output),
	"The name of the output workspace" );
    }

    /** Executes the algorithm
     *
     *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
     *  @throw runtime_error If unable to run one of the sub-algorithms successfully
     */
    void ReadGroupsFromFile::exec()
    {
      // Construct the full filename from the symbol of the instrument (
      const std::string instname=getProperty("InstrumentName");
      //std::string instshort=instname.substr(0,3);
      std::string instshort=instname;
      std::transform(instshort.begin(),instshort.end(),instshort.begin(),toupper);
      instshort=instshort+"_Definition.xml";

      DataObjects::Workspace2D_sptr localWorkspace = loadEmptyInstrument(instshort);

      const std::string groupfile=getProperty("GroupingFilename");

      if ( ! groupfile.empty() )
      {
	std::string filename(groupfile);
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
	if ( filename.find(".xml") != std::string::npos )
	{
	  readXMLGroupingFile(groupfile);
	}
	else
	{
	  readGroupingFile(groupfile);
	}
      }

      // Get the instrument.
      const int nHist=localWorkspace->getNumberHistograms();
      API::Axis* specAxis=localWorkspace->getAxis(1);
      // Get the spectra to detector map
      const API::SpectraDetectorMap& spectramap=localWorkspace->spectraMap();

      // Determine whether the user wants to see unselected detectors or not
      const std::string su=getProperty("ShowUnselected");
      bool showunselected=(!su.compare("True"));
      bool success=false;
	
      for (int i=0;i<nHist;i++)
      {
	int spec=specAxis->spectraNo(i);
	std::vector<int> dets=spectramap.getDetectors(spec);
	if (dets.empty()) // Nothing
	{
	  localWorkspace->dataY(i)[0]=0.0;
	  continue;
	}
	calmap::const_iterator it=calibration.find(dets[0]);
	if (it==calibration.end()) //Could not find the detector
	{
	  localWorkspace->dataY(i)[0]=0.0;
	  continue;
	}
	if (showunselected)
	{
	  if (((*it).second).second==0)
	    localWorkspace->dataY(i)[0]=0.0;
	  else
	    localWorkspace->dataY(i)[0]=static_cast<double>(((*it).second).first);
	}
	else
	  localWorkspace->dataY(i)[0]=static_cast<double>(((*it).second).first);
	if (!success) success=true; //At least one detector is found in the cal file 
       
      }
      progress(1);
 
      calibration.clear();
      if (!success) //Do some cleanup
      {
	localWorkspace.reset();
	throw std::runtime_error("Fail to found a detector in "+groupfile+" existing in instrument "+instshort);
      }
      setProperty("OutputWorkspace",localWorkspace);
      return;
    }
    /**
     * Load an empty instrument
     * @param instrument_xml_name The name of the instrument definition file
     * @returns A pointer to a Workspace2D containing the loaded instrument
     */    
    DataObjects::Workspace2D_sptr ReadGroupsFromFile::loadEmptyInstrument(const std::string& instrument_xml_name)
    {
      // Determine the search directory for XML instrument definition files (IDFs)
      std::string directoryName = Kernel::ConfigService::Instance().getInstrumentDirectory();
      API::IAlgorithm_sptr loadInst = createSubAlgorithm("LoadEmptyInstrument");
      loadInst->setPropertyValue("Filename", directoryName+'/'+instrument_xml_name);
  
      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
	loadInst->execute();
      }
      catch (std::runtime_error&)
      {
	g_log.error("Unable to successfully run LoadEmptyInstrument sub-algorithm");
      }
      progress(0.3);
      // Get back the output workspace
      return loadInst->getProperty("OutputWorkspace");
    }

    void ReadGroupsFromFile::readGroupingFile(const std::string& filename)
    {
      std::ifstream grFile(filename.c_str());
      if (!grFile.is_open())
      {
	g_log.error() << "Unable to open grouping file " << filename << std::endl;
	throw Exception::FileError("Error reading .cal file",filename);
      }
      calibration.clear();
      std::string str;
      while(getline(grFile,str))
      {
	// Comment, not read
	if (str.empty() || str[0] == '#') continue;
	std::istringstream istr(str);
	int n,udet,sel,group;
	double offset;
	istr >> n >> udet >> offset >> sel >> group;
	calibration[udet]=std::make_pair<int,int>(group,sel);
      }
      grFile.close();
      progress(0.7);
      return;
    }

    /**
     * Reads detctor ids for groups from an XML grouping file, such as one created by the SpatialGrouping algorithm.
     * @param filename path and name of input file
     * @throws FileError is there is a problem with the XML file
     */
    void ReadGroupsFromFile::readXMLGroupingFile(const std::string& filename)
    {
      Poco::XML::DOMParser xmlParser;
      Poco::XML::Document* file;
      try
      {
	file = xmlParser.parse(filename);
      }
      catch ( ... )
      {
	throw Kernel::Exception::FileError("Unable to parse file: ", filename);
      }

      Poco::XML::Element* root = file->documentElement();

      if ( ! root->hasChildNodes() )
      {
	throw Kernel::Exception::FileError("No root element in XML grouping file: ", filename);
      }
  
      Poco::XML::NodeList* groups = root->getElementsByTagName("group");

      if ( groups->length() == 0 )
      {
	throw Kernel::Exception::FileError("XML group file contains no group elements:", filename);
      }
  
      unsigned int nGroups = groups->length();
      for ( unsigned int i = 0; i < nGroups; i++ )
      {
	// Get the "detids" element from the grouping file
	Poco::XML::Element* elem = static_cast<Poco::XML::Element*>(groups->item(i));
	Poco::XML::Element* group = elem->getChildElement("detids");

	if ( ! group )
	{
	  throw Mantid::Kernel::Exception::FileError("XML Group File, group contains no <detids> element:", filename);
	}

	std::string ids = group->getAttribute("val");

	Poco::StringTokenizer data(ids, ",", Poco::StringTokenizer::TOK_TRIM);

	if ( data.begin() != data.end() )
	{
	  for ( Poco::StringTokenizer::Iterator it = data.begin(); it != data.end(); ++it )
	  {
	    // cast the string to an int
	    int detID;
	    try
	    {
	      detID = boost::lexical_cast<int>(*it);
	    } catch ( boost::bad_lexical_cast & )
	    {
	      throw Mantid::Kernel::Exception::FileError("Could cast string to integer in input XML file", filename);
	    }

	    if ( calibration.find(detID) == calibration.end() )
	    {
	      // add detector to a group
	      calibration[detID] = std::pair<int,int>(i+1, 1);
	    }
	  }
	}
      }

      progress(0.7);
    }


  } // namespace Algorithm
} // namespace Mantid
