/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidMDAlgorithms/SXDMDNorm.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidMDEvents/CoordTransformAffineParser.h"
#include "MantidMDEvents/CoordTransformAligned.h"
#include "MantidMDEvents/MDBoxBase.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/BinMD.h"
#include <boost/algorithm/string.hpp>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Element.h>
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidMDEvents/CoordTransformAffine.h"
#include "MantidKernel/TimeSeriesProperty.h"
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::Kernel;
namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SXDMDNorm)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SXDMDNorm::SXDMDNorm()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SXDMDNorm::~SXDMDNorm()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string SXDMDNorm::name() const { return "SXDMDNorm";}
  
  /// Algorithm's version for identification. @see Algorithm::version
  int SXDMDNorm::version() const { return 1;}
  
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string SXDMDNorm::category() const { return "MDAlgorithms";}

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SXDMDNorm::initDocs()
  {
    this->setWikiSummary("TODO: Enter a quick description of your algorithm.");
    this->setOptionalMessage("TODO: Enter a quick description of your algorithm.");
  }

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
/*
      auto wsValidator = boost::make_shared<CompositeValidator>();
      wsValidator->add<WorkspaceUnitValidator>("Momentum");
      wsValidator->add<InstrumentValidator>();
      wsValidator->add<CommonBinsValidator>();

      declareProperty(new WorkspaceProperty<>("FluxWorkspace","",Direction::Input,wsValidator), "An input workspace containing momentum dependent flux.");
      declareProperty(new WorkspaceProperty<>("SolidAngleWorkspace","",Direction::Input,wsValidator->clone()), "An input workspace containing momentum integrated vanadium (a measure of the solid angle).");
*/
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
      else if (((hist.lastAlgorithm()->name()=="Load")||(hist.lastAlgorithm()->name()=="LoadMD"))&&(hist.getAlgorithmHistory(hist.size()-2).name()=="ConvertToMD"))
      {
          //get deanaluysisMode
          std::vector<Kernel::PropertyHistory> histvec=hist.getAlgorithmHistory(hist.size()-2).getProperties();
          for(std::vector<Kernel::PropertyHistory>::iterator it=histvec.begin();it!=histvec.end();++it)
          {
             if((*it).name()=="dEAnalysisMode")
             {
                 dEMode=(*it).value();
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
      std::vector<double> otherValues;
      for(size_t i=3;i<m_inputWS->getNumDims();i++)
      {
          float dimMin=static_cast<float>(m_inputWS->getDimension(i)->getMinimum());
          float dimMax=static_cast<float>(m_inputWS->getDimension(i)->getMaximum());

          otherDimsMin.push_back(dimMin);
          otherDimsMax.push_back(dimMax);
          Kernel::TimeSeriesProperty<double> *run_property = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(m_inputWS->getExperimentInfo(0)->run().getProperty(m_inputWS->getDimension(i)->getName()));
          double value=run_property->firstValue();
          otherValues.push_back(value);
          //in the original MD data no time was spent measuring between dimMin and dimMax
          if ((value<dimMin)||(value>dimMax))
          {
              skipProcessing=true;
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

      for (size_t row=0; row<mat.numRows()-1; row++)
      {
          if(mat[row][0]==1.)
          {
              hIntegrated=false;
              hIndex=row;
              if(hMin<m_normWS->getDimension(row)->getMinimum()) hMin=m_normWS->getDimension(row)->getMinimum();
              if(hMax>m_normWS->getDimension(row)->getMaximum()) hMax=m_normWS->getDimension(row)->getMaximum();
              dim[row]=0;
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
              dim[row]=1;
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
              dim[row]=2;
              if((lMin>m_normWS->getDimension(row)->getMaximum())||(lMax<m_normWS->getDimension(row)->getMinimum()))
              {
                  skipProcessing=true;
              }

              //TODO: do the same thing for otherdimensions

          }

          //g_log.warning()<<"dimension "<<row<<" original "<<dim[row]<<std::endl;
      }

      //TODO: read from workspace
      KincidentMin=1.85;
      KincidentMax=10.;

      if (skipProcessing)
      {
          g_log.warning("Binning limits are outside the limits of the MDWorkspace\n");
      }
      else
      {
          Kernel::PropertyWithValue< std::vector<double> > *prop=dynamic_cast< Mantid::Kernel::PropertyWithValue<std::vector<double> >*>(m_normWS->getExperimentInfo(0)->getLog("RUBW_MATRIX"));
          Mantid::Kernel::DblMatrix RUBW((*prop)()); //includes the 2*pi factor but not goniometer for now :)
          transf=m_normWS->getExperimentInfo(0)->run().getGoniometerMatrix()*RUBW;
          transf.Invert();
          g_log.warning()<<transf<<"\n";
          std::vector<detid_t> detIDS=m_normWS->getExperimentInfo(0)->getInstrument()->getDetectorIDs(true);
          //TODO make parallel
          size_t j=0;
          for(size_t i=0;i<detIDS.size();i++)
          {
              Mantid::Geometry::IDetector_const_sptr detector=m_normWS->getExperimentInfo(0)->getInstrument()->getDetector(detIDS[i]);
              if(!detector->isMonitor()&&!detector->isMasked())
              {
                  j++;
                  std::vector<Mantid::Kernel::VMD> intersections=calculateIntersections(detector);
                  if(!intersections.empty())
                  {
                      //calculate indices
                      //add to the correct signal at that particular index
                      //NOTE: if parallel it has to be atomic

                      std::vector<Mantid::Kernel::VMD>::iterator it;
                      for (it=intersections.begin();it!=intersections.end();it++)
                      {
                          size_t hind,kind,lind;
                          hind=static_cast<size_t>(((*it)[0]-m_normWS->getDimension(0)->getMinimum())/m_normWS->getDimension(0)->getBinWidth());
                          kind=static_cast<size_t>(((*it)[1]-m_normWS->getDimension(1)->getMinimum())/m_normWS->getDimension(1)->getBinWidth());
                          lind=static_cast<size_t>(((*it)[2]-m_normWS->getDimension(2)->getMinimum())/m_normWS->getDimension(2)->getBinWidth());
                          m_normWS->setSignalAt(m_normWS->getLinearIndex(hind,kind,lind),1.);
                      }

                  }
              }
              //if (j>3) break;
          }
          g_log.warning()<<j<<"\n";
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

      //TODO: replace dimension0,1,2 with the corresponding HKL and take care of integrated dimensions
      double eps=1e-7;
      //calculate intersections with planes perpendicular to h
      if (fabs(hStart-hEnd)>eps)
      {
          double fmom=(KincidentMax-KincidentMin)/(hEnd-hStart);
          double fk=(kEnd-kStart)/(hEnd-hStart);
          double fl=(lEnd-lStart)/(hEnd-hStart);
          for(size_t i=0;i<m_normWS->getDimension(0)->getNBins();i++)
          {
              double hi=m_normWS->getDimension(0)->getX(i);
              if ((hi>=hMin)&&(hi<=hMax)&&((hStart-hi)*(hEnd-hi)<0))
              {
                  // if hi is between hStart and hEnd, then ki and li will be between kStart, kEnd and lStart, lEnd
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
          //TODO: add intersection with hMin,hMax
      }

      //calculate intersections with planes perpendicular to k
      if (fabs(kStart-kEnd)>eps)
      {
          double fmom=(KincidentMax-KincidentMin)/(kEnd-kStart);
          double fh=(hEnd-hStart)/(kEnd-kStart);
          double fl=(lEnd-lStart)/(kEnd-kStart);
          for(size_t i=0;i<m_normWS->getDimension(1)->getNBins();i++)
          {
              double ki=m_normWS->getDimension(1)->getX(i);
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

      //calculate intersections with planes perpendicular to l
      if (fabs(lStart-lEnd)>eps)
      {
          double fmom=(KincidentMax-KincidentMin)/(lEnd-lStart);
          double fh=(hEnd-hStart)/(lEnd-lStart);
          double fk=(kEnd-kStart)/(lEnd-lStart);
          for(size_t i=0;i<m_normWS->getDimension(2)->getNBins();i++)
          {
              double li=m_normWS->getDimension(2)->getX(i);
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
      return intersections;
  }


} // namespace MDAlgorithms
} // namespace Mantid
