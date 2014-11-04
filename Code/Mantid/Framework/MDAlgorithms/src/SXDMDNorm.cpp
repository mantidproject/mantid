#include "MantidMDAlgorithms/SXDMDNorm.h"

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid
{
  namespace MDAlgorithms
  {
    
    using Mantid::Kernel::Direction;
    using Mantid::API::WorkspaceProperty;
    using namespace Mantid::MDEvents;
    using namespace Mantid::API;
    using namespace Mantid::Kernel;
    
    namespace
    {
      //function to  compare two intersections (h,k,l,Momentum) by Momentum
      bool compareMomentum(const Mantid::Kernel::VMD &v1, const Mantid::Kernel::VMD &v2)
      {
        return (v1[3] < v2[3]);
      }
    }
    
    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(SXDMDNorm)
    
    //----------------------------------------------------------------------------------------------
    /**
     * Constructor
     */
    SXDMDNorm::SXDMDNorm() :
      m_nDims(0), m_normWS(), m_inputWS(), m_hmin(0.0f), m_hmax(0.0f),
      m_kmin(0.0f), m_kmax(0.0f), m_lmin(0.0f), m_lmax(0.0f), m_hIntegrated(true),
      m_kIntegrated(true), m_lIntegrated(true), m_rubw(3,3),
      m_kiMin(0.0), m_kiMax(EMPTY_DBL()), m_hIdx(-1), m_kIdx(-1), m_lIdx(-1),
      m_hX(), m_kX(), m_lX()
    {
    }
    
    /// Algorithm's version for identification. @see Algorithm::version
    int SXDMDNorm::version() const { return 1; }
    
    /// Algorithm's category for identification. @see Algorithm::category
    const std::string SXDMDNorm::category() const { return "MDAlgorithms"; }
    
    /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
    const std::string SXDMDNorm::summary() const
    { 
      return "Calculate normalization for an MDEvent workspace for single crystal diffraction.";
    }
    
    /// Algorithm's name for use in the GUI and help. @see Algorithm::name
    const std::string SXDMDNorm::name() const { return "SXDMDNorm"; }
    
    //----------------------------------------------------------------------------------------------
    /**
      * Initialize the algorithm's properties.
      */
    void SXDMDNorm::init()
    {
      declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::Input),
                      "An input MDWorkspace.");
      
      std::string dimChars = getDimensionChars();
      // --------------- Axis-aligned properties ---------------------------------------
      for (size_t i = 0; i<dimChars.size(); i++)
      {
        std::string dim(" "); dim[0] = dimChars[i];
        std::string propName = "AlignedDim" + dim;
        declareProperty(new PropertyWithValue<std::string>(propName,"",Direction::Input),
                        "Binning parameters for the " + Strings::toString(i) + "th dimension.\n"
                        "Enter it as a comma-separated list of values with the format: 'name,minimum,maximum,number_of_bins'. Leave blank for NONE.");
      }
      
      auto wsValidator = boost::make_shared<CompositeValidator>();
      wsValidator->add<WorkspaceUnitValidator>("Momentum");
      wsValidator->add<InstrumentValidator>();
      wsValidator->add<CommonBinsValidator>();
      
      declareProperty(new WorkspaceProperty<>("FluxWorkspace","",Direction::Input,wsValidator), 
                      "An input workspace containing momentum dependent flux.");
      declareProperty(new WorkspaceProperty<>("SolidAngleWorkspace","",Direction::Input,wsValidator->clone()),
                      "An input workspace containing momentum integrated vanadium (a measure of the solid angle).");
      
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output),
                      "A name for the output data MDHistoWorkspace.");
      declareProperty(new WorkspaceProperty<Workspace>("OutputNormalizationWorkspace","",Direction::Output),
                      "A name for the output normalization MDHistoWorkspace.");
    }
    
    //----------------------------------------------------------------------------------------------
    /**
     * Execute the algorithm.
     */
    void SXDMDNorm::exec()
    {
      initCaches();
      auto outputWS = binInputWS();
      setProperty<Workspace_sptr>("OutputWorkspace", outputWS);
      createNormalizationWS(*outputWS);

      // Check for other dimensions if we could measure anything in the original data
      bool skipNormalization = false;
      const std::vector<coord_t> otherValues = getValuesFromOtherDimensions(skipNormalization);
      const auto affineMat = findIntergratedDimensions(otherValues, skipNormalization);

      auto &hDim = *m_normWS->getDimension(m_hIdx);
      m_hX.resize( hDim.getNBins() );
      for(size_t i = 0; i < m_hX.size(); ++i)
      {
        m_hX[i] = hDim.getX(i);
      }
      auto &kDim = *m_normWS->getDimension(m_kIdx);
      m_kX.resize( kDim.getNBins() );
      for(size_t i = 0; i < m_kX.size(); ++i)
      {
        m_kX[i] = kDim.getX(i);
      }
      auto &lDim = *m_normWS->getDimension(m_lIdx);
      m_lX.resize( lDim.getNBins() );
      for(size_t i = 0; i < m_lX.size(); ++i)
      {
        m_lX[i] = lDim.getX(i);
      }

      API::MatrixWorkspace_const_sptr fW = getProperty("FluxWorkspace");
      DataObjects::EventWorkspace_const_sptr fluxW = boost::dynamic_pointer_cast<const DataObjects::EventWorkspace>(fW);
      m_kiMin = fluxW->getEventXMin();
      m_kiMax = fluxW->getEventXMax();
      API::MatrixWorkspace_const_sptr sA = getProperty("SolidAngleWorkspace");

      if(skipNormalization)
      {
        g_log.warning("Binning limits are outside the limits of the MDWorkspace\n");
      }
      else
      {
        double PC = m_normWS->getExperimentInfo(0)->run().getProtonCharge();
        Kernel::PropertyWithValue< std::vector<double> > *prop = dynamic_cast< Mantid::Kernel::PropertyWithValue<std::vector<double> >*>(m_normWS->getExperimentInfo(0)->getLog("RUBW_MATRIX"));
        if (prop==NULL)
        {
          throw std::runtime_error("No RUBW_MATRIX");
        }
        else
        {
          Mantid::Kernel::DblMatrix RUBW((*prop)()); //includes the 2*pi factor but not goniometer for now :)
          m_rubw = m_normWS->getExperimentInfo(0)->run().getGoniometerMatrix()*RUBW;
          m_rubw.Invert();
        }
        //FIXME: the detector positions are from the IDF. Need to account for calibration
        std::vector<detid_t> detIDS = m_normWS->getExperimentInfo(0)->getInstrument()->getDetectorIDs(true);
        
        Mantid::API::Progress *prog = new Mantid::API::Progress(this,0.3,1,static_cast<int64_t>(detIDS.size()));
        const detid2index_map d2m = fluxW->getDetectorIDToWorkspaceIndexMap();
        const detid2index_map d2mSA = sA->getDetectorIDToWorkspaceIndexMap();
        auto instrument = m_normWS->getExperimentInfo(0)->getInstrument();
        
        PARALLEL_FOR1(m_normWS)
        for(int64_t i = 0; i < static_cast<int64_t>(detIDS.size()); i++)
        {
          PARALLEL_START_INTERUPT_REGION

          Mantid::Geometry::IDetector_const_sptr detector = instrument->getDetector(detIDS[i]);
          if(!detector->isMonitor() && !detector->isMasked())
          {
            //get intersections
            std::vector<Mantid::Kernel::VMD> intersections = calculateIntersections(detector);
            
            if(!intersections.empty())
            {
              //calculate indices
              //add to the correct signal at that particular index
              //NOTE: if parallel it has to be atomic/critical
              
              //get event vector
              size_t sp = d2m.find(detIDS[i])->second;
              std::vector<Mantid::DataObjects::WeightedEventNoTime> el = fluxW->getEventList(sp).getWeightedEventsNoTime();
              //get iterator to the first event that has momentum >= (*intersections.begin())[3]
              std::vector<Mantid::DataObjects::WeightedEventNoTime>::iterator start = el.begin();
              // check that el isn't empty
              if ( start == el.end() ) continue;
              while((*start).tof()<(*intersections.begin())[3]) ++start;
              
              double solid = sA->readY(d2mSA.find(detIDS[i])->second)[0]*PC;
              
              const size_t sizeOfMVD = intersections.front().size();
              // pre-allocate for efficiency
              std::vector<coord_t> pos( sizeOfMVD + otherValues.size() );
              
              for (auto it = intersections.begin()+1;it!=intersections.end();++it)
              {
                //Mantid::Kernel::VMD deltav=(*it)-(*(it-1));//difference between consecutive intersections
                // the full vector isn't used so compute only what is necessary
                double delta = (*it)[3] - (*(it-1))[3];
                
                double eps = 1e-7;//do not integrate if momemntum difference is smaller than eps, assume contribution is 0
                if (delta > eps)
                {
                  //Mantid::Kernel::VMD avev=((*it)+(*(it-1)))*0.5;//average between two intersection (to get position)
                  //std::vector<coord_t> pos = avev.toVector<coord_t>();
                  //pos.insert(pos.end()-1,otherValues.begin(),otherValues.end());
                  // a bit longer and less readable but faster version of the above
                  std::transform( it->getBareArray(), it->getBareArray() + sizeOfMVD, (it-1)->getBareArray(), pos.begin(), std::plus<coord_t>() );
                  std::transform( pos.begin(), pos.begin() + sizeOfMVD, pos.begin(), std::bind2nd( std::multiplies<coord_t>(), 0.5 ) );
                  std::copy( otherValues.begin(), otherValues.end(), pos.begin() + sizeOfMVD );
                  
                  std::vector<coord_t> posNew = affineMat*pos;
                  size_t linIndex = m_normWS->getLinearIndexAtCoord(posNew.data());
                  
                  if(linIndex!=size_t(-1))
                  {
                    double signal = 0.;
                    while((*start).tof()<(*it)[3])
                    {
                      if (start==el.end())
                        break;
                      signal+=(*start).weight();
                      ++start;
                    }
                    signal*=solid;
                    
                    PARALLEL_CRITICAL(updateMD)
                    {
                      signal+=m_normWS->getSignalAt(linIndex);
                      m_normWS->setSignalAt(linIndex,signal);
                    }
                  }
                }
              }
              
            }
          }
          prog->report();
          
          PARALLEL_END_INTERUPT_REGION
        }
        PARALLEL_CHECK_INTERUPT_REGION

       delete prog;
      }
      
      this->setProperty("OutputNormalizationWorkspace",m_normWS);
      
    }

    /**
     * Set up starting values for cached variables
     */
    void SXDMDNorm::initCaches()
    {
      m_inputWS = getProperty("InputWorkspace");
      if( inputEnergyMode() != "Elastic" )
      {
        throw std::invalid_argument("Invalid energy transfer mode. Algorithm currently only supports elastic data.");
      }
      // Min/max dimension values
      const auto hdim(m_inputWS->getDimension(0)), kdim(m_inputWS->getDimension(1)),
          ldim(m_inputWS->getDimension(2));
      m_hmin = hdim->getMinimum();
      m_kmin = kdim->getMinimum();
      m_lmin = ldim->getMinimum();
      m_hmax = hdim->getMaximum();
      m_kmax = kdim->getMaximum();
      m_lmax = ldim->getMaximum();
    }

    /**
     * Currently looks for the ConvertToMD algorithm in the history
     * @return A string donating the energy transfer mode of the input workspace
     */
    std::string SXDMDNorm::inputEnergyMode() const
    {
      const auto & hist = m_inputWS->getHistory();
      const size_t nalgs = hist.size();
      const auto & lastAlgorithm = hist.lastAlgorithm();

      std::string emode("");
      if(lastAlgorithm->name() == "ConvertToMD")
      {
        emode = lastAlgorithm->getPropertyValue("dEAnalysisMode");
      }
      else if ( (lastAlgorithm->name() == "Load" || hist.lastAlgorithm()->name() == "LoadMD") &&
                hist.getAlgorithmHistory(nalgs - 2)->name() == "ConvertToMD" )
      {
        //get dEAnalysisMode
        PropertyHistories histvec = hist.getAlgorithmHistory(nalgs - 2)->getProperties();
        for(auto it = histvec.begin(); it != histvec.end(); ++it)
        {
          if((*it)->name() == "dEAnalysisMode")
          {
            emode = (*it)->value();
            break;
          }
        }
      }
      else
      {
        throw std::invalid_argument("The last algorithm in the history of the input workspace is not ConvertToMD");
      }
      return emode;
    }

    /**
     * Runs the BinMD algorithm on the input to provide the output workspace
     * All slicing algorithm properties are passed along
     * @return MDHistoWorkspace as a result of the binning
     */
    MDHistoWorkspace_sptr SXDMDNorm::binInputWS()
    {
      const auto & props = getProperties();
      IAlgorithm_sptr binMD = createChildAlgorithm("BinMD", 0.0, 0.3);
      binMD->setPropertyValue("AxisAligned","1");
      for(auto it = props.begin(); it != props.end(); ++it)
      {
        const auto & propName = (*it)->name();
        if(propName != "FluxWorkspace" && propName != "SolidAngleWorkspace" &&
           propName != "OutputNormalizationWorkspace")
        {
          binMD->setPropertyValue(propName,(*it)->value());
        }
      }
      binMD->executeAsChildAlg();
      Workspace_sptr outputWS = binMD->getProperty("OutputWorkspace");
      return boost::dynamic_pointer_cast<MDHistoWorkspace>(outputWS);
    }
    
    /**
     * Create & cached the normalization workspace
     * @param dataWS The binned workspace that will be used for the data
     */
    void SXDMDNorm::createNormalizationWS(const MDHistoWorkspace &dataWS)
    {
      // Copy the MDHisto workspace, and change signals and errors to 0.
      m_normWS = boost::make_shared<MDHistoWorkspace>(dataWS);
      m_normWS->setTo(0.,0.,0.);
    }

    /**
     * Retrieve logged values from non-HKL dimensions
     * @param skipNormalization [InOut] Updated to false if any values are outside range measured by input workspace
     * @return A vector of values from other dimensions to be include in normalized MD position calculation
     */
    std::vector<coord_t> SXDMDNorm::getValuesFromOtherDimensions(bool &skipNormalization) const
    {
      const auto & runZero = m_inputWS->getExperimentInfo(0)->run();

      std::vector<coord_t> otherDimValues;
      for(size_t i = 3; i < m_inputWS->getNumDims(); i++)
      {
        const auto dimension = m_inputWS->getDimension(i);
        float dimMin = static_cast<float>(dimension->getMinimum());
        float dimMax = static_cast<float>(dimension->getMaximum());
        auto *dimProp = \
            dynamic_cast<Kernel::TimeSeriesProperty<double> *>(runZero.getProperty(dimension->getName()));
        if (dimProp)
        {
          coord_t value = static_cast<coord_t>(dimProp->firstValue());
          otherDimValues.push_back(value);
          //in the original MD data no time was spent measuring between dimMin and dimMax
          if (value < dimMin || value > dimMax)
          {
            skipNormalization = true;
          }
        }
      }
      return otherDimValues;
    }

    /**
     * Checks the normalization workspace against the indices of the original dimensions.
     * If not found, the corresponding dimension is integrated
     * @param otherDimValues Values from non-HKL dimensions
     * @param skipNormalization [InOut] Sets the flag true if normalization values are outside of original inputs
     * @return Affine trasform matrix
     */
    Kernel::Matrix<coord_t> SXDMDNorm::findIntergratedDimensions(const std::vector<coord_t> &otherDimValues, 
                                                                    bool &skipNormalization)
    {
      // Get indices of the original dimensions in the output workspace,
      // and if not found, the corresponding dimension is integrated
      Kernel::Matrix<coord_t> affineMat = m_normWS->getTransformFromOriginal(0)->makeAffineMatrix();

      const size_t nrm1 = affineMat.numRows() - 1;
      const size_t ncm1 = affineMat.numCols() - 1;
      for( size_t row = 0; row < nrm1; row++ ) //affine matrix, ignore last row
      {
        const auto dimen = m_normWS->getDimension(row);
        const auto dimMin(dimen->getMinimum()), dimMax(dimen->getMaximum());
        if(affineMat[row][0] == 1.)
        {
          m_hIntegrated = false;
          m_hIdx = row;
          if( m_hmin < dimMin ) m_hmin = dimMin;
          if( m_hmax > dimMax ) m_hmax = dimMax;
          if( m_hmin > dimMax || m_hmax < dimMin)
          {
            skipNormalization = true;
          }
        }
        if(affineMat[row][1] == 1.)
        {
          m_kIntegrated = false;
          m_kIdx = row;
          if( m_kmin < dimMin ) m_kmin = dimMin;
          if( m_kmax > dimMax ) m_kmax = dimMax;
          if( m_kmin > dimMax || m_kmax < dimMin )
          {
            skipNormalization = true;
          }
        }
        if(affineMat[row][2] == 1.)
        {
          m_lIntegrated = false;
          m_lIdx = row;
          if( m_lmin < dimMin ) m_lmin = dimMin;
          if( m_lmax > dimMax ) m_lmax = dimMax;
          if( m_lmin > dimMax || m_lmax < dimMin )
          {
            skipNormalization = true;
          }

          for(size_t col = 3; col < ncm1; col++) //affine matrix, ignore last column
          {
            if(affineMat[row][col] == 1.)
            {
              double val = otherDimValues.at(col - 3);
              if( val > dimMax || val < dimMin )
              {
                skipNormalization = true;
              }
            }
          }
        }
      }
      
      return affineMat;
    }

    /**
     * Calculate the points of intersection for the given detector with cuboid surrounding the
     * detector position in HKL
     * @param detector A pointer to a detector object
     * @return A list of intersections in HKL space
     */
    std::vector<Kernel::VMD> SXDMDNorm::calculateIntersections(const Geometry::IDetector_const_sptr & detector)
    {
      double th = detector->getTwoTheta(V3D(0,0,0),V3D(0,0,1));
      double phi = detector->getPhi();
      V3D q(-sin(th)*cos(phi),-sin(th)*sin(phi),1.-cos(th));
      q = m_rubw*q;
      double hStart = q.X()*m_kiMin,hEnd = q.X()*m_kiMax;
      double kStart = q.Y()*m_kiMin,kEnd = q.Y()*m_kiMax;
      double lStart = q.Z()*m_kiMin,lEnd = q.Z()*m_kiMax;
      
      double eps = 1e-7;
      
      auto hNBins = m_hX.size();
      auto kNBins = m_kX.size();
      auto lNBins = m_lX.size();
      std::vector<Kernel::VMD> intersections;
      intersections.reserve(hNBins + kNBins + lNBins + 8);
      
      //calculate intersections with planes perpendicular to h
      if (fabs(hStart-hEnd) > eps)
      {
        double fmom=(m_kiMax-m_kiMin)/(hEnd-hStart);
        double fk=(kEnd-kStart)/(hEnd-hStart);
        double fl=(lEnd-lStart)/(hEnd-hStart);
        if(!m_hIntegrated)
        {
          for(size_t i = 0;i<hNBins;i++)
          {
            double hi = m_hX[i];
            if((hi>=m_hmin)&&(hi<=m_hmax) && ((hStart-hi)*(hEnd-hi)<0))
            {
              // if hi is between hStart and hEnd, then ki and li will be between kStart, kEnd and lStart, lEnd and momi will be between m_kiMin and KnincidemtmMax
              double ki = fk*(hi-hStart)+kStart;
              double li = fl*(hi-hStart)+lStart;
              if ((ki>=m_kmin)&&(ki<=m_kmax)&&(li>=m_lmin)&&(li<=m_lmax))
              {
                double momi = fmom*(hi-hStart)+m_kiMin;
                Mantid::Kernel::VMD v(hi,ki,li,momi);
                intersections.push_back(v);
              }
            }
          }
        }
        
        double momhMin = fmom*(m_hmin-hStart)+m_kiMin;
        if ((momhMin>m_kiMin)&&(momhMin<m_kiMax))
        {
          //khmin and lhmin
          double khmin = fk*(m_hmin-hStart)+kStart;
          double lhmin = fl*(m_hmin-hStart)+lStart;
          if((khmin>=m_kmin)&&(khmin<=m_kmax)&&(lhmin>=m_lmin)&&(lhmin<=m_lmax))
          {
            Mantid::Kernel::VMD v(m_hmin,khmin,lhmin,momhMin);
            intersections.push_back(v);
          }
        }
        double momhMax = fmom*(m_hmax-hStart)+m_kiMin;
        if ((momhMax>m_kiMin)&&(momhMax<m_kiMax))
        {
          //khmax and lhmax
          double khmax = fk*(m_hmax-hStart)+kStart;
          double lhmax = fl*(m_hmax-hStart)+lStart;
          if((khmax>=m_kmin)&&(khmax<=m_kmax)&&(lhmax>=m_lmin)&&(lhmax<=m_lmax))
          {
            Mantid::Kernel::VMD v(m_hmax,khmax,lhmax,momhMax);
            intersections.push_back(v);
          }
        }
      }
      
      //calculate intersections with planes perpendicular to k
      if (fabs(kStart-kEnd)>eps)
      {
        double fmom=(m_kiMax-m_kiMin)/(kEnd-kStart);
        double fh=(hEnd-hStart)/(kEnd-kStart);
        double fl=(lEnd-lStart)/(kEnd-kStart);
        if(!m_kIntegrated)
        {
          for(size_t i = 0;i<kNBins;i++)
          {
            double ki = m_kX[i];
            if ((ki>=m_kmin)&&(ki<=m_kmax)&&((kStart-ki)*(kEnd-ki)<0))
            {
              // if ki is between kStart and kEnd, then hi and li will be between hStart, hEnd and lStart, lEnd
              double hi = fh*(ki-kStart)+hStart;
              double li = fl*(ki-kStart)+lStart;
              if ((hi>=m_hmin)&&(hi<=m_hmax)&&(li>=m_lmin)&&(li<=m_lmax))
              {
                double momi = fmom*(ki-kStart)+m_kiMin;
                Mantid::Kernel::VMD v(hi,ki,li,momi);
                intersections.push_back(v);
              }
            }
          }
        }
        
        double momkMin = fmom*(m_kmin-kStart)+m_kiMin;
        if ((momkMin>m_kiMin)&&(momkMin<m_kiMax))
        {
          //hkmin and lkmin
          double hkmin = fh*(m_kmin-kStart)+hStart;
          double lkmin = fl*(m_kmin-kStart)+lStart;
          if((hkmin>=m_hmin)&&(hkmin<=m_hmax)&&(lkmin>=m_lmin)&&(lkmin<=m_lmax))
          {
            Mantid::Kernel::VMD v(hkmin,m_kmin,lkmin,momkMin);
            intersections.push_back(v);
          }
        }
        double momkMax = fmom*(m_kmax-kStart)+m_kiMin;
        if ((momkMax>m_kiMin)&&(momkMax<m_kiMax))
        {
          //hkmax and lkmax
          double hkmax = fh*(m_kmax-kStart)+hStart;
          double lkmax = fl*(m_kmax-kStart)+lStart;
          if((hkmax>=m_hmin)&&(hkmax<=m_hmax)&&(lkmax>=m_lmin)&&(lkmax<=m_lmax))
          {
            Mantid::Kernel::VMD v(hkmax,m_kmax,lkmax,momkMax);
            intersections.push_back(v);
          }
        }
      }
      
      //calculate intersections with planes perpendicular to l
      if (fabs(lStart-lEnd)>eps)
      {
        double fmom=(m_kiMax-m_kiMin)/(lEnd-lStart);
        double fh=(hEnd-hStart)/(lEnd-lStart);
        double fk=(kEnd-kStart)/(lEnd-lStart);
        if(!m_lIntegrated)
        {
          for(size_t i = 0;i<lNBins;i++)
          {
            double li = m_lX[i];
            if ((li>=m_lmin)&&(li<=m_lmax)&&((lStart-li)*(lEnd-li)<0))
            {
              // if li is between lStart and lEnd, then hi and ki will be between hStart, hEnd and kStart, kEnd
              double hi = fh*(li-lStart)+hStart;
              double ki = fk*(li-lStart)+kStart;
              if ((hi>=m_hmin)&&(hi<=m_hmax)&&(ki>=m_kmin)&&(ki<=m_kmax))
              {
                double momi = fmom*(li-lStart)+m_kiMin;
                Mantid::Kernel::VMD v(hi,ki,li,momi);
                intersections.push_back(v);
              }
            }
          }
        }
        
        double momlMin = fmom*(m_lmin-lStart)+m_kiMin;
        if ((momlMin>m_kiMin)&&(momlMin<m_kiMax))
        {
          //hlmin and klmin
          double hlmin = fh*(m_lmin-lStart)+hStart;
          double klmin = fk*(m_lmin-lStart)+kStart;
          if((hlmin>=m_hmin)&&(hlmin<=m_hmax)&&(klmin>=m_kmin)&&(klmin<=m_kmax))
          {
            Mantid::Kernel::VMD v(hlmin,klmin,m_lmin,momlMin);
            intersections.push_back(v);
          }
        }
        double momlMax = fmom*(m_lmax-lStart)+m_kiMin;
        if ((momlMax>m_kiMin)&&(momlMax<m_kiMax))
        {
          //khmax and lhmax
          double hlmax = fh*(m_lmax-lStart)+hStart;
          double klmax = fk*(m_lmax-lStart)+kStart;
          if((hlmax>=m_hmin)&&(hlmax<=m_hmax)&&(klmax>=m_kmin)&&(klmax<=m_kmax))
          {
            Mantid::Kernel::VMD v(hlmax,klmax,m_lmax,momlMax);
            intersections.push_back(v);
          }
        }
      }
      
      //add endpoints
      if ((hStart>=m_hmin)&&(hStart<=m_hmax)&&(kStart>=m_kmin)&&(kStart<=m_kmax)&&(lStart>=m_lmin)&&(lStart<=m_lmax))
      {
        Mantid::Kernel::VMD v(hStart,kStart,lStart,m_kiMin);
        intersections.push_back(v);
      }
      if ((hEnd>=m_hmin)&&(hEnd<=m_hmax)&&(kEnd>=m_kmin)&&(kEnd<=m_kmax)&&(lEnd>=m_lmin)&&(lEnd<=m_lmax))
      {
        Mantid::Kernel::VMD v(hEnd,kEnd,lEnd,m_kiMax);
        intersections.push_back(v);
      }
      
      //sort intersections by momentum
      typedef std::vector<Mantid::Kernel::VMD>::iterator IterType;
      std::stable_sort<IterType,bool (*)(const Mantid::Kernel::VMD&,const Mantid::Kernel::VMD&)>(intersections.begin(),intersections.end(),compareMomentum);
      
      return intersections;
    }
    
  } // namespace MDAlgorithms
} // namespace Mantid
