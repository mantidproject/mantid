//-------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidDataHandling/LoadSampleDetailsFromRaw.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Sample.h"

// The isis RAW data structure
#include "LoadRaw/isisraw2.h"
#include <cstdio> //MG: Required for gcc 4.4

namespace Mantid
{
namespace DataHandling
{
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadSampleDetailsFromRaw)
}
}

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataHandling;

/**
 * Initialize the algorithm
 */
void LoadSampleDetailsFromRaw::init()
{
  this->setWikiSummary("Loads the simple sample geometry that is defined within an ISIS raw file.");
  this->setOptionalMessage("Loads the simple sample geometry that is defined within an ISIS raw file.");

  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
		  "The workspace to which the information should be added");
  
  std::vector<std::string> exts;
  exts.push_back("raw");
  exts.push_back(".s*");
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts), 
		  "The file from which to extract the information");
}

/**
 * Execute the algorithm
 */
void LoadSampleDetailsFromRaw::exec()
{
  MatrixWorkspace_sptr data_ws = getProperty("InputWorkspace");
  if( !data_ws.get() )
  {
    g_log.error() << "Cannot retrieve InputWorkspace " << getPropertyValue("InputWorkspace");
    throw Exception::NotFoundError("Cannot retrieve InputWorkspace", getPropertyValue("InputWorkspace"));
  }
  
  std::string filename = getPropertyValue("Filename");
  FILE* file = fopen(filename.c_str(), "rb");
  if (file == NULL)
  {
    g_log.error("Unable to open file " + filename);
    throw Exception::FileError("Unable to open File:", filename);
  }

  ISISRAW2 *isis_raw = new ISISRAW2;
  isis_raw->ioRAW(file, true);
  fclose(file);

  // Pick out the geometry information
  data_ws->mutableSample().setGeometryFlag(isis_raw->spb.e_geom);
  data_ws->mutableSample().setThickness(static_cast<double> (isis_raw->spb.e_thick));
  data_ws->mutableSample().setHeight(static_cast<double> (isis_raw->spb.e_height));
  data_ws->mutableSample().setWidth(static_cast<double> (isis_raw->spb.e_width));

  g_log.debug() << "Raw file sample details:\n" << "\tsample geometry flag: " << isis_raw->spb.e_geom
      << "\n" << "\tsample thickness: " << data_ws->mutableSample().getThickness() << "\n"
      << "\tsample height: " << data_ws->mutableSample().getHeight() << "\n" << "\tsample width: "
      << data_ws->mutableSample().getWidth() << std::endl;

  // Free the used memory
  delete isis_raw;
  // Not much happens really
  progress(1.);
}

