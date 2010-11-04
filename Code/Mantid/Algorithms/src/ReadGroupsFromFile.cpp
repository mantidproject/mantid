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
  declareProperty(new FileProperty("GroupingFileName","", FileProperty::Load, ".cal"),
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

	readGroupingFile(groupfile);

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

DataObjects::Workspace2D_sptr ReadGroupsFromFile::loadEmptyInstrument(const std::string& instrument_xml_name)
{
  // Determine the search directory for XML instrument definition files (IDFs)
	std::string directoryName = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
	if ( directoryName.empty() )
	{
		// This is the assumed deployment directory for IDFs, where we need to be relative to the
		// directory of the executable, not the current working directory.
		directoryName = Poco::Path(Mantid::Kernel::ConfigService::Instance().getBaseDir()).resolve("../Instrument").toString();
	}
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


} // namespace Algorithm
} // namespace Mantid
