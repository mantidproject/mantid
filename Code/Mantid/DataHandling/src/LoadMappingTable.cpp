#include "MantidKernel/Exception.h"
#include "LoadRaw/isisraw.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataHandling/LoadMappingTable.h"


namespace Mantid
{
namespace DataHandling
{

using namespace Kernel;
using namespace API;

DECLARE_ALGORITHM(LoadMappingTable)

Logger& LoadMappingTable::g_log = Logger::get("LoadMappingTable");

LoadMappingTable::LoadMappingTable()
{
}
void LoadMappingTable::init()
{
	declareProperty("Filename","",new MandatoryValidator); // Filename for RAW file
	declareProperty(new WorkspaceProperty<Workspace>("Workspace","Anonymous",Direction::InOut)); // Associated workspace
}
void LoadMappingTable::exec()
{
	//Get the raw file name
	m_filename = getPropertyValue("Filename");
	// Get the input workspace
	const Workspace_sptr localWorkspace = getProperty("Workspace");
	
	ISISRAW iraw(0);
	if (iraw.readFromFile(m_filename.c_str(),0) != 0) // ReadFrom File with no data
	{
	    g_log.error("Unable to open file " + m_filename);
	    throw Kernel::Exception::FileError("Unable to open File:" , m_filename);	  
	 }
	boost::shared_ptr<SpectraDetectorMap> localmap=localWorkspace->getSpectraMap();  //Get hold of the workspace 
	boost::shared_ptr<Instrument> localInstrument=localWorkspace->getInstrument(); // Get hold of the instrument associated to the workspace
	int number_spectra=iraw.i_det; // Number of entries in the spectra/udet table
	localmap->populate(iraw.spec,iraw.udet,number_spectra,localInstrument.get()); //Populate the Spectra Map with parameters
	    
	 return;
}

} // Namespace DataHandling
} // Namespace Mantid
