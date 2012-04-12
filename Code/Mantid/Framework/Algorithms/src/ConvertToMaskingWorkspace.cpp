/*WIKI*

Convert a Workspace2D

*WIKI*/
#include "MantidAlgorithms/ConvertToMaskingWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(ConvertToMaskingWorkspace)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ConvertToMaskingWorkspace::ConvertToMaskingWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ConvertToMaskingWorkspace::~ConvertToMaskingWorkspace()
  {
  }
  
  void ConvertToMaskingWorkspace::initDocs()
  {
    this->setWikiSummary("Convert a Workspace to a Masking workspace");
    this->setOptionalMessage("Convert Workspace2D to a MaskWorkspace.");
  }

  void ConvertToMaskingWorkspace::init()
  {

    this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("InputWorkspace", "", Direction::Input),
        "Input Workspace2D.  Must have instrument associated, and cannot be focussed.");

    this->declareProperty(new API::WorkspaceProperty<DataObjects::MaskWorkspace>("OutputWorkspace", "", Direction::Output),
        "Output masking workspace.");

    return;
  }

  void ConvertToMaskingWorkspace::exec()
  {
    // 1. Get Input
    DataObjects::Workspace2D_const_sptr inWS = this->getProperty("InputWorkspace");
    g_log.debug() << "Input Workspace " << inWS->getName() << "  has " << inWS->getNumberHistograms() << " specs" << std::endl;

    // 2. Initialize output workspace
    Geometry::Instrument_const_sptr myinstrument = inWS->getInstrument();
    if (!myinstrument){
      g_log.error() << "Input Workspace " << inWS->getName() << " has not instrument associated with." << std::endl;
      throw std::invalid_argument("Input workspace has not instrument set up");
    }

    DataObjects::MaskWorkspace_sptr maskWS(new DataObjects::MaskWorkspace(myinstrument));
    g_log.debug() << "Output Masking Workspace has " << maskWS->getNumberHistograms() << " specs" << std::endl;
    std::vector<detid_t> detids = myinstrument->getDetectorIDs();
    g_log.debug() << "Instrument has " << detids.size() << " Detectors " << std::endl;

    if (inWS->getNumberHistograms() < maskWS->getNumberHistograms()){
      g_log.error() << "Input Workspace " << inWS->getName() << " has fewer spectra than the masking workspace" << std::endl;
      throw std::invalid_argument("Input workspace is focused");
    }

    // 3. Set up value
    size_t negvalue = 0;
    size_t lt1value = 0;

    // In some case, the number of spectra is larger than number of detectors, as
    // there are some monitors.  So mapping is not a trivial work
    detid2index_map *detidindexmap = inWS->getDetectorIDToWorkspaceIndexMap(true);
    g_log.debug() << "Detector Index Map Size = " << detidindexmap->size() << std::endl;
    detid2index_map::iterator mapiter;
    /*
    for (mapiter=detidindexmap->begin(); mapiter!=detidindexmap->end(); ++mapiter){
      g_log.debug() << "Key = " << mapiter->first << ",  Value = " << mapiter->second << std::endl;
    }
    */

    for (size_t i = 0; i < maskWS->getNumberHistograms(); i ++){

      // a) Get detector
      detid_t detid = maskWS->getDetectorID(i);
      size_t inpindex = detidindexmap->find(detid)->second;

      /*
      g_log.debug() << " Masking Workspace  Workspace Index = " << i << ", Detector = " << detid
          << " <--->  Input Workspace Index = " << inpindex << std::endl;
      */

      // b) Check
      if (inpindex >= inWS->getNumberHistograms()){
        g_log.warning() << "Detector ID " << detid << " Cannot Be Found In Input Workspace" << std::endl;
        continue;
      }

      // c) Set up value
      double y = inWS->dataY(inpindex)[0];
      double newy = 1.0;
      if (y < -0.001){
        negvalue ++;
        newy = 0.0;
      }
      else if (y > 1.001){
        lt1value ++;
        newy = 1.0;
      }
      else if (y < 0.5){
        newy = 0.0;
      }
      else {
        newy = 1.0;
      }
      maskWS->dataY(i)[0] = newy;
    } // ENDFOR

    g_log.warning() << "There are " << negvalue << " negative values in input Workspace " << inWS->getName() << std::endl;
    g_log.warning() << "There are " << lt1value << " values larger than 1.0 in input Workspace " << inWS->getName() << std::endl;

    // 4. Set result
    this->setProperty("OutputWorkspace", maskWS);

    // 5. Clean
    delete detidindexmap;

    return;
  }



} // namespace Mantid
} // namespace Algorithms
