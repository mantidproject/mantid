//-------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidDataHandling/LoadSampleDetailsFromRaw.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidAPI/Sample.h"

// The isis RAW data structure
#include "LoadRaw/isisraw2.h"

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
  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
		  "The workspace to which the information should be added");
  std::vector<std::string> exts(2, "raw");
  exts[1] = "s*";
  
  declareProperty("Filename", "", new FileValidator(exts), 
		  "The file from which to extract the information");
}

/**
 * Execute the algorithm
 */
void LoadSampleDetailsFromRaw::exec()
{
  std::string filename = getPropertyValue("Filename");
  FILE* file = fopen(filename.c_str(), "rb");
  if (file == NULL)
  {
    g_log.error("Unable to open file " + filename);
    throw Exception::FileError("Unable to open File:", filename);
  }

  ISISRAW2 *isis_raw = new ISISRAW2;
  isis_raw->ioRAW(file, true);

  MatrixWorkspace_sptr data_ws = getProperty("InputWorkspace");
  if( !data_ws.get() )
  {
    g_log.error() << "Cannot retrieve InputWorkspace " << getPropertyValue("InputWorkspace");
    throw Exception::NotFoundError("Cannot retrieve InputWorkspace", getPropertyValue("InputWorkspace"));
  }
  boost::shared_ptr<Sample> ws_sample = data_ws->getSample();
  
  if( ws_sample.get() )
  {  
    // Pick out the geometry information
    ws_sample->setGeometryFlag(isis_raw->spb.e_geom);
    double thickness = static_cast<double>(isis_raw->spb.e_thick);
    double height = static_cast<double>(isis_raw->spb.e_height);
    double width = static_cast<double>(isis_raw->spb.e_width);
    ws_sample->setGeometry(thickness, height, width);
    g_log.debug() << "Raw file sample details:\n"
		  << "\tsample geometry flag: " << isis_raw->spb.e_geom << "\n"
		  << "\tsample thickness: " << thickness << "\n"
		  << "\tsample height: " << height << "\n"
		  << "\tsample widths: " << width << std::endl;
  }
  else
  {
    g_log.warning() << getPropertyValue("InputWorkspace") << " has no sample object, sample details not set.";
  }

  // Free the used memory
  delete isis_raw;
  // Not much happens really
  progress(1.);
}

