/* WIKI*
  [[PreprocessDetectorsToMD]] is helper algorithm, used to make general part of transformation from real to reciprocal space. 
  It is used by ConvertToMD algorithm to save time on this transformation when the algorithm used multiple times for the same instrument. 
  It is also should be used to calculate limits of transformation in Q-space and the detectors trajectories in Q-space. 

   The result of this algorithm do in fact define an "ideal instrument", which is used to convert experimental data into the reciprocal space.
   Additional purpose of '''PreprocessDetectorsToMD''' is to return these data in the form, which can be easy extracted, observed and modified 
   to see and check all corrections done to the real instrument before starting transformation from the experiment's(instrument) space to the 
   target physical space. 
  
*WIKI*/
#include "MantidMDAlgorithms/PreprocessDetectorsToMD.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/NumericAxis.h"

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
      this->setWikiSummary("Servicing algorigthm to preprocess detector's positions namely  \n"
        "to perform generic part of the transformation from a matrix workspace of a real instrument to\n"
        "physical MD workspace of an experimental results (e.g Q-space).");
      this->setOptionalMessage("preprocess detector's positions namely perform generic part of the transformation \n"
        "from a physical space of a real instrument to\n"
        "physical MD workspace of an experimental results (e.g Q-space).");
    }
    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties. */
    void PreprocessDetectorsToMD::init()
    {
      auto ws_valid = boost::make_shared<Kernel::CompositeValidator>();
      // workspace needs instrument
      ws_valid->add<API::InstrumentValidator>();
      // the validator which checks if the workspace has axis and any units
      //ws_valid->add<API::WorkspaceUnitValidator>("");


      declareProperty(new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Kernel::Direction::Input,ws_valid),
        "Name of an input Matrix Workspace with instrument.");

      declareProperty(new WorkspaceProperty<TableWorkspace>("OutputWorkspace","",Kernel::Direction::Output),
        "Name of the output Table workspace with preprocessed detectors data. If the workspace exists, it will be replaced.");

      declareProperty(new Kernel::PropertyWithValue<bool>("GetEFixed",false,Kernel::Direction::Input),
        "This option makes sense for Indirect instrument, where each detector can have its own energy, defined by correspondent crystal-analyser position.\n"
        "If this option is selected for other instrument types, the value of eFixed is taken from workspace property ""Ei"" or ""eFixed"" if ""Ei""\n"
        "is missing and is set to NaN if no such properties are defined on the input workspace.");

     declareProperty(new Kernel::PropertyWithValue<bool>("GetMaskState",true,Kernel::Direction::Input),
        "Returns addiitonal column with information if the detector is masked.");

      //declareProperty(new PropertyWithValue<bool>("FakeDetectors",false,Kernel::Direction::Input),
      //  "If selected, generates table workspace with fake detectors, all allocated in a monitor position.\n"
      //  "Number of detectors is equal to number of spectra and real detectors, if present are ignored ");


    }

    //----------------------------------------------------------------------------------------------
    /* Execute the algorithm.   */
    void PreprocessDetectorsToMD::exec()
    {
      // -------- get Input workspace
      MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");  
      // -------- build target workspace:
      auto  targWS = createTableWorkspace(inputWS);
      if(this->isDetInfoLost(inputWS))
        this->buildFakeDetectorsPositions(inputWS,targWS);
      else    // process real detectors positions
        this->processDetectorsPositions(inputWS,targWS);

      // set up target workspace 
      setProperty("OutputWorkspace",targWS);
    }
    /** helper method to create resilting table workspace */
    boost::shared_ptr<DataObjects::TableWorkspace> PreprocessDetectorsToMD::createTableWorkspace(const API::MatrixWorkspace_const_sptr &inputWS)
    {
      const size_t nHist = inputWS->getNumberHistograms();


      // set the target workspace
      auto targWS = boost::shared_ptr<TableWorkspace>(new TableWorkspace(nHist));
      // detectors positions
      if(!targWS->addColumn("V3D","DetDirections"))throw(std::runtime_error("Can not add column DetDirectrions"));
      // sample-detector distance;
      if(!targWS->addColumn("double","L2"))throw(std::runtime_error("Can not add column L2"));
      // Diffraction angle
      if(!targWS->addColumn("double","TwoTheta"))throw(std::runtime_error("Can not add column TwoTheta"));
      if(!targWS->addColumn("double","Azimuthal"))throw(std::runtime_error("Can not add column Azimuthal"));
      // the detector ID;
      if(!targWS->addColumn("int32_t","DetectorID"))throw(std::runtime_error("Can not add column DetectorID"));
      // stores spectra index which corresponds to a valid detector index;
      if(!targWS->addColumn("size_t","detIDMap"))throw(std::runtime_error("Can not add column detIDMap"));
      // stores detector index which corresponds to the workspace index;
      if(!targWS->addColumn("size_t","spec2detMap"))throw(std::runtime_error("Can not add column spec2detMap"));

      m_getIsMasked = this->getProperty("GetMaskState");
      if(m_getIsMasked)
        if(!targWS->addColumn("bool","detMask"))throw(std::runtime_error("Can not add column containing for detector masks"));

      // check if one wants to obtain detector's efixed"    
      m_getEFixed = this->getProperty("GetEFixed");
      if(m_getEFixed)
        if(!targWS->addColumn("float","eFixed"))throw(std::runtime_error("Can not add column containing efixed"));



      // will see about that
      // sin^2(Theta)
      //    std::vector<double>      SinThetaSq;

      double Efi = getEi(inputWS);
      targWS->logs()->addProperty<double>("Ei",Efi,true);

      return targWS;
    }

    /** method does preliminary calculations of the detectors positions to convert results into k-dE space ;
    and places the resutls into static cash to be used in subsequent calls to this algorithm */
    void PreprocessDetectorsToMD::processDetectorsPositions(const API::MatrixWorkspace_const_sptr &inputWS,DataObjects::TableWorkspace_sptr &targWS)
    {
      g_log.information() << "Preprocessing detector locations in a target reciprocal space\n";
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
        targWS->logs()->addProperty<double>("L1",L1,true);
        g_log.debug() << "Source-sample distance: " << L1 << std::endl;
      }
      catch (Kernel::Exception::NotFoundError &)
      { 
        throw Kernel::Exception::InstrumentDefinitionError("Unable to calculate source-sample distance for workspace", inputWS->getTitle());  
      }
      // Instrument name
      std::string InstrName=instrument->getName();
      targWS->logs()->addProperty<std::string>("InstrumentName",InstrName,true); // "The name which should unique identify current instrument");
      targWS->logs()->addProperty<bool>("FakeDetectors",false,true);

      // get access to the workspace memory
      auto &sp2detMap  = targWS->getColVector<size_t>("spec2detMap");
      auto &detId      = targWS->getColVector<int32_t>("DetectorID");
      auto &detIDMap   = targWS->getColVector<size_t>("detIDMap");
      auto &L2         = targWS->getColVector<double>("L2");
      auto &TwoTheta   = targWS->getColVector<double>("TwoTheta");
      auto &Azimuthal  = targWS->getColVector<double>("Azimuthal");
      auto &detDir     = targWS->getColVector<Kernel::V3D>("DetDirections"); 
