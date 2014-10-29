#include "MantidAlgorithms/BackgroundHelper.h"


namespace Mantid
{
  namespace Algorithms
  {
  
    /// Constructor
    BackgroundHelper::BackgroundHelper():
      m_WSUnit(),m_bgWs(),m_wkWS(),
      m_singleValueBackground(false),
      m_NBg(0),m_dtBg(1), //m_ErrSq(0),
      m_Emode(0),
      m_L1(0),m_Efix(0),
      m_Sample(),
      FailingSpectraList()
    {};

    /** Initialization method: 
    @param bkgWS    -- shared pointer to the workspace which contains background 
    @param sourceWS -- shared pointer to the workspace to remove background from
    @param emode    -- energy conversion mode used during internal units conversion (0 -- elastic, 1-direct, 2 indirect, as defined in 
    */
    void BackgroundHelper::initialize(const API::MatrixWorkspace_const_sptr &bkgWS,const API::MatrixWorkspace_sptr &sourceWS,int emode)
    {
      m_bgWs = bkgWS;
      m_wkWS = sourceWS;
      m_Emode = emode;
      FailingSpectraList.clear();

      std::string bgUnits = bkgWS->getAxis(0)->unit()->unitID();
      if(bgUnits!="TOF")
        throw std::invalid_argument(" Background Workspace: "+bkgWS->getName()+" should be in the units of TOF");

      if(!(bkgWS->getNumberHistograms() == 1 || sourceWS->getNumberHistograms()==bkgWS->getNumberHistograms()))
        throw std::invalid_argument(" Background Workspace: "+bkgWS->getName()+" should have the same number of spectra as source workspace or be a single histogram workspace");  

      m_WSUnit = sourceWS->getAxis(0)->unit();
      if(!m_WSUnit)
        throw std::invalid_argument(" Source Workspace: "+sourceWS->getName()+" should have units");

      Geometry::IComponent_const_sptr source = sourceWS->getInstrument()->getSource();
      m_Sample = sourceWS->getInstrument()->getSample();
      if ((!source) || (!m_Sample)) 
        throw std::invalid_argument("Instrument on Source workspace:"+sourceWS->getName()+"is not sufficiently defined: failed to get source and/or sample");
      m_L1 = source->getDistance(*m_Sample);


      m_singleValueBackground = false;
      if(bkgWS->getNumberHistograms()==0)
        m_singleValueBackground = true;
      const MantidVec& dataX = bkgWS->dataX(0);
      const MantidVec& dataY = bkgWS->dataY(0);
      //const MantidVec& dataE = bkgWS->dataE(0);
      m_NBg    = dataY[0];
      m_dtBg   = dataX[1]-dataX[0];
      //m_ErrSq  = dataE[0]*dataE[0]; // needs further clarification


      m_Efix = this->getEi(sourceWS);
    }
    /**Method removes background from vectors which represent a histogram data for a single spectra 
    * @param nHist   -- number (workspaceID) of the spectra in the workspace, where background going to be removed
    * @param XValues -- the spectra x-values (presumably not in TOF units)
    * @param y_data  -- the spectra signal
    * @param e_data  -- the spectra errors
    */
    void BackgroundHelper::removeBackground(int nHist,const MantidVec &XValues,MantidVec &y_data,MantidVec &e_data)const
    {

      double dtBg,IBg;
      if(m_singleValueBackground)
      {
        dtBg  = m_dtBg;
       // ErrSq = m_ErrSq;
        IBg   = m_NBg;
      }
      else
      {
        const MantidVec& dataX = m_bgWs->dataX(nHist);
        const MantidVec& dataY = m_bgWs->dataY(nHist);
        //const MantidVec& dataE = m_bgWs->dataX(nHist);
        dtBg = (dataX[1]-dataX[0]);
        IBg  = dataY[0];
        //ErrSq= dataE[0]*dataE[0]; // Needs further clarification
      }

      try
      {
        auto detector = m_wkWS->getDetector(nHist);
        //
        double twoTheta = m_wkWS->detectorTwoTheta(detector);
        double L2 = detector->getDistance(*m_Sample);
        double delta(std::numeric_limits<double>::quiet_NaN());
        // clone unit conversion to avoid multithreading issues
        auto unitConv = m_WSUnit->clone();
        unitConv->initialize(m_L1, L2,twoTheta, m_Emode, m_Efix,delta);
        double tof1 = unitConv->singleToTOF(XValues[0]);
        for(size_t i=0;i<y_data.size();i++)
        {
          double tof2=unitConv->singleToTOF(XValues[i+1]);
          double Jack = std::fabs((tof2-tof1)/dtBg);
          double normBkgrnd = IBg*Jack;
          tof1=tof2;
          y_data[i] -=normBkgrnd;
          //e_data[i]  =std::sqrt((ErrSq*Jack*Jack+e_data[i]*e_data[i])/2.); // needs further clarification -- Gaussian error summation?
          //--> assume error for background is sqrt(signal):
          e_data[i]  =std::sqrt((normBkgrnd+e_data[i]*e_data[i])/2.); // needs further clarification -- Gaussian error summation?
        }

      }
      catch(...)
      {
        // no background removal for this spectra as it does not have a detector or other reason; How should it be reported?
        FailingSpectraList.push_front(nHist);
      }

    }
    /** Method returns the efixed or Ei value stored in properties of the input workspace.
     *  Indirect instruments can have eFxed and Direct instruments can have Ei defined as the properties of the workspace. 
     *
     *  This method provide guess for efixed for all other kind of instruments. Correct indirect instrument will overwrite 
     *  this value while wrongly defined or different types of instruments will provide the value of "Ei" property (log value)
     *  or undefined if "Ei" property is not found.
     *
     */
    double BackgroundHelper::getEi(const API::MatrixWorkspace_const_sptr &inputWS)const
    { 
      double Efi = std::numeric_limits<double>::quiet_NaN();
   
    
      // is Ei on workspace properties? (it can be defined for some reason if detectors do not have one, and then it would exist as Ei)
      bool EiFound(false);
      try
      {
         Efi =  inputWS->run().getPropertyValueAsType<double>("Ei");
         EiFound = true;
      }
      catch(Kernel::Exception::NotFoundError &)
      {}
      // try to get Efixed as property on a workspace, obtained for indirect instrument
      //bool eFixedFound(false);
      if (!EiFound)
      {
        try
        {
           Efi  =inputWS->run().getPropertyValueAsType<double>("eFixed");
           //eFixedFound = true;
        }
        catch(Kernel::Exception::NotFoundError &)
        {}
      }

      //if (!(EiFound||eFixedFound))
      //  g_log.debug()<<" Ei/eFixed requested but have not been found\n";

      return Efi;
    }


  } // end API
} // end Mantid
