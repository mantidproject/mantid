#include "MantidMDEvents/ConvToMDPreprocDet.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace MDEvents
{
/// function sets appropriate energy conversion mode to work with detectors and unit conversion
void ConvToMDPreprocDet::setEmode(int mode)
{   
    if(mode<-1||mode>2){
        std::string Err="Energy conversion mode has to be between -1 and 2 but trying to set: "+boost::lexical_cast<std::string>(mode);
        throw(std::invalid_argument(Err));
    }
    emode = mode;
}
/// function sets appropriate energy  to work with detectors and unit conversion
void ConvToMDPreprocDet::setEfix(double Ei)
{
    if(Ei<=0){
        std::string Err="Input neutron's energy can not be negative but equal: "+boost::lexical_cast<std::string>(Ei);
        throw(std::invalid_argument(Err));
    }
    efix  = Ei;
}
/// function sets source-sample distance  to work with detectors and unit conversion
void ConvToMDPreprocDet::setL1(double Dist)
{
    if(Dist<0){
        std::string Err="Source-sample distance has to be positive  but equal: "+boost::lexical_cast<std::string>(Dist);
        throw(std::invalid_argument(Err));
    }
    L1  = Dist;
}
/// function checks if preprocessed detectors are already calculated and the class is actually defined properly
bool ConvToMDPreprocDet::isDefined(const API::MatrixWorkspace_const_sptr &inputWS)const
{
    if(det_dir.empty())return false;

    if(pBaseInstr !=inputWS->getInstrument()->baseInstrument())return false;
    return true;
}

void ConvToMDPreprocDet::allocDetMemory(size_t nHist)
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
void ConvToMDPreprocDet::processDetectorsPositions(const API::MatrixWorkspace_sptr inputWS,Kernel::Logger& convert_log,API::Progress *pProg)
{
  convert_log.information()<<" Preprocessing detectors locations in a target reciprocal space\n";
  // 
  Instrument_const_sptr instrument = inputWS->getInstrument();
  this->pBaseInstr               = instrument->baseInstrument();
  //
  IObjComponent_const_sptr source = instrument->getSource();
  IObjComponent_const_sptr sample = instrument->getSample();
  if ((!source) || (!sample)) {
    convert_log.error()<<" Instrument is not fully defined. Can not identify source or sample\n";
    throw Exception::InstrumentDefinitionError("Instrubment not sufficiently defined: failed to get source and/or sample");
  }

  // L1
  try{
    this->L1  = source->getDistance(*sample);
    convert_log.debug() << "Source-sample distance: " << L1 << std::endl;
  }catch (Exception::NotFoundError &)  {
    convert_log.error("Unable to calculate source-sample distance");
    throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", inputWS->getTitle());
  }
  // efix
  this->setEi(inputWS);

  //
  const size_t nHist = inputWS->getNumberHistograms();

  this->allocDetMemory(nHist);

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

     this->spec2detMap[i]= ic;
     this->det_id[ic]    = spDet->getID();
     this->detIDMap[ic]  = i;
     this->L2[ic]        = spDet->getDistance(*sample);
     

     double polar        =  inputWS->detectorTwoTheta(spDet);
     this->TwoTheta[ic]  =  polar;
     double azim         =  spDet->getPhi();    

     double sPhi=sin(polar);
     double ez = cos(polar);
     double ex = sPhi*cos(azim);
     double ey = sPhi*sin(azim);
 
     this->det_dir[ic].setX(ex);
     this->det_dir[ic].setY(ey);
     this->det_dir[ic].setZ(ez);

     ic++;
     if(i%div==0){
        pProg->report(i);
     }
   }
   // 
   if(ic<nHist){
       this->det_dir.resize(ic);
       this->det_id.resize(ic);
       this->L2.resize(ic);
       this->TwoTheta.resize(ic);
       this->detIDMap.resize(ic);
   }
   convert_log.information()<<"finished preprocessing detectors locations \n";
   pProg->report();
}
/** Function sets up energy of neurtorns used by number of conversion algorithms */
void ConvToMDPreprocDet::setEi(const API::MatrixWorkspace_sptr inputWS)
{
  try{
      Kernel::PropertyWithValue<double>  *pProp(NULL);
      pProp=dynamic_cast<Kernel::PropertyWithValue<double>  *>(inputWS->run().getProperty("Ei"));
      efix=(*pProp);
  }catch(...){
      efix= std::numeric_limits<double>::quiet_NaN();
  }
}

void ConvToMDPreprocDet::buildFakeDetectorsPositions(const API::MatrixWorkspace_sptr inputWS)
{

    L1 = 1;
    double polar(0);
    emode = 0;
    // efix
    this->setEi(inputWS);
    //
    const size_t nHist = inputWS->getNumberHistograms();
    this->allocDetMemory(nHist);
   // Loop over the spectra
   for (size_t i = 0; i < nHist; i++){


     this->spec2detMap[i]= i;
     this->det_id[i]     = (detid_t)i;
     this->detIDMap[i]   = i;
     this->L2[i]         = 10;
     

     this->TwoTheta[i] =  polar;


     double ez = 1.;
     double ex = 0.;
     double ey = 0.;
 
     this->det_dir[i].setX(ex);
     this->det_dir[i].setY(ey);
     this->det_dir[i].setZ(ez);

   }
   // 
}

// constructor
ConvToMDPreprocDet::ConvToMDPreprocDet():
emode(-2),
efix(std::numeric_limits<double>::quiet_NaN()),
L1(-1)
{}
// destructor
void ConvToMDPreprocDet::clearAll()
{
    emode = -2,
    efix  = std::numeric_limits<double>::quiet_NaN();
    L1    = -1;

    this->det_dir.clear();
    this->det_id.clear();
    this->L2.clear();
    this->TwoTheta.clear();
    this->detIDMap.clear();
    this->spec2detMap.clear();

    pBaseInstr.reset();
}

} // END MDAlgorithms ns
} // end Mantid ns
