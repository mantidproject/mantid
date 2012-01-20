/*WIKI* 


*WIKI*/
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidCrystal/SortHKL.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/V3D.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SortHKL)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SortHKL::SortHKL()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SortHKL::~SortHKL()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SortHKL::initDocs()
  {
    this->setWikiSummary("Sorts a PeaksWorkspace by HKL.");
    this->setOptionalMessage("Sorts a PeaksWorkspace by HKL.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SortHKL::init()
  {
    declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace","",Direction::InOut),
        "An input PeaksWorkspace with an instrument.");

  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SortHKL::exec()
  {

    PeaksWorkspace_sptr ws = getProperty("InputWorkspace");

    std::vector< std::pair<std::string, bool> > criteria;
    // Sort by detector ID then descending wavelength
    criteria.push_back( std::pair<std::string, bool>("H", true) );
    criteria.push_back( std::pair<std::string, bool>("K", true) );
    criteria.push_back( std::pair<std::string, bool>("L", true) );
    ws->sort(criteria);

  }


} // namespace Mantid
} // namespace Crystal

