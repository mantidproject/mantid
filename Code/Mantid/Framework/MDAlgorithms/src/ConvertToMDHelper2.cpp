/*WIKI*

*WIKI*/

#include "MantidMDAlgorithms/ConvertToMDHelper2.h"
#include "MantidMDEvents/MDWSTransform.h"

#include <cfloat>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace MDAlgorithms
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ConvertToMDHelper2)
  


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ConvertToMDHelper2::ConvertToMDHelper2()
  { }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ConvertToMDHelper2::~ConvertToMDHelper2()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Algorithm's name for identification. @see Algorithm::name
  const std::string ConvertToMDHelper2::name() const { return "ConvertToMDHelper";}
  
  
  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm

  void ConvertToMDHelper2::initDocs()
  {
    this->setWikiSummary("Calculate limits required for ConvertToMD");
    this->setOptionalMessage("Calculate limits required for ConvertToMD");
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */

  void ConvertToMDHelper2::exec()
  {
    std::vector<double> MinValues,MaxValues;
   // -------- get Input workspace
     Mantid::API::MatrixWorkspace_const_sptr InWS2D = getProperty("InputWorkspace");
   
   
  // Collect and Analyze the requests to the job, specified by the input parameters:
    //a) Q selector:
    std::string QModReq                    = getProperty("QDimensions");
    //b) the energy exchange mode
    std::string dEModReq                   = getProperty("dEAnalysisMode");
    //c) other dim property;
    std::vector<std::string> otherDimNames = getProperty("OtherDimensions");
    //d) The output dimensions in the Q3D mode, processed together with QConversionScales
    std::string QFrame                     = getProperty("Q3DFrames");
    //e) part of the procedure, specifying the target dimensions units. Currently only Q3D target units can be converted to different flavours of hkl
    std::string convertTo_                 = getProperty("QConversionScales");


    // Build the target ws description as function of the input & output ws and the parameters, supplied to the algorithm 
    MDEvents::MDWSDescription targWSDescr;

   // get raw pointer to Q-transformation (do not delete this pointer, it's held by MDTransfFactory!)
    MDEvents::MDTransfInterface* pQtransf =  MDEvents::MDTransfFactory::Instance().create(QModReq).get();
   // get number of dimensions this Q transformation generates from the workspace. 
   auto iEmode = Kernel::DeltaEMode().fromString(dEModReq);
   // get total numner of dimensions the workspace would have.
   unsigned int nMatrixDim = pQtransf->getNMatrixDimensions(iEmode,m_InWS2D);
   // total number of dimensions
   size_t nDim =nMatrixDim+otherDimNames.size();
   MinValues.resize(nDim,FLT_MAX);
   MaxValues.resize(nDim,-FLT_MAX);


    // verify that the number min/max values is equivalent to the number of dimensions defined by properties and min is less max
    targWSDescr.setMinMax(MinValues,MaxValues);   
    targWSDescr.buildFromMatrixWS(m_InWS2D,QModReq,dEModReq,otherDimNames);

  // instanciate class, responsible for defining Mslice-type projection
    MDEvents::MDWSTransform MsliceProj;
   //identify if u,v are present among input parameters and use defaults if not
    std::vector<double> ut = getProperty("UProj");
    std::vector<double> vt = getProperty("VProj");
    std::vector<double> wt = getProperty("WProj");
    try
    {  
      // otherwise input uv are ignored -> later it can be modified to set ub matrix if no given, but this may overcomplicate things. 
      MsliceProj.setUVvectors(ut,vt,wt);   
    }
    catch(std::invalid_argument &)
    {    
      g_log.error() << "The projections are coplanar. Will use defaults [1,0,0],[0,1,0] and [0,0,1]" << std::endl;    
    }

   // set up target coordinate system and identify/set the (multi) dimension's names to use
    targWSDescr.m_RotMatrix = MsliceProj.getTransfMatrix(targWSDescr,QFrame,convertTo_);           

    // this builds reduced workspace with fake instrument used to calculate min-max values. We may avoid this and use source workspace instead
    // but this left for compartibility with ConvertToMDHelper Version 1)
    buildReducedMinMaxWorkspace(InWS2D);
    //std::vector<double> MinValues,MaxValues;
    //std::string QDimension=getPropertyValue("QDimensions");
    //std::string GeometryMode=getPropertyValue("dEAnalysisMode");
    //std::string Q3DFrames=getPropertyValue("Q3DFrames");
    //std::vector<std::string> OtherDimensions=getProperty("OtherDimensions");

    //MatrixWorkspace_sptr ws=getProperty("InputWorkspace"),wstemp;
    //DataObjects::EventWorkspace_sptr evWS;
    //double xmin,xmax;

    //if (QDimension=="CopyToMD")
    //{
    //    ws->getXMinMax(xmin,xmax);
    //    MinValues.push_back(xmin);
    //    MaxValues.push_back(xmax);
    //}
    //else //need to calculate the appropriate q values
    //{
    //    double qmax,deltaEmax,deltaEmin;
    //    IAlgorithm_sptr conv = createChildAlgorithm("ConvertUnits",0.0,0.9);
    //    conv->setProperty<MatrixWorkspace_sptr>("InputWorkspace", ws);
    //    conv->setProperty<MatrixWorkspace_sptr>("OutputWorkspace", wstemp);
    //    //Calculate maxumum momentum transfer Q
    //    if(GeometryMode=="Elastic")
    //    {
    //        conv->setProperty("Target","Momentum");
    //        conv->setProperty("Emode","Elastic");
    //        conv->executeAsChildAlg();

    //        wstemp=conv->getProperty("OutputWorkspace");
    //        evWS=boost::dynamic_pointer_cast< Mantid::DataObjects::EventWorkspace >(wstemp);
    //        if (evWS)
    //        {
    //            qmax=evWS->getTofMax()*2;//assumes maximum scattering angle 180 degrees
    //        }
    //        else
    //        {
    //            qmax=wstemp->getXMax()*2.;//assumes maximum scattering angle 180 degrees
    //        }
    //    }
    //    else //inelastic
    //    {
    //        conv->setProperty("Target","DeltaE");
    //        conv->setProperty("Emode",GeometryMode);
    //        conv->executeAsChildAlg();
    //        wstemp=conv->getProperty("OutputWorkspace");
    //        evWS=boost::dynamic_pointer_cast< Mantid::DataObjects::EventWorkspace>(wstemp);
    //        if(evWS)
    //        {
    //            deltaEmin=evWS->getTofMin();
    //            deltaEmax=evWS->getTofMax();
    //        }
    //        else
    //        {
    //            wstemp->getXMinMax(deltaEmin,deltaEmax);
    //        }

    //        //Deal with nonphisical energies - conversion to DeltaE yields +-DBL_MAX
    //        if (deltaEmin < -DBL_MAX/2) deltaEmin=-deltaEmax;
    //        if (deltaEmax > DBL_MAX/2) deltaEmax=-deltaEmin;

    //        // Conversion constant for E->k. k(A^-1) = sqrt(energyToK*E(meV))
    //        const double energyToK = 8.0*M_PI*M_PI*PhysicalConstants::NeutronMass*PhysicalConstants::meV*1e-20 /
    //          (PhysicalConstants::h*PhysicalConstants::h);
    //        if(GeometryMode=="Direct")
    //        {
    //            double Ei=boost::lexical_cast<double,std::string>(ws->run().getProperty("Ei")->value());
    //            qmax=std::sqrt(energyToK*Ei)+std::sqrt(energyToK*(Ei-deltaEmin));
    //        }
    //        else//indirect
    //        {
    //            double Ef=-DBL_MAX,Eftemp=Ef;
    //            const Geometry::ParameterMap& pmap = ws->constInstrumentParameters();
    //            for(size_t i=0;i<ws->getNumberHistograms();i++)
    //            {
    //                Geometry::IDetector_const_sptr spDet;
    //                try
    //                {
    //                  spDet= ws->getDetector(i);
    //                  Geometry::Parameter_sptr par = pmap.getRecursive(spDet.get(),"eFixed");
    //                  if(par) Eftemp=par->value<double>();
    //                  if(Eftemp>Ef) Ef=Eftemp;
    //                }
    //                catch(...)
    //                {
    //                  continue;
    //                }
    //                if(Ef<=0)
    //                {
    //                    throw std::runtime_error("Could not find a fixed final energy for indirect geometry instrument.");
    //                }
    //            }
    //            qmax=std::sqrt(energyToK*Ef)+std::sqrt(energyToK*(Ef+deltaEmax));
    //        }
    //    }
    //    //Calculate limits from qmax
    //    if (QDimension=="|Q|")
    //    {
    //        MinValues.push_back(0.);
    //        MaxValues.push_back(qmax);
    //    }
    //    else//Q3D
    //    {
    //        //Q in angstroms
    //        if ((Q3DFrames=="Q")||((Q3DFrames=="AutoSelect")&&(!ws->sample().hasOrientedLattice())))
    //        {
    //            MinValues.push_back(-qmax);MinValues.push_back(-qmax);MinValues.push_back(-qmax);
    //            MaxValues.push_back(qmax);MaxValues.push_back(qmax);MaxValues.push_back(qmax);
    //        }
    //        else //HKL
    //        {
    //            if(!ws->sample().hasOrientedLattice())
    //            {
    //                g_log.error()<<"Samplem has no oriented lattice"<<std::endl;
    //                throw std::invalid_argument("No UB set");
    //            }
    //            Mantid::Geometry::OrientedLattice ol=ws->sample().getOrientedLattice();
    //            qmax/=(2.*M_PI);
    //            MinValues.push_back(-qmax*ol.a());MinValues.push_back(-qmax*ol.b());MinValues.push_back(-qmax*ol.c());
    //            MaxValues.push_back(qmax*ol.a());MaxValues.push_back(qmax*ol.b());MaxValues.push_back(qmax*ol.c());
    //        }
    //    }


    //    //Push deltaE if necessary
    //    if(GeometryMode!="Elastic")
    //    {
    //        MinValues.push_back(deltaEmin);
    //        MaxValues.push_back(deltaEmax);
    //    }
    //}

    //for(size_t i=0;i<OtherDimensions.size();++i)
    //{
    //    if(!ws->run().hasProperty(OtherDimensions[i]))
    //    {
    //        g_log.error()<<"The workspace does not have a property "<<OtherDimensions[i]<<std::endl;
    //        throw std::invalid_argument("Property not found. Please see error log.");
    //    }
    //    Kernel::Property *pProperty = (ws->run().getProperty(OtherDimensions[i]));
    //    TimeSeriesProperty<double> *p=dynamic_cast<TimeSeriesProperty<double> *>(pProperty);
    //    if (p)
    //    {
    //      MinValues.push_back(p->getStatistics().minimum);
    //      MaxValues.push_back(p->getStatistics().maximum);
    //    }
    //    else // it may be not a time series property but just number property
    //    {
    //        Kernel::PropertyWithValue<double> *p = dynamic_cast<Kernel::PropertyWithValue<double> *>(pProperty);  
    //        if(!p)
    //        {
    //            std::string ERR = " Can not interpret property, used as dimension.\n Property: "+OtherDimensions[i]+
    //                              " is neither a time series (run) property nor a property with value<double>";
    //             throw(std::invalid_argument(ERR));
    //        }
    //        double val = *p;
    //        MinValues.push_back(val);
    //        MaxValues.push_back(val);

    //    }
    //}

    //setProperty("MinValues",MinValues);
    //setProperty("MaxValues",MaxValues);
  }

   void ConvertToMDHelper2::buildReducedMinMaxWorkspace(Mantid::API::MatrixWorkspace_const_sptr &InWS2D)
   {

   }

} // namespace MDAlgorithms
} // namespace Mantid
