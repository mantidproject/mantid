//-------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidDataHandling/LoadLOQDistancesFromRaw.h"

#include "MantidAPI/FileProperty.h"

#include "MantidGeometry/V3D.h"
#include "MantidGeometry/IComponent.h"

// The isis RAW data structure
#include "LoadRaw/isisraw2.h"
#include <cstdio> //Required for gcc 4.4

namespace Mantid
{
namespace DataHandling
{
  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadLOQDistancesFromRaw)
}
}

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataHandling;

/**
 * Initialize the algorithm
 */
void LoadLOQDistancesFromRaw::init()
{

  declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input),
		  "The workspace to which the information should be added");

  std::vector<std::string> exts;
  exts.push_back(".raw");
  exts.push_back(".s*");
  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts), 
		  "The file from which to extract the information");
}

/**
 * Execute the algorithm
 */
void LoadLOQDistancesFromRaw::exec()
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
  // Specific LOQ distances
  // Moderator to sample distance (primary flight path)
  double i_l1 = static_cast<double>(isis_raw->ivpb.i_l1);
  // Sample-det distance (secondary flight path)
  double sddist = static_cast<double>(isis_raw->ivpb.i_sddist);
  g_log.debug() << "Flight paths: moderator-sample: " << i_l1 << "m,  sample-detector: " << sddist << "m." << std::endl;

  // Get current sample position
  std::string compname = "some-sample-holder";
  boost::shared_ptr<Geometry::IComponent> sample_holder = data_ws->getInstrument()->getComponentByName(compname);
  if( sample_holder.get() )
  {
    double curr_zpos = sample_holder->getPos().Z();
    double rel_shift = i_l1 - curr_zpos;
    if( std::abs(rel_shift) / curr_zpos > 1e-08 )
    {
      g_log.debug("Running MoveInstrumentComponent as a sub algorithm for the sample holder.");
      // This performs a relative shift in the component along the Z axis
      performMoveComponent(compname, rel_shift, 0.0, 0.5);
    }
  }
  //Now the secondary flight path relative the the sample
  // Redo the get so that it retrieves an up-to-date parameterized component if a move occurred
  sample_holder = data_ws->getInstrument()->getComponentByName(compname);
  //Get the main detector component
  compname = "main-detector-bank";
  boost::shared_ptr<Geometry::IComponent> main_det = data_ws->getInstrument()->getComponentByName(compname);
  if( main_det.get() && sample_holder.get() )
  {
    double curr_zpos = main_det->getPos().Z();
    double rel_shift = i_l1 + sddist - curr_zpos;
    if( std::abs(rel_shift) / curr_zpos > 1e-08 )
    {
      g_log.debug("Running MoveInstrumentComponent as a sub algorithm for the main-detector-bank");
      // This performs a relative shift in the component along the Z axis
      performMoveComponent(compname, rel_shift, 0.5, 1.0);
    }
  }
  // Free the used memory
  delete isis_raw;
}

/**
 * Run the MoveInstrumentComponent algorithm as a child algorithm
 * @param comp_name :: The component name
 * @param zshift :: The shift along the Z-axis
 * @param start_progress :: The starting percentage for progress reporting
 * @param end_progress :: The end percentage for progress reporting
 */
void LoadLOQDistancesFromRaw::performMoveComponent(const std::string & comp_name, double zshift, 
						   double start_progress, double end_progress)
{
  IAlgorithm_sptr alg = createSubAlgorithm("MoveInstrumentComponent", start_progress, end_progress);
  alg->setPropertyValue("Workspace", getPropertyValue("InputWorkspace"));
  alg->setPropertyValue("ComponentName", comp_name);
  alg->setProperty("Z", zshift);
  alg->setPropertyValue("RelativePosition", "1");
  try
  {
    alg->execute();
  }
  catch (std::runtime_error&)
  {
    g_log.information("Unable to successfully run MoveInstrumentComponent sub-algorithm");
    throw std::runtime_error("Error while executing MoveInstrumentComponent as a sub algorithm.");
  }

}