//      auto &detMask    = targWS->getColVector<bool>("detMask");

      // Efixed; do we need one and does one exist?
      double Efi = targWS->getLogs()->getPropertyValueAsType<double>("Ei");
      float *pEfixedArray(NULL);
      const Geometry::ParameterMap& pmap = inputWS->constInstrumentParameters(); 
      if (m_getEFixed)
        pEfixedArray     = targWS->getColDataArray<float>("eFixed"); 


      //// progress messave appearence
      size_t div=100;
      size_t nHist = targWS->rowCount();
      Mantid::API::Progress theProgress(this,0,1,nHist);
      //// Loop over the spectra
      uint32_t liveDetectorsCount(0);
      for (size_t i = 0; i < nHist; i++)
      {
        sp2detMap[i]=std::numeric_limits<size_t>::quiet_NaN();
        detId[i]    =std::numeric_limits<int32_t>::quiet_NaN();
        detIDMap[i] =std::numeric_limits<size_t>::quiet_NaN();
        L2[i]       =std::numeric_limits<double>::quiet_NaN(); 
        TwoTheta[i] =std::numeric_limits<double>::quiet_NaN(); 
        Azimuthal[i]=std::numeric_limits<double>::quiet_NaN(); 
   //     detMask[i]  = true;

   
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

        // calculate the requested values;
        sp2detMap[i]                = liveDetectorsCount;
        detId[liveDetectorsCount]   = int32_t(spDet->getID());
        detIDMap[liveDetectorsCount]= i;
        L2[liveDetectorsCount]      = spDet->getDistance(*sample);

        double polar   =  inputWS->detectorTwoTheta(spDet);
        double azim    =  spDet->getPhi();    
        TwoTheta[liveDetectorsCount]  =  polar;
        Azimuthal[liveDetectorsCount] =  azim;

        double sPhi=sin(polar);
        double ez = cos(polar);
        double ex = sPhi*cos(azim);
        double ey = sPhi*sin(azim);

        detDir[liveDetectorsCount].setX(ex);
        detDir[liveDetectorsCount].setY(ey);
        detDir[liveDetectorsCount].setZ(ez);

  //      detMask[i] = spDet->isMasked();
        //double sinTheta=sin(0.5*polar);
        //this->SinThetaSq[liveDetectorsCount]  = sinTheta*sinTheta;

        // specific code which should work and makes sence 
        // for indirect instrument but may be deployed on any code with Ei property defined;
        if(pEfixedArray)
        {
          try
          {
            Geometry::Parameter_sptr par = pmap.getRecursive(spDet.get(),"eFixed");
            if (par) Efi = par->value<double>();
          }
          catch(std::runtime_error&)
          {}
          // set efixed for each existing detector
          *(pEfixedArray+liveDetectorsCount) = (float)(Efi);
        }

        liveDetectorsCount++;
        if(i%div==0) theProgress.report(i,"Preprocessing detectors");

      }
      targWS->logs()->addProperty<uint32_t>("ActualDetectorsNum",liveDetectorsCount,true);

      theProgress.report();
      g_log.information()<<"Finished preprocessing detector locations. Found: "<<liveDetectorsCount<<" detectors out of: "<<nHist<<" histograms\n";
    }

    /** method calculates fake detectors positions in the situation when real detector information has been lost  */
    void PreprocessDetectorsToMD::buildFakeDetectorsPositions(const API::MatrixWorkspace_const_sptr &inputWS,DataObjects::TableWorkspace_sptr &targWS)
    {
      UNUSED_ARG(inputWS);
      // set sample-detector postion equal to 1;
      targWS->logs()->addProperty<double>("L1",1.,true);
      // 
      targWS->logs()->addProperty<std::string>("InstrumentName","FakeInstrument",true);    
      targWS->logs()->addProperty<bool>("FakeDetectors",true,true);



      // get access to the workspace memory
      auto &sp2detMap  = targWS->getColVector<size_t>("spec2detMap");
      auto &detId      = targWS->getColVector<int32_t>("DetectorID");
      auto &detIDMap   = targWS->getColVector<size_t>("detIDMap");
      auto &L2         = targWS->getColVector<double>("L2");
      auto &TwoTheta   = targWS->getColVector<double>("TwoTheta");
      auto &Azimuthal  = targWS->getColVector<double>("Azimuthal");
      auto &detDir     = targWS->getColVector<Kernel::V3D>("DetDirections"); 
  //    auto &detMask    = targWS->getColVector<bool>("detMask");

      //// progress messave appearence  
      size_t nHist = targWS->rowCount();
      targWS->logs()->addProperty<uint32_t>("ActualDetectorsNum",uint32_t(nHist),true);

      double polar(0);
      // Loop over the spectra
      for (size_t i = 0; i < nHist; i++)
      {
        sp2detMap[i]= i;
        detId[i]    = (detid_t)i;
        detIDMap[i] = i;
        L2[i]       = 1;

        TwoTheta[i] =  polar;
        Azimuthal[i] = 0;
        //this->SinThetaSq[i]= 0;

        double ez = 1.;
        double ex = 0.;
        double ey = 0.;

        detDir[i].setX(ex);
        detDir[i].setY(ey);
        detDir[i].setZ(ez);
//        detMask[i]=false;

      }
      // 
    }

    /// function checks if source workspace still has information about detectors. Some ws (like rebinned one) do not have this information any more. 
    bool PreprocessDetectorsToMD::isDetInfoLost(Mantid::API::MatrixWorkspace_const_sptr inWS2D)const
    {
      auto pYAxis = dynamic_cast<API::NumericAxis *>(inWS2D->getAxis(1));
      // if this is numeric axis, then the detector's information has been lost:
      if(pYAxis)     return true; 
      return false;
    }

    /** Method returns the efixed or Ei value stored in properties of the input workspace.
     *  Indirect instruments can have eFxed and Direct instruments can have Ei defined as the properties of the workspace. 
     *
     *  This method provide guess for efixed for all other kind of instruments. Correct indirect instrument will overwrite 
     *  this value while wrongly defined or different types of instruments will provide the value of "Ei" property (log value)
     *  or undefined if "Ei" property is not found.
     *
     */
    double PreprocessDetectorsToMD::getEi(const API::MatrixWorkspace_const_sptr &inputWS)const
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
      bool eFixedFound(false);
      if (!EiFound)
      {
        try
        {
           Efi  =inputWS->run().getPropertyValueAsType<double>("eFixed");
           eFixedFound = true;
        }
        catch(Kernel::Exception::NotFoundError &)
        {}
      }

      if (!(EiFound||eFixedFound))
        g_log.debug()<<" Ei/eFixed requested but have not been found\n";

      return Efi;
    }
    
  }
}
