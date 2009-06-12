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
  declareProperty("Filename","", new MandatoryValidator<std::string>,
    "The name of the RAW file from which to obtain the mapping information,\n"
    "including its full or relative path" );
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
    "The name of the input and output workspace on which to perform the\n"
    "algorithm" );
}

void LoadMappingTable::exec()
{
  //Get the raw file name
  m_filename = getPropertyValue("Filename");
  // Get the input workspace
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");
  ISISRAW iraw(NULL);
  if (iraw.readFromFile(m_filename.c_str(),0) != 0) // ReadFrom File with no data
  {
    g_log.error("Unable to open file " + m_filename);
    throw Kernel::Exception::FileError("Unable to open File:" , m_filename);
  }
  const int number_spectra=iraw.i_det; // Number of entries in the spectra/udet table
  if ( number_spectra == 0 )
  {
    g_log.warning("The spectra to detector mapping table is empty");
  }
  //Populate the Spectra Map with parameters
  localWorkspace->mutableSpectraMap().populate(iraw.spec,iraw.udet,number_spectra);

  return;
}

} // Namespace DataHandling
} // Namespace Mantid
