#include "MantidAlgorithms/BackgroundHelper.h"


namespace Mantid
{
  namespace Algorithms
  {
    /// Constructor
    BackgroundHelper::BackgroundHelper():
      m_singleValueBackground(false),
      m_Emode(0),
      m_Efix(0),
      m_Jack05(0)
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
      const MantidVec& dataY = bkgWS->dataY(0);
      const MantidVec& dataX = bkgWS->dataX(0);
      m_Jack05 = dataY[0]/(dataX[1]-dataX[0]);


    }
    /**Method removes background from vectors which represent a histogram data for a single spectra 
    * @param nHist   -- number (workspaceID) of the spectra in the workspace, where background going to be removed
    * @param XValues -- the spectra x-values (presumably not in TOF units)
    * @param y_data  -- the spectra signal
    * @param e_data  -- the spectra errors
    */
    void BackgroundHelper::removeBackground(int nHist,const MantidVec &XValues,MantidVec &y_data,MantidVec &e_data)
    {
      double Jack05;
      if(m_singleValueBackground)
      {
        Jack05= m_Jack05;
      }
      else
      {
        const MantidVec& dataY = m_bgWs->dataY(nHist);
        const MantidVec& dataX = m_bgWs->dataX(nHist);
        Jack05 = dataY[0]/(dataX[1]-dataX[0]);
      }

      try
      {
        auto detector = m_wkWS->getDetector(nHist);
        //
        double twoTheta = m_wkWS->detectorTwoTheta(detector);
        double L2 = detector->getDistance(*m_Sample);
        double delta(std::numeric_limits<double>::quiet_NaN());
        //
        double tof1 = m_WSUnit->convertSingleToTOF(XValues[0], m_L1, L2,twoTheta, m_Emode, m_Efix,delta);
        for(size_t i=0;i<y_data.size();i++)
        {
          double tof2=m_WSUnit->convertSingleToTOF(XValues[i+1], m_L1, L2,twoTheta, m_Emode, m_Efix, delta);
          double normBkgrnd = (tof1-tof2)*Jack05;
          tof1=tof2;
          double normErr    = std::sqrt(normBkgrnd);
          y_data[i-1] -=normBkgrnd;
          e_data[i-1] +=normErr;
        }

      }
      catch(...)
      {
        // no background removal for this detector; How should it be reported?
      }

    }

  } // end API
} // end Mantid
