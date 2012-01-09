#include "MantidMDAlgorithms/ConvertToMDEventsDetInfo.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace MDAlgorithms
{


/** helper function, does preliminary calculations of the detectors positions to convert results into k-dE space ;
      and places the resutls into static cash to be used in subsequent calls to this algorithm */
void DLLExport
processDetectorsPositions(const API::MatrixWorkspace_sptr inputWS,PreprocessedDetectors &det_loc,Kernel::Logger& convert_log,API::Progress *pProg)
{
  convert_log.information()<<" Preprocessing detectors locations in a target reciprocal space\n";
  // 
  Instrument_const_sptr instrument = inputWS->getInstrument();
  //
  IObjComponent_const_sptr source = instrument->getSource();
  IObjComponent_const_sptr sample = instrument->getSample();
  if ((!source) || (!sample)) {
    convert_log.error()<<" Instrument is not fully defined. Can not identify source or sample\n";
    throw Exception::InstrumentDefinitionError("Instrubment not sufficiently defined: failed to get source and/or sample");
  }

  // L1
  try{
    det_loc.L1 = source->getDistance(*sample);
    convert_log.debug() << "Source-sample distance: " << det_loc.L1 << std::endl;
  }catch (Exception::NotFoundError &)  {
    convert_log.error("Unable to calculate source-sample distance");
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", inputWS->getTitle());
  }
  //
  const size_t nHist = inputWS->getNumberHistograms();

    det_loc.det_dir.resize(nHist);
    det_loc.det_id.resize(nHist);
    det_loc.L2.resize(nHist);
    det_loc.TwoTheta.resize(nHist);
    det_loc.detIDMap.resize(nHist);
     // Loop over the spectra
   size_t ic(0);
   for (size_t i = 0; i < nHist; i++){

     Geometry::IDetector_const_sptr spDet;
     try{
        spDet= inputWS->getDetector(i);
     }catch(Kernel::Exception::NotFoundError &){
        continue;
     }
 
    // Check that we aren't dealing with monitor...
    if (spDet->isMonitor())continue;   

     det_loc.det_id[ic]  = spDet->getID();
     det_loc.detIDMap[ic]= i;
     det_loc.L2[ic]      = spDet->getDistance(*sample);
     

     double polar        =  inputWS->detectorTwoTheta(spDet);
     det_loc.TwoTheta[ic]=  polar;
     double azim         =  spDet->getPhi();    

     double sPhi=sin(polar);
     double ez = cos(polar);
     double ex = sPhi*cos(azim);
     double ey = sPhi*sin(azim);
 
     det_loc.det_dir[ic].setX(ex);
     det_loc.det_dir[ic].setY(ey);
     det_loc.det_dir[ic].setZ(ez);

     ic++;
     pProg->report(i);
   }
   // 
   if(ic<nHist){
       det_loc.det_dir.resize(ic);
       det_loc.det_id.resize(ic);
       det_loc.L2.resize(ic);
       det_loc.TwoTheta.resize(ic);
       det_loc.detIDMap.resize(ic);
   }
   convert_log.information()<<"finished preprocessing detectors locations \n";
}

} // END MDAlgorithms ns
} // end Mantid ns
