#include "MantidMDAlgorithms/SXDMDNorm.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid
{
namespace MDAlgorithms
{

  using Mantid::Kernel::Direction;
  using Mantid::API::WorkspaceProperty;
  using namespace Mantid::MDEvents;
  using namespace Mantid::API;
  using namespace Mantid::Kernel;

  namespace{

  ///function to  compare two intersections (h,k,l,Momentum) by Momentum
  bool compareMomentum(const Mantid::Kernel::VMD &v1, const Mantid::Kernel::VMD &v2)
  {
    return (v1[3]<v2[3]);
  }

  // size of the interpolation tables for the integrated flux spectra
  const size_t interpolationSize = 1000;

  }

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SXDMDNorm)



  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SXDMDNorm::SXDMDNorm()
  {
      hIndex=-1;
      kIndex=-1;
      lIndex=-1;
      hIntegrated=true;
      kIntegrated=true;
      lIntegrated=true;
      m_nDims=0;
      transf=Mantid::Kernel::DblMatrix(3,3);
      KincidentMin=0;
      KincidentMax=EMPTY_DBL();
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SXDMDNorm::~SXDMDNorm()
  {
  }


  //----------------------------------------------------------------------------------------------


  /// Algorithm's version for identification. @see Algorithm::version
  int SXDMDNorm::version() const { return 1;}

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string SXDMDNorm::category() const { return "MDAlgorithms";}

  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string SXDMDNorm::summary() const { return "Calculate normalization for an MDEvent workspace for single crystal diffraction.";}

  /// Algorithm's name for use in the GUI and help. @see Algorithm::name
  const std::string SXDMDNorm::name() const { return "SXDMDNorm";}

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SXDMDNorm::init()
  {
      declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::Input), "An input MDWorkspace.");

      std::string dimChars = getDimensionChars();
      // --------------- Axis-aligned properties ---------------------------------------
      for (size_t i=0; i<dimChars.size(); i++)
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

      declareProperty(new WorkspaceProperty<>("FluxWorkspace","",Direction::Input,wsValidator), "An input workspace containing momentum dependent flux.");
      declareProperty(new WorkspaceProperty<>("SolidAngleWorkspace","",Direction::Input,wsValidator->clone()), "An input workspace containing momentum integrated vanadium (a measure of the solid angle).");

      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output), "A name for the output data MDHistoWorkspace.");
      declareProperty(new WorkspaceProperty<Workspace>("OutputNormalizationWorkspace","",Direction::Output), "A name for the output normalization MDHistoWorkspace.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SXDMDNorm::exec()
  {
      bool skipProcessing=false;
      m_inputWS=getProperty("InputWorkspace");
      WorkspaceHistory hist=m_inputWS->getHistory();
      std::string dEMode("");
      if (hist.lastAlgorithm()->name()=="ConvertToMD")
      {
          dEMode=hist.lastAlgorithm()->getPropertyValue("dEAnalysisMode");
      }
      else if (((hist.lastAlgorithm()->name()=="Load")||(hist.lastAlgorithm()->name()=="LoadMD"))&&(hist.getAlgorithmHistory(hist.size()-2)->name()=="ConvertToMD"))
      {
          //get dEAnalysisMode
          PropertyHistories histvec=hist.getAlgorithmHistory(hist.size()-2)->getProperties();
          for(auto it=histvec.begin();it!=histvec.end();++it)
          {
             if((*it)->name()=="dEAnalysisMode")
             {
                 dEMode=(*it)->value();
             }
          }
      }
      else
      {
          throw std::runtime_error("The last algorithm in the history of the input workspace is not ConvertToMD");
      }
      if (dEMode!="Elastic")
      {
          throw std::runtime_error("This is not elastic scattering data");
      }
      hMin=m_inputWS->getDimension(0)->getMinimum();
      kMin=m_inputWS->getDimension(1)->getMinimum();
      lMin=m_inputWS->getDimension(2)->getMinimum();
      hMax=m_inputWS->getDimension(0)->getMaximum();
      kMax=m_inputWS->getDimension(1)->getMaximum();
      lMax=m_inputWS->getDimension(2)->getMaximum();

      //initialize some variables
      hIntegrated=true;
      kIntegrated=true;
      lIntegrated=true;
      hIndex=-1;
      kIndex=-1;
      lIndex=-1;

      //check for other dimensions if we could measure anything in the original data
      std::vector<coord_t> otherValues;
      for(size_t i=3;i<m_inputWS->getNumDims();i++)
      {
          float dimMin=static_cast<float>(m_inputWS->getDimension(i)->getMinimum());
          float dimMax=static_cast<float>(m_inputWS->getDimension(i)->getMaximum());
          Kernel::TimeSeriesProperty<double> *run_property = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(m_inputWS->getExperimentInfo(0)->run().getProperty(m_inputWS->getDimension(i)->getName()));
          if (run_property!=NULL)
          {
            coord_t value=static_cast<coord_t>(run_property->firstValue());
            otherValues.push_back(value);
            //in the original MD data no time was spent measuring between dimMin and dimMax
            if ((value<dimMin)||(value>dimMax))
            {
              skipProcessing=true;
            }
          }
          delete run_property;
      }

      // Run BinMD
      Workspace_sptr outputWS = getProperty("OutputWorkspace");
      auto props=getProperties();
      IAlgorithm_sptr bin = createChildAlgorithm("BinMD",0.0,0.3);
      bin->setPropertyValue("AxisAligned","1");
      for(auto it=props.begin();it!=props.end();++it)
      {
          if(((*it)->name()!="FluxWorkspace")&&((*it)->name()!="SolidAngleWorkspace")&&((*it)->name()!="OutputNormalizationWorkspace"))
              bin->setPropertyValue((*it)->name(),(*it)->value());
      }
      bin->executeAsChildAlg();
      outputWS=bin->getProperty("OutputWorkspace");
      this->setProperty("OutputWorkspace", outputWS);

      //copy the MDHisto workspace, and change signals and errors to 0.
      m_normWS=MDHistoWorkspace_sptr(new MDHistoWorkspace(*(boost::dynamic_pointer_cast<MDHistoWorkspace>(outputWS))));
      m_normWS->setTo(0.,0.,0.);

      //get indices of the original dimensions in the output workspace, and if not found, the corresponding dimension is integrated
      Mantid::Kernel::Matrix<coord_t> mat=m_normWS->getTransformFromOriginal(0)->makeAffineMatrix();

      for (size_t row=0; row<mat.numRows()-1; row++)//affine matrix, ignore last row
      {
          if(mat[row][0]==1.)
          {
              hIntegrated=false;
              hIndex=row;
              if(hMin<m_normWS->getDimension(row)->getMinimum()) hMin=m_normWS->getDimension(row)->getMinimum();
              if(hMax>m_normWS->getDimension(row)->getMaximum()) hMax=m_normWS->getDimension(row)->getMaximum();
              if((hMin>m_normWS->getDimension(row)->getMaximum())||(hMax<m_normWS->getDimension(row)->getMinimum()))
              {
                  skipProcessing=true;
              }
          }
          if(mat[row][1]==1.)
          {
              kIntegrated=false;
              kIndex=row;
              if(kMin<m_normWS->getDimension(row)->getMinimum()) kMin=m_normWS->getDimension(row)->getMinimum();
              if(kMax>m_normWS->getDimension(row)->getMaximum()) kMax=m_normWS->getDimension(row)->getMaximum();
              if((kMin>m_normWS->getDimension(row)->getMaximum())||(kMax<m_normWS->getDimension(row)->getMinimum()))
              {
                  skipProcessing=true;
              }
          }
          if(mat[row][2]==1.)
          {
              lIntegrated=false;
              lIndex=row;
              if(lMin<m_normWS->getDimension(row)->getMinimum()) lMin=m_normWS->getDimension(row)->getMinimum();
              if(lMax>m_normWS->getDimension(row)->getMaximum()) lMax=m_normWS->getDimension(row)->getMaximum();
              if((lMin>m_normWS->getDimension(row)->getMaximum())||(lMax<m_normWS->getDimension(row)->getMinimum()))
              {
                  skipProcessing=true;
              }

              for(size_t col=3;col<mat.numCols()-1;col++) //affine matrix, ignore last column
              {
                  if(mat[row][col]==1.)
                  {
                      double val=otherValues.at(col-3);
                      if((val>m_normWS->getDimension(row)->getMaximum())||(val<m_normWS->getDimension(row)->getMinimum()))
                      {
                          skipProcessing=true;
                      }
                  }
              }
          }
      }

      auto &hDim = *m_normWS->getDimension(hIndex);
      m_hX.resize( hDim.getNBins() );
      for(size_t i = 0; i < m_hX.size(); ++i)
      {
        m_hX[i] = hDim.getX(i);
      }

      auto &kDim = *m_normWS->getDimension(kIndex);
      m_kX.resize( kDim.getNBins() );
      for(size_t i = 0; i < m_kX.size(); ++i)
      {
        m_kX[i] = kDim.getX(i);
      }

      auto &lDim = *m_normWS->getDimension(lIndex);
      m_lX.resize( lDim.getNBins() );
      for(size_t i = 0; i < m_lX.size(); ++i)
      {
        m_lX[i] = lDim.getX(i);
      }

      Mantid::API::MatrixWorkspace_const_sptr fW=getProperty("FluxWorkspace");
      Mantid::DataObjects::EventWorkspace_const_sptr fluxW = boost::dynamic_pointer_cast<const Mantid::DataObjects::EventWorkspace>(fW);
      KincidentMin=fluxW->getEventXMin();
      KincidentMax=fluxW->getEventXMax();
      Mantid::API::MatrixWorkspace_const_sptr sA=getProperty("SolidAngleWorkspace");

      // a workspace to store integrated flux spectra
      // the spectra sizes are smaller than in fW for efficeiency
      // actual integrals are linearly interpolated between points in integrFlux
      auto integrFlux = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
        API::WorkspaceFactory::Instance().create("Workspace2D", fW->getNumberHistograms(),interpolationSize,interpolationSize) );

      // integrate spectra in flixW and store the results in integrFlux
      integrateFlux( *fluxW, *integrFlux );

      if (skipProcessing)
      {
          g_log.warning("Binning limits are outside the limits of the MDWorkspace\n");
      }
      else
      {
          double PC=m_normWS->getExperimentInfo(0)->run().getProtonCharge();
          Kernel::PropertyWithValue< std::vector<double> > *prop=dynamic_cast< Mantid::Kernel::PropertyWithValue<std::vector<double> >*>(m_normWS->getExperimentInfo(0)->getLog("RUBW_MATRIX"));
          if (prop==NULL)
          {
              throw std::runtime_error("No RUBW_MATRIX");
          }
          else
          {
              Mantid::Kernel::DblMatrix RUBW((*prop)()); //includes the 2*pi factor but not goniometer for now :)
              transf=m_normWS->getExperimentInfo(0)->run().getGoniometerMatrix()*RUBW;
              transf.Invert();
          }
          //FIXME: the detector positions are from the IDF. Need to account for calibration
          std::vector<detid_t> detIDS=m_normWS->getExperimentInfo(0)->getInstrument()->getDetectorIDs(true);

          Mantid::API::Progress *prog=new Mantid::API::Progress(this,0.3,1,static_cast<int64_t>(detIDS.size()));
          const detid2index_map d2m=fluxW->getDetectorIDToWorkspaceIndexMap();
          const detid2index_map d2mSA=sA->getDetectorIDToWorkspaceIndexMap();
          auto instrument = m_normWS->getExperimentInfo(0)->getInstrument();

          PARALLEL_FOR1(m_normWS)
          for(int i=0;i<static_cast<int>(detIDS.size());i++)
          {
              PARALLEL_START_INTERUPT_REGION
              Mantid::Geometry::IDetector_const_sptr detector=instrument->getDetector(detIDS[i]);
              if(!detector->isMonitor()&&!detector->isMasked())
              {
                  //get intersections
                  std::vector<Mantid::Kernel::VMD> intersections=calculateIntersections(detector);

                  if(!intersections.empty())
                  {
                      //calculate indices
                      //add to the correct signal at that particular index
                      //NOTE: if parallel it has to be atomic/critical

                      //get flux spectrum number
                      size_t sp=d2m.find(detIDS[i])->second;
                      // get the solid angle
                      double solid=sA->readY(d2mSA.find(detIDS[i])->second)[0]*PC;

                      const size_t sizeOfMVD = intersections.front().size();
                      // pre-allocate for efficiency
                      std::vector<coord_t> pos( sizeOfMVD + otherValues.size() );

                      auto intersectionsBegin = intersections.begin();

                      // calculate integrals for the intersection

                      // momentum values at intersections
                      std::vector<double> xValues( intersections.size() );
                      // buffer for the integrals
                      std::vector<double> yValues( intersections.size() );
                      {
                        // copy momenta to xValues
                        auto x = xValues.begin();
                        for (auto it = intersectionsBegin; it != intersections.end(); ++it, ++x)
                        {
                          *x = (*it)[3];
                        }
                      }
                      // calculate integrals at momenta from xValues by interpolating between points in spectrum sp
                      // of workspace integrFlux. The result is stored in yValues
                      calcIntegralsForIntersections( xValues, *integrFlux, sp, yValues );

                      for (auto it = intersectionsBegin + 1; it != intersections.end(); ++it)
                      {
                          //Mantid::Kernel::VMD deltav=(*it)-(*(it-1));//difference between consecutive intersections
                          // the full vector isn't used so compute only what is necessary
                          const double xStart = (*(it-1))[3];
                          const double xEnd = (*it)[3];
                          const double delta = xEnd - xStart;
                          const double eps=1e-7;//do not integrate if momemntum difference is smaller than eps, assume contribution is 0

                          if (delta > eps)
                          {
                              //Mantid::Kernel::VMD avev=((*it)+(*(it-1)))*0.5;//average between two intersection (to get position)
                              //std::vector<coord_t> pos=avev.toVector<coord_t>();
                              //pos.insert(pos.end()-1,otherValues.begin(),otherValues.end());
                              // a bit longer and less readable but faster version of the above
                              std::transform( it->getBareArray(), it->getBareArray() + sizeOfMVD, (it-1)->getBareArray(), pos.begin(), std::plus<coord_t>() );
                              std::transform( pos.begin(), pos.begin() + sizeOfMVD, pos.begin(), std::bind2nd( std::multiplies<coord_t>(), 0.5 ) );
                              std::copy( otherValues.begin(), otherValues.end(), pos.begin() + sizeOfMVD );

                              std::vector<coord_t> posNew = mat*pos;
                              size_t linIndex=m_normWS->getLinearIndexAtCoord(posNew.data());

                              if(linIndex!=size_t(-1))
                              {
                                  // index of the current intersection
                                  size_t k = static_cast<size_t>( std::distance( intersectionsBegin, it ) );
                                  // signal = integral between two consecutive intersections
                                  double signal = yValues[k] - yValues[k - 1];
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


  std::vector<Mantid::Kernel::VMD> SXDMDNorm::calculateIntersections(Mantid::Geometry::IDetector_const_sptr detector)
  {
        std::vector<Mantid::Kernel::VMD> intersections;
        double th=detector->getTwoTheta(V3D(0,0,0),V3D(0,0,1));
        double phi=detector->getPhi();
        V3D q(-sin(th)*cos(phi),-sin(th)*sin(phi),1.-cos(th));
        q=transf*q;
        double hStart=q.X()*KincidentMin,hEnd=q.X()*KincidentMax;
        double kStart=q.Y()*KincidentMin,kEnd=q.Y()*KincidentMax;
        double lStart=q.Z()*KincidentMin,lEnd=q.Z()*KincidentMax;

        double eps=1e-7;

        auto hNBins = m_hX.size();
        auto kNBins = m_kX.size();
        auto lNBins = m_lX.size();
        intersections.reserve( hNBins + kNBins + lNBins + 8 );

        //calculate intersections with planes perpendicular to h
        if (fabs(hStart-hEnd)>eps)
        {
            double fmom=(KincidentMax-KincidentMin)/(hEnd-hStart);
            double fk=(kEnd-kStart)/(hEnd-hStart);
            double fl=(lEnd-lStart)/(hEnd-hStart);
            if(!hIntegrated)
            {
              for(size_t i=0;i<hNBins;i++)
              {
                double hi = m_hX[i];
                if ((hi>=hMin)&&(hi<=hMax)&&((hStart-hi)*(hEnd-hi)<0))
                {
                    // if hi is between hStart and hEnd, then ki and li will be between kStart, kEnd and lStart, lEnd and momi will be between KincidentMin and KnincidemtmMax
                    double ki=fk*(hi-hStart)+kStart;
                    double li=fl*(hi-hStart)+lStart;
                    if ((ki>=kMin)&&(ki<=kMax)&&(li>=lMin)&&(li<=lMax))
                    {
                        double momi=fmom*(hi-hStart)+KincidentMin;
                        Mantid::Kernel::VMD v(hi,ki,li,momi);
                        intersections.push_back(v);
                    }
                }
              }
            }

            double momhMin=fmom*(hMin-hStart)+KincidentMin;
            if ((momhMin>KincidentMin)&&(momhMin<KincidentMax))
            {
                //khmin and lhmin
                double khmin=fk*(hMin-hStart)+kStart;
                double lhmin=fl*(hMin-hStart)+lStart;
                if((khmin>=kMin)&&(khmin<=kMax)&&(lhmin>=lMin)&&(lhmin<=lMax))
                {
                    Mantid::Kernel::VMD v(hMin,khmin,lhmin,momhMin);
                    intersections.push_back(v);
                }
            }
            double momhMax=fmom*(hMax-hStart)+KincidentMin;
            if ((momhMax>KincidentMin)&&(momhMax<KincidentMax))
            {
                //khmax and lhmax
                double khmax=fk*(hMax-hStart)+kStart;
                double lhmax=fl*(hMax-hStart)+lStart;
                if((khmax>=kMin)&&(khmax<=kMax)&&(lhmax>=lMin)&&(lhmax<=lMax))
                {
                    Mantid::Kernel::VMD v(hMax,khmax,lhmax,momhMax);
                    intersections.push_back(v);
                }
            }
        }

        //calculate intersections with planes perpendicular to k
        if (fabs(kStart-kEnd)>eps)
        {
            double fmom=(KincidentMax-KincidentMin)/(kEnd-kStart);
            double fh=(hEnd-hStart)/(kEnd-kStart);
            double fl=(lEnd-lStart)/(kEnd-kStart);
            if(!kIntegrated)
            {
              for(size_t i=0;i<kNBins;i++)
              {
                double ki = m_kX[i];
                if ((ki>=kMin)&&(ki<=kMax)&&((kStart-ki)*(kEnd-ki)<0))
                {
                    // if ki is between kStart and kEnd, then hi and li will be between hStart, hEnd and lStart, lEnd
                    double hi=fh*(ki-kStart)+hStart;
                    double li=fl*(ki-kStart)+lStart;
                    if ((hi>=hMin)&&(hi<=hMax)&&(li>=lMin)&&(li<=lMax))
                    {
                        double momi=fmom*(ki-kStart)+KincidentMin;
                        Mantid::Kernel::VMD v(hi,ki,li,momi);
                        intersections.push_back(v);
                    }
                }
              }
            }

            double momkMin=fmom*(kMin-kStart)+KincidentMin;
            if ((momkMin>KincidentMin)&&(momkMin<KincidentMax))
            {
                //hkmin and lkmin
                double hkmin=fh*(kMin-kStart)+hStart;
                double lkmin=fl*(kMin-kStart)+lStart;
                if((hkmin>=hMin)&&(hkmin<=hMax)&&(lkmin>=lMin)&&(lkmin<=lMax))
                {
                    Mantid::Kernel::VMD v(hkmin,kMin,lkmin,momkMin);
                    intersections.push_back(v);
                }
            }
            double momkMax=fmom*(kMax-kStart)+KincidentMin;
            if ((momkMax>KincidentMin)&&(momkMax<KincidentMax))
            {
                //hkmax and lkmax
                double hkmax=fh*(kMax-kStart)+hStart;
                double lkmax=fl*(kMax-kStart)+lStart;
                if((hkmax>=hMin)&&(hkmax<=hMax)&&(lkmax>=lMin)&&(lkmax<=lMax))
                {
                    Mantid::Kernel::VMD v(hkmax,kMax,lkmax,momkMax);
                    intersections.push_back(v);
                }
            }
        }

        //calculate intersections with planes perpendicular to l
        if (fabs(lStart-lEnd)>eps)
        {
            double fmom=(KincidentMax-KincidentMin)/(lEnd-lStart);
            double fh=(hEnd-hStart)/(lEnd-lStart);
            double fk=(kEnd-kStart)/(lEnd-lStart);
            if(!lIntegrated)
            {
              for(size_t i=0;i<lNBins;i++)
              {
                double li = m_lX[i];
                if ((li>=lMin)&&(li<=lMax)&&((lStart-li)*(lEnd-li)<0))
                {
                    // if li is between lStart and lEnd, then hi and ki will be between hStart, hEnd and kStart, kEnd
                    double hi=fh*(li-lStart)+hStart;
                    double ki=fk*(li-lStart)+kStart;
                    if ((hi>=hMin)&&(hi<=hMax)&&(ki>=kMin)&&(ki<=kMax))
                    {
                        double momi=fmom*(li-lStart)+KincidentMin;
                        Mantid::Kernel::VMD v(hi,ki,li,momi);
                        intersections.push_back(v);
                    }
                }
              }
            }

            double momlMin=fmom*(lMin-lStart)+KincidentMin;
            if ((momlMin>KincidentMin)&&(momlMin<KincidentMax))
            {
                //hlmin and klmin
                double hlmin=fh*(lMin-lStart)+hStart;
                double klmin=fk*(lMin-lStart)+kStart;
                if((hlmin>=hMin)&&(hlmin<=hMax)&&(klmin>=kMin)&&(klmin<=kMax))
                {
                    Mantid::Kernel::VMD v(hlmin,klmin,lMin,momlMin);
                    intersections.push_back(v);
                }
            }
            double momlMax=fmom*(lMax-lStart)+KincidentMin;
            if ((momlMax>KincidentMin)&&(momlMax<KincidentMax))
            {
                //khmax and lhmax
                double hlmax=fh*(lMax-lStart)+hStart;
                double klmax=fk*(lMax-lStart)+kStart;
                if((hlmax>=hMin)&&(hlmax<=hMax)&&(klmax>=kMin)&&(klmax<=kMax))
                {
                    Mantid::Kernel::VMD v(hlmax,klmax,lMax,momlMax);
                    intersections.push_back(v);
                }
            }
        }

        //add endpoints
        if ((hStart>=hMin)&&(hStart<=hMax)&&(kStart>=kMin)&&(kStart<=kMax)&&(lStart>=lMin)&&(lStart<=lMax))
        {
            Mantid::Kernel::VMD v(hStart,kStart,lStart,KincidentMin);
            intersections.push_back(v);
        }
        if ((hEnd>=hMin)&&(hEnd<=hMax)&&(kEnd>=kMin)&&(kEnd<=kMax)&&(lEnd>=lMin)&&(lEnd<=lMax))
        {
            Mantid::Kernel::VMD v(hEnd,kEnd,lEnd,KincidentMax);
            intersections.push_back(v);
        }

        //sort intersections by momentum
        typedef std::vector<Mantid::Kernel::VMD>::iterator IterType;
        std::stable_sort<IterType,bool (*)(const Mantid::Kernel::VMD&,const Mantid::Kernel::VMD&)>(intersections.begin(),intersections.end(),compareMomentum);

        return intersections;
  }

  /**
   * Integrate spectra in flux at x-values in integrFlux and save the results in y-vectors of integrFlux.
   * @param flux :: A workspace to integrate.
   * @param integrFlux :: A workspace to store the results.
   */
  void SXDMDNorm::integrateFlux( const DataObjects::EventWorkspace& flux, API::MatrixWorkspace &integrFlux )
  {
    size_t nSpec = flux.getNumberHistograms();
    assert( nSpec == integrFlux.getNumberHistograms() );

    // claculate the integration points and save them in the x-vactors of integrFlux
    double xMin = flux.getEventXMin();
    double xMax = flux.getEventXMax();
    double dx = ( xMax - xMin ) / ( integrFlux.blocksize() - 1 );
    auto &X = integrFlux.dataX(0);
    auto ix = X.begin();
    // x-values are equally spaced between the min and max tof in the first flux spectrum
    for(double x = xMin; ix != X.end(); ++ix, x += dx)
    {
      *ix = x;
    }

    // loop overr the spectra and integrate
    for(size_t sp = 0; sp < nSpec; ++sp)
    {
      if ( sp > 0 )
      {
        integrFlux.setX(sp,X);
      }
      std::vector<Mantid::DataObjects::WeightedEventNoTime> el = flux.getEventList(sp).getWeightedEventsNoTime();
      auto &outY = integrFlux.dataY(sp);
      double sum = 0;
      auto x = X.begin() + 1;
      size_t i = 1;
      // the integral is a running sum of the event weights in the spectrum
      for(auto evnt = el.begin(); evnt != el.end(); ++evnt)
      {
        double tof = evnt->tof();
        while( x != X.end() && *x < tof )
        {
          ++x; ++i;
        }
        if ( x == X.end() ) break;
        sum += evnt->weight();
        outY[i] = sum;
      }
    }
  }

  /**
   * LInearly interpolate between the points in integrFlux at xValues and save the results in yValues.
   * @param xValues :: X-values at which to interpolate
   * @param integrFlux :: A workspace with the spectra to interpolate
   * @param sp :: A workspace index for a spectrum in integrFlux to interpolate.
   * @param yValues :: A vector to save the results.
   */
  void SXDMDNorm::calcIntegralsForIntersections( const std::vector<double> &xValues, const API::MatrixWorkspace &integrFlux, size_t sp, std::vector<double> &yValues ) const
  {
    assert( xValues.size() == yValues.size() );

    // the x-data from the workspace
    auto &xData = integrFlux.readX(sp);
    const double xStart = xData.front();
    const double xEnd = xData.back();

    // the values in integrFlux are expected to be integrals of a non-negative function
    // ie they must make a non-decreasing function
    auto &yData = integrFlux.readY(sp);
    size_t spSize = yData.size();

    const double yMin = 0.0;
    const double yMax = yData.back();

    size_t nData = xValues.size();
    // all integrals below xStart must be 0
    if (xValues[nData-1] < xStart)
    {
      std::fill( yValues.begin(), yValues.end(), yMin );
      return;
    }

    // all integrals above xEnd must be equal tp yMax
    if ( xValues[0] > xEnd )
    {
      std::fill( yValues.begin(), yValues.end(), yMax );
      return;
    }

    size_t i = 0;
    // integrals below xStart must be 0
    while(i < nData - 1 && xValues[i] < xStart)
    {
      yValues[i] = yMin;
      i++;
    }
    size_t j = 0;
    for(;i<nData;i++)
    {
      // integrals above xEnd must be equal tp yMax
      if (j >= spSize - 1)
      {
        yValues[i] = yMax;
      }
      else
      {
        double xi = xValues[i];
        while(j < spSize - 1 && xi > xData[j]) j++;
        // if x falls onto an interpolation point return the corresponding y
        if (xi == xData[j])
        {
          yValues[i] = yData[j];
        }
        else if (j == spSize - 1)
        {
          // if we get above xEnd it's yMax
          yValues[i] = yMax;
        }
        else if (j > 0)
        {
          // interpolate between the consecutive points
          double x0 = xData[j-1];
          double x1 = xData[j];
          double y0 = yData[j-1];
          double y1 = yData[j];
          yValues[i] = y0 + (y1 - y0)*(xi - x0)/(x1 - x0);
        }
        else // j == 0
        {
          yValues[i] = yMin;
        }
      }
    }

  }

} // namespace MDAlgorithms
} // namespace Mantid
