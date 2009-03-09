//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------


#include <fstream>
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/IInstrument.h"
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

// Get a reference to the logger
Logger& ReadGroupsFromFile::g_log = Logger::get("ReadGroupsFromFile");

ReadGroupsFromFile::ReadGroupsFromFile():API::Algorithm(),calibration()
{
}
/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void ReadGroupsFromFile::init()
{

	// The name of the instrument
  declareProperty("InstrumentName","");
  // The calibration file that contains the grouping information
  declareProperty("GroupingFileName","",new FileValidator(std::vector<std::string>(1,"cal"),false));
  // Flag to consider unselected detectors in the cal file
  std::vector<std::string> select;
  select.push_back("True");
  select.push_back("False");
  declareProperty("ShowUnselected","True",new ListValidator(select));
  // The output worksapce (2D) that will contain the group information
  declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace","",Direction::Output));
  // Show not selected detectors

}

/** Executes the algorithm
 *
 *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
 *  @throw runtime_error If unable to run one of the sub-algorithms successfully
 */
void ReadGroupsFromFile::exec()
{
	// create the workspace that is going to hold the instrument
	DataObjects::Workspace2D_sptr localWorkspace
	        = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(API::WorkspaceFactory::Instance().create("Workspace2D"));
	// Construct the full filename from the symbol of the instrument (
	const std::string instname=getProperty("InstrumentName");
	std::string instshort=instname.substr(0,3);
	std::transform(instshort.begin(),instshort.end(),instshort.begin(),toupper);
	instshort=instshort+"_Definition.xml";

	loadEmptyInstrument(instshort,localWorkspace);

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

	calibration.clear();
	if (!success) //Do some cleanup
	{
		localWorkspace.reset();
		throw std::runtime_error("Fail to found a detector in "+groupfile+" existing in instrument "+instshort);
	}
	setProperty("OutputWorkspace",localWorkspace);
	return;
}

void ReadGroupsFromFile::loadEmptyInstrument(const std::string& instrument_xml_name, DataObjects::Workspace2D_sptr& work)
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
	loadInst->setProperty<DataObjects::Workspace2D_sptr>("OutputWorkspace",work);

	// Now execute the sub-algorithm. Catch and log any error, but don't stop.
	try
	{
		loadInst->execute();
	}
	catch (std::runtime_error& err)
	{
		g_log.error("Unable to successfully run LoadEmptyInstrument sub-algorithm");
	}
	// Get back the output workspace
	work=loadInst->getProperty("OutputWorkspace");
	return;
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
	  return;
}


} // namespace Algorithm
} // namespace Mantid
