#include "MantidMDEvents/ConvToMDPreprocDet.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid
{
  namespace MDEvents
  {
    /// function sets source-sample distance  to work with detectors and unit conversion
    void ConvToMDPreprocDet::setL1(double Dist)
    {
      if(Dist<0)
      {
        std::string Err="Source-sample distance has to be positive  but equal: "+boost::lexical_cast<std::string>(Dist);
        throw(std::invalid_argument(Err));
      }
      L1  = Dist;
    }
    /// function checks if preprocessed detectors are already calculated and the class is actually defined properly
    bool ConvToMDPreprocDet::isDefined(const API::MatrixWorkspace_const_sptr &inputWS)const
    {
      if(det_dir.empty())return false;
      if(!inputWS)throw(std::invalid_argument("ConvToMDPreprocDet::isDefined function does not work with empty input workspace pointer"));

      //TODO: an rough instrument comparison has to be performed here.
      //if(pBaseInstr !=inputWS->getInstrument()->baseInstrument())return false;
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
    void ConvToMDPreprocDet::processDetectorsPositions(const API::MatrixWorkspace_sptr inputWS,Kernel::Logger& Log,API::Progress *pProgress)
    {
      Log.information()<<" Preprocessing detectors locations in a target reciprocal space\n";
      // 
      Instrument_const_sptr instrument = inputWS->getInstrument();
      this->pBaseInstr               = instrument->baseInstrument();
      //
      IObjComponent_const_sptr source = instrument->getSource();
      IObjComponent_const_sptr sample = instrument->getSample();
      if ((!source) || (!sample)) {
        Log.error()<<" Instrument is not fully defined. Can not identify source or sample\n";
        throw Exception::InstrumentDefinitionError("Instrubment not sufficiently defined: failed to get source and/or sample");
      }

      // L1
      try{
        this->L1  = source->getDistance(*sample);
        Log.debug() << "Source-sample distance: " << L1 << std::endl;
      }catch (Exception::NotFoundError &)  {
        Log.error("Unable to calculate source-sample distance");
        throw Exception::InstrumentDefinitionError("Unable to calculate source-sample distance", inputWS->getTitle());
      }
      // efix
      //  this->setEi(inputWS);

      //
      const size_t nHist = inputWS->getNumberHistograms();

      this->allocDetMemory(nHist);

      size_t div=100;
      // Loop over the spectra
      size_t actual_detectors_count(0);
      for (size_t i = 0; i < nHist; i++)
      {

        Geometry::IDetector_const_sptr spDet;
        try
        {
          // get detector or detector group which corresponds to the spectra i
          spDet= inputWS->getDetector(i);
        }
        catch(Kernel::Exception::NotFoundError &)
        {
          continue;
        }

        // Check that we aren't dealing with monitor...
        if (spDet->isMonitor())continue;   

        this->spec2detMap[i] = actual_detectors_count;
        this->det_id[actual_detectors_count]    = spDet->getID();
        this->detIDMap[actual_detectors_count]  = i;
        this->L2[actual_detectors_count]        = spDet->getDistance(*sample);

        double polar        =  inputWS->detectorTwoTheta(spDet);
        double azim         =  spDet->getPhi();    
        this->TwoTheta[actual_detectors_count]  =  polar;

        double sPhi=sin(polar);
        double ez = cos(polar);
        double ex = sPhi*cos(azim);
        double ey = sPhi*sin(azim);

        this->det_dir[actual_detectors_count].setX(ex);
        this->det_dir[actual_detectors_count].setY(ey);
        this->det_dir[actual_detectors_count].setZ(ez);

        actual_detectors_count++;

        if(i%div==0) pProgress->report(i);

      }
      // 
      if(actual_detectors_count<nHist)
      {
        this->det_dir.resize(actual_detectors_count);
        this->det_id.resize(actual_detectors_count);
        this->L2.resize(actual_detectors_count);
        this->TwoTheta.resize(actual_detectors_count);
        this->detIDMap.resize(actual_detectors_count);
      }
      Log.information()<<"finished preprocessing detectors locations \n";
      pProgress->report();
    }
    //
    void ConvToMDPreprocDet::buildFakeDetectorsPositions(const API::MatrixWorkspace_sptr inputWS)
    {

      L1 = 1;
      double polar(0);
      //    m_Emode = 0;
      // efix
      //    this->setEi(inputWS);
      //
      const size_t nHist = inputWS->getNumberHistograms();
      this->allocDetMemory(nHist);
      // Loop over the spectra
      for (size_t i = 0; i < nHist; i++)
      {


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
    //m_Emode(-2),
    //efix(std::numeric_limits<double>::quiet_NaN()),
    L1(-1)
    {}
    // destructor
    void ConvToMDPreprocDet::clearAll()
    {
      //    m_Emode = -2,
      //    efix  = std::numeric_limits<double>::quiet_NaN();
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
