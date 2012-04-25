#include "MantidMDAlgorithms/ConvToMDPreprocDetectors.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace MDAlgorithms
{
/// function sets appropriate energy conversion mode to work with detectors and unit conversion
void ConvToMDPreprocDetectors::setEmode(int mode)
{   
    if(mode<-1||mode>2){
        std::string Err="Energy conversion mode has to be between -1 and 2 but trying to set: "+boost::lexical_cast<std::string>(mode);
        throw(std::invalid_argument(Err));
    }
    emode = mode;
}
/// function sets appropriate energy  to work with detectors and unit conversion
void ConvToMDPreprocDetectors::setEfix(double Ei)
{
    if(Ei<=0){
        std::string Err="Input neutron's energy can not be negative but equal: "+boost::lexical_cast<std::string>(Ei);
        throw(std::invalid_argument(Err));
    }
    efix  = Ei;
}
/// function sets source-sample distance  to work with detectors and unit conversion
void ConvToMDPreprocDetectors::setL1(double Dist)
{
    if(Dist<0){
        std::string Err="Source-sample distance has to be positive  but equal: "+boost::lexical_cast<std::string>(Dist);
        throw(std::invalid_argument(Err));
    }
    L1  = Dist;
}
/// function checks if preprocessed detectors are already calculated
bool ConvToMDPreprocDetectors::isDefined(const API::MatrixWorkspace_const_sptr &inputWS)const
{
    if(det_dir.empty())return false;

    if(pBaseInstr !=inputWS->getInstrument()->baseInstrument())return false;
    return true;
}

void ConvToMDPreprocDetectors::allocDetMemory(size_t nHist)
{
    this->det_dir.resize(nHist);
    this->det_id.resize(nHist);
    this->L2.resize(nHist);
    this->TwoTheta.resize(nHist);
    this->detIDMap.resize(nHist);
    this->spec2detMap.assign(nHist,uint32_t(-1));


}

/** helper function, does preliminary calculations of the detectors positions to convert results into k-dE space ;
      and places the resutls into static cash to be used in subsequent calls to this algorithm */
void DLLExport processDetectorsPositions(const API::MatrixWorkspace_sptr inputWS,ConvToMDPreprocDetectors &det_loc,Kernel::Logger& convert_log,API::Progress *pProg)
{
  convert_log.information()<<" Preprocessing detectors locations in a target reciprocal space\n";
  // 
  Instrument_const_sptr instrument = inputWS->getInstrument();
  det_loc.pBaseInstr               = instrument->baseInstrument();
  //
  IObjComponent_const_sptr source = instrument->getSource();
  IObjComponent_const_sptr sample = instrument->getSample();
  if ((!source) || (!sample)) {
    convert_log.error()<<" Instrument is not fully defined. Can not identify source or sample\n";
    throw Exception::InstrumentDefinitionError("Instrubment not sufficiently defined: failed to get source and/or sample");
  }

  // L1
  try{
    double L1  = source->getDistance(*sample);
    det_loc.setL1(L1);
    convert_log.debug() << "Source-sample distance: " << L1 << std::endl;
  }catch (Exception::NotFoundError &)  {
    convert_log.error("Unable to calculate source-sample distance");
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", inputWS->getTitle());
  }
  //
  const size_t nHist = inputWS->getNumberHistograms();

  det_loc.allocDetMemory(nHist);

  size_t div=100;
     // Loop over the spectra
  size_t ic(0);
  for (size_t i = 0; i < nHist; i++){

     Geometry::IDetector_const_sptr spDet;
     try{
        // get detector or detector group which corresponds to the spectra i
         spDet= inputWS->getDetector(i);
     }catch(Kernel::Exception::NotFoundError &){
        continue;
     }
 
    // Check that we aren't dealing with monitor...
    if (spDet->isMonitor())continue;   

     det_loc.spec2detMap[i]= ic;
     det_loc.det_id[ic]    = spDet->getID();
     det_loc.detIDMap[ic]  = i;
     det_loc.L2[ic]        = spDet->getDistance(*sample);
     

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
     if( i%div==0){
        pProg->report(i);
     }
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
   pProg->report();
}

void DLLExport buildFakeDetectorsPositions(const API::MatrixWorkspace_sptr inputWS,ConvToMDPreprocDetectors &det_loc)
{

    det_loc.setL1(1);
    double polar(0);
    //
    const size_t nHist = inputWS->getNumberHistograms();
    det_loc.allocDetMemory(nHist);
   // Loop over the spectra
   for (size_t i = 0; i < nHist; i++){


     det_loc.spec2detMap[i]= i;
     det_loc.det_id[i]     = (detid_t)i;
     det_loc.detIDMap[i]   = i;
     det_loc.L2[i]         = 10;
     

     det_loc.TwoTheta[i] =  polar;


     double ez = 1.;
     double ex = 0.;
     double ey = 0.;
 
     det_loc.det_dir[i].setX(ex);
     det_loc.det_dir[i].setY(ey);
     det_loc.det_dir[i].setZ(ez);

   }
   // 
}

} // END MDAlgorithms ns
} // end Mantid ns
