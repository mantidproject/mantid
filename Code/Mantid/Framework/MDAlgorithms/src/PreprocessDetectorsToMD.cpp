/* WIKI*
dfkl;aflfk;

*WIKI*/
#include "MantidMDAlgorithms/PreprocessDetectorsToMD.h"
#include "MantidKernel/CompositeValidator.h"
//#include "


using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;


namespace Mantid
{
  namespace MDAlgorithms
  {
    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(PreprocessDetectorsToMD)

   PreprocessDetectorsToMD::PreprocessDetectorsToMD()
   {};

    // Sets documentation strings for this algorithm
    void PreprocessDetectorsToMD::initDocs()
    {
      this->setWikiSummary("Service algorigthm ");
      this->setOptionalMessage("");
    }
    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties. */
    void PreprocessDetectorsToMD::init()
    {
      auto ws_valid = boost::make_shared<Kernel::CompositeValidator>();
      // workspace needs instrument
      ws_valid->add<API::InstrumentValidator>();
      // the validator which checks if the workspace has axis and any units
      ws_valid->add<API::WorkspaceUnitValidator>("");


      declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Kernel::Direction::Input,ws_valid),
        "An input Matrix Workspace");

      declareProperty(new WorkspaceProperty<TableWorkspace>("OutputWorkspace","",Kernel::Direction::InOut),
        "Name of the output Table workspace. If the workspace exists, it will be replaced");

    }

    //----------------------------------------------------------------------------------------------
    /* Execute the algorithm.   */
    void PreprocessDetectorsToMD::exec()
    {
        // -------- get Input workspace
      MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
      const size_t nHist = inputWS->getNumberHistograms();
  
      // set the target workspace
      auto targWS = boost::shared_ptr<TableWorkspace>(new TableWorkspace(nHist));
      // detectors positions
      if(!targWS->addColumn("V3D","DetDirections"))throw(std::runtime_error("Can not add column DetDirectrions"));
      // sample-detector distance;
      if(!targWS->addColumn("double","L2"))throw(std::runtime_error("Can not add column L2"));
      // Diffraction angle
      if(!targWS->addColumn("double","TwoTheta"))throw(std::runtime_error("Can not add column TwoTheta"));
      // the detector ID;
      if(!targWS->addColumn("int32_t","DetectorID"))throw(std::runtime_error("Can not add column DetectorID"));
      // stores spectra index which corresponds to a valid detector index;
      if(!targWS->addColumn("size_t","detIDMap"))throw(std::runtime_error("Can not add column detIDMap"));
       // stores detector index which corresponds to the workspace index;
      if(!targWS->addColumn("size_t","spec2detMap"))throw(std::runtime_error("Can not add column spec2detMap"));
      // will see about that
    // sin^2(Theta)
  //    std::vector<double>      SinThetaSq;


    }
    /** helper function, does preliminary calculations of the detectors positions to convert results into k-dE space ;
    and places the resutls into static cash to be used in subsequent calls to this algorithm */
    void PreprocessDetectorsToMD::processDetectorsPositions(const API::MatrixWorkspace_sptr inputWS,DataObjects::TableWorkspace_sptr &targWS)
    {
      g_log.information()<<" Preprocessing detectors locations in a target reciprocal space\n";
      // 
      Geometry::Instrument_const_sptr instrument = inputWS->getInstrument();
      //this->pBaseInstr                = instrument->baseInstrument();
      //
      Geometry::IObjComponent_const_sptr source = instrument->getSource();
      Geometry::IObjComponent_const_sptr sample = instrument->getSample();
      if ((!source) || (!sample)) 
      {
        g_log.error()<<" Instrument is not fully defined. Can not identify source or sample\n";
        throw Kernel::Exception::InstrumentDefinitionError("Instrubment not sufficiently defined: failed to get source and/or sample");
      }

      // L1
      try
      {
        double L1  = source->getDistance(*sample);
        g_log.debug() << "Source-sample distance: " << L1 << std::endl;
        targWS->declareProperty(new Kernel::PropertyWithValue<double>("L1",L1),"L1 is the source to sample distance");
      }
      catch (Kernel::Exception::NotFoundError &)
      { throw Kernel::Exception::InstrumentDefinitionError("Unable to calculate source-sample distance for workspace", inputWS->getTitle());  }
     
    

       auto sp2detMap = targWS->getColumn("spec2detMap");
       auto detId     = targWS->getColumn("DetectorID");
       auto detIDMap  = targWS->getColumn("detIDMap");
  //  this->detIDMap[actual_detectors_count]  = i;
       //std::vector<size_t> & sp2detMap = aColumn->data();
//       aColumm


      //// progress messave appearence
       size_t div=100;
       size_t nHist = targWS->rowCount();
       Mantid::API::Progress theProgress(this,0,1,nHist);
      //// Loop over the spectra
       size_t liveDetectorsCount(0);
      for (size_t i = 0; i < nHist; i++)
      {
       sp2detMap->cell<size_t>(i)=std::numeric_limits<size_t>::quiet_NaN();
       detId->cell<uint32_t>(i)  =std::numeric_limits<uint32_t>::quiet_NaN();
       detIDMap->cell<size_t>(i) =std::numeric_limits<size_t>::quiet_NaN();

       // get detector or detector group which corresponds to the spectra i
        Geometry::IDetector_const_sptr spDet;
        try
        {
          spDet= inputWS->getDetector(i);      
        }
        catch(Kernel::Exception::NotFoundError &)
        {
          continue;
        }

        // Check that we aren't dealing with monitor...
        if (spDet->isMonitor())continue;   

        sp2detMap->cell<size_t>(i) = liveDetectorsCount;
        detId->cell<uint32_t>(liveDetectorsCount)=spDet->getID();
        detIDMap->cell<size_t>(liveDetectorsCount)=i;



      //  this->L2[actual_detectors_count]        = spDet->getDistance(*sample);

      //  double polar        =  inputWS->detectorTwoTheta(spDet);
      //  double azim         =  spDet->getPhi();    
      //  this->TwoTheta[actual_detectors_count]  =  polar;
     
      //  double sPhi=sin(polar);
      //  double ez = cos(polar);
      //  double ex = sPhi*cos(azim);
      //  double ey = sPhi*sin(azim);
      //  double sinTheta=sin(0.5*polar);
      //  this->SinThetaSq[actual_detectors_count]  = sinTheta*sinTheta;


      //  this->det_dir[actual_detectors_count].setX(ex);
      //  this->det_dir[actual_detectors_count].setY(ey);
      //  this->det_dir[actual_detectors_count].setZ(ez);

         liveDetectorsCount++;

        if(i%div==0) theProgress.report(i,"Preprocessing detectors");

      }
      //// 
      //if(actual_detectors_count<nHist)
      //{
      //  this->det_dir.resize(actual_detectors_count);
      //  this->det_id.resize(actual_detectors_count);
      //  this->L2.resize(actual_detectors_count);
      //  this->TwoTheta.resize(actual_detectors_count);
      //  this->SinThetaSq.resize(actual_detectors_count);
      //  this->detIDMap.resize(actual_detectors_count);
      //}
      //Log.information()<<"finished preprocessing detectors locations \n";
      theProgress.report();
    }


  }
}