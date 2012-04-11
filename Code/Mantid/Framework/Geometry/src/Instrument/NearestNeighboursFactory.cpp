#include "MantidGeometry/Instrument/NearestNeighboursFactory.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
//using namespace Mantid::API;

namespace Mantid
{
namespace Geometry
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  NearestNeighboursFactory::NearestNeighboursFactory()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  NearestNeighboursFactory::~NearestNeighboursFactory()
  {
  }

  /*
  Factory Method
  @param instrument : Instrument containing detectors
  @param spectraMap : Spectra to detector map.
  @param ignoreMasked : True to ignore masked detectors 
  */
  NearestNeighbours* NearestNeighboursFactory::create(boost::shared_ptr<const Instrument> instrument,
                        const ISpectraDetectorMap & spectraMap, bool ignoreMasked)
  {
    return new NearestNeighbours(instrument, spectraMap, ignoreMasked);
  }
  
  /*
  Factory Method 
  @param numberOfNeighbours : Number of neighbours.
  @param instrument : Instrument containing detectors
  @param spectraMap : Spectra to detector map.
  @param ignoreMasked : True to ignore masked detectors 
  */
  NearestNeighbours* NearestNeighboursFactory::create(int numberOfNeighbours, boost::shared_ptr<const Instrument> instrument,
        const ISpectraDetectorMap & spectraMap, bool ignoreMasked)
  {
    return new NearestNeighbours(numberOfNeighbours, instrument, spectraMap, ignoreMasked);
  }

} // namespace Mantid
} // namespace Geometry