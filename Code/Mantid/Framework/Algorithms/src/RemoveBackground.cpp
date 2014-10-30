//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/RemoveBackground.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"


namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(RemoveBackground)

    using namespace Kernel;
    using namespace API;
 
    //---------------------------------------------------------------------------------------------
    // Public methods
    //---------------------------------------------------------------------------------------------

    /** Initialisation method. Declares properties to be used in algorithm.
    *
    */
    void RemoveBackground::init()
    {
      auto sourceValidator = boost::make_shared<CompositeValidator>();
      sourceValidator->add<InstrumentValidator>();
      sourceValidator->add<HistogramValidator>();
      declareProperty(
        new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,sourceValidator),
        "Workspace containing the input data");
      declareProperty(
        new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
        "The name to give the output workspace");

      auto vsValidator = boost::make_shared<CompositeValidator>();
      vsValidator->add<WorkspaceUnitValidator>("TOF");
      vsValidator->add<HistogramValidator>();
      declareProperty(new WorkspaceProperty<>("FlatBkgWorkspace","",Direction::Input,vsValidator),
        "An optional histogram workspace in the units of TOF defining background for removal during rebinning."
        "The workspace has to have single value or contain the same number of spectra as the \"InputWorkspace\" and single Y value per each spectra," 
        "representing flat background in the background time region. "
        "If such workspace is present, the value of the flat background provided by this workspace is removed "
        "from each spectra of the rebinned workspace. This works for histogram and event workspace when events are not retained "
        "but actually useful mainly for removing background while rebinning an event workspace in the units different from TOF.");

      std::vector<std::string> dE_modes = Kernel::DeltaEMode().availableTypes();
      declareProperty("EMode",dE_modes[Kernel::DeltaEMode::Direct],boost::make_shared<Kernel::StringListValidator>(dE_modes),
        "If FlatBkgWorkspace is present, this property used to define the units for conversion from the units of the InputWorkspace to TOF" ,Direction::Input);
    }


    /** Executes the rebin algorithm
    *
    *  @throw runtime_error Thrown if the bin range does not intersect the range of the input workspace
    */
    void RemoveBackground::exec()
    {
      // Get the input workspace
      MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
      MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

       // Removing background in-place
      bool inPlace = (inputWS == outputWS);


      // workspace independent determination of length
      const int histnumber = static_cast<int>(inputWS->getNumberHistograms());

      //-- Flat Background removal ----------------------------
      int eMode; // in convert units emode is still integer
      API::MatrixWorkspace_const_sptr bkgWS = checkRemoveBackgroundParameters(inputWS,eMode,PreserveEvents);
      const bool remove_background = bool(bkgWS);
      if(remove_background)
      {
        int nThreads = omp_get_max_threads();
        m_BackgroundHelper.initialize(bkgWS,inputWS,eMode,nThreads);
      }
      //-------------------------------------------------------

      bool fullBinsOnly = getProperty("FullBinsOnly");

      MantidVecPtr XValues_new;
      // create new output X axis
      const int ntcnew = VectorHelper::createAxisFromRebinParams(rbParams, XValues_new.access(),
        true, fullBinsOnly);

      //---------------------------------------------------------------------------------
      //Now, determine if the input workspace is actually an EventWorkspace
      EventWorkspace_const_sptr eventInputWS = boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);

      if (eventInputWS != NULL)
      {
        //------- EventWorkspace as input -------------------------------------
        EventWorkspace_sptr eventOutputWS = boost::dynamic_pointer_cast<EventWorkspace>(outputWS);

        if (inPlace && PreserveEvents)
        {
          // -------------Rebin in-place, preserving events ----------------------------------------------
          // This only sets the X axis. Actual rebinning will be done upon data access.
          eventOutputWS->setAllX(XValues_new);
          this->setProperty("OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(eventOutputWS));
        }
        else if (!inPlace && PreserveEvents)
        {
          // -------- NOT in-place, but you want to keep events for some reason. ----------------------
          // Must copy the event workspace to a new EventWorkspace (and bin that).

          //Make a brand new EventWorkspace
          eventOutputWS = boost::dynamic_pointer_cast<EventWorkspace>(
            API::WorkspaceFactory::Instance().create("EventWorkspace", inputWS->getNumberHistograms(), 2, 1));
          //Copy geometry over.
          API::WorkspaceFactory::Instance().initializeFromParent(inputWS, eventOutputWS, false);
          //You need to copy over the data as well.
          eventOutputWS->copyDataFrom( (*eventInputWS) );

          // This only sets the X axis. Actual rebinning will be done upon data access.
          eventOutputWS->setAllX(XValues_new);

          //Cast to the matrixOutputWS and save it
          this->setProperty("OutputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(eventOutputWS));
        }
        else
        {
          //--------- Different output, OR you're inplace but not preserving Events --- create a Workspace2D -------
          g_log.information() << "Creating a Workspace2D from the EventWorkspace " << eventInputWS->getName() << ".\n";

          //Create a Workspace2D
          // This creates a new Workspace2D through a torturous route using the WorkspaceFactory.
          // The Workspace2D is created with an EMPTY CONSTRUCTOR
          outputWS = WorkspaceFactory::Instance().create("Workspace2D",histnumber,ntcnew,ntcnew-1);
          WorkspaceFactory::Instance().initializeFromParent(inputWS, outputWS, true);

          //Initialize progress reporting.
          Progress prog(this,0.0,1.0, histnumber);

          //Go through all the histograms and set the data
          PARALLEL_FOR3(inputWS, eventInputWS, outputWS)
            for (int i=0; i < histnumber; ++i)
            {
              PARALLEL_START_INTERUPT_REGION

                //Set the X axis for each output histogram
                outputWS->setX(i, XValues_new);

              //Get a const event list reference. eventInputWS->dataY() doesn't work.
              const EventList& el = eventInputWS->getEventList(i);
              MantidVec y_data, e_data;
              // The EventList takes care of histogramming.
              el.generateHistogram(*XValues_new, y_data, e_data);
              if(remove_background)
              {
                int id = omp_get_thread_num();
                m_BackgroundHelper.removeBackground(i,*XValues_new,y_data,e_data,id);
              }


              //Copy the data over.
              outputWS->dataY(i).assign(y_data.begin(), y_data.end());
              outputWS->dataE(i).assign(e_data.begin(), e_data.end());

              //Report progress
              prog.report(name());
              PARALLEL_END_INTERUPT_REGION
            }
            PARALLEL_CHECK_INTERUPT_REGION


              //Copy all the axes
              for (int i=1; i<inputWS->axes(); i++)
              {
                outputWS->replaceAxis( i, inputWS->getAxis(i)->clone(outputWS.get()) );
                outputWS->getAxis(i)->unit() = inputWS->getAxis(i)->unit();
              }

              //Copy the units over too.
              for (int i=0; i < outputWS->axes(); ++i)
                outputWS->getAxis(i)->unit() = inputWS->getAxis(i)->unit();
              outputWS->setYUnit(eventInputWS->YUnit());
              outputWS->setYUnitLabel(eventInputWS->YUnitLabel());

              // Assign it to the output workspace property
              setProperty("OutputWorkspace", outputWS);
        }

      } // END ---- EventWorkspace

      else

      { //------- Workspace2D or other MatrixWorkspace ---------------------------

        if ( ! isHist )
        {
          g_log.information() << "Rebin: Converting Data to Histogram." << std::endl;
          Mantid::API::Algorithm_sptr ChildAlg = createChildAlgorithm("ConvertToHistogram");
          ChildAlg->initialize();
          ChildAlg->setProperty("InputWorkspace", inputWS);
          ChildAlg->execute();
          inputWS = ChildAlg->getProperty("OutputWorkspace");
        }

        // This will be the output workspace (exact type may vary)
        API::MatrixWorkspace_sptr outputWS;

        // make output Workspace the same type is the input, but with new length of signal array
        outputWS = API::WorkspaceFactory::Instance().create(inputWS,histnumber,ntcnew,ntcnew-1);


        // Copy over the 'vertical' axis
        if (inputWS->axes() > 1) outputWS->replaceAxis( 1, inputWS->getAxis(1)->clone(outputWS.get()) );

        Progress prog(this,0.0,1.0,histnumber);
        PARALLEL_FOR2(inputWS,outputWS)
          for (int hist=0; hist <  histnumber;++hist)
          {
            PARALLEL_START_INTERUPT_REGION
              // get const references to input Workspace arrays (no copying)
              const MantidVec& XValues = inputWS->readX(hist);
            const MantidVec& YValues = inputWS->readY(hist);
            const MantidVec& YErrors = inputWS->readE(hist);

            //get references to output workspace data (no copying)
            MantidVec& YValues_new=outputWS->dataY(hist);
            MantidVec& YErrors_new=outputWS->dataE(hist);

            // output data arrays are implicitly filled by function
            try {
              VectorHelper::rebin(XValues,YValues,YErrors,*XValues_new,YValues_new,YErrors_new, dist);
              if(remove_background)
              {
                int id = omp_get_thread_num();
                m_BackgroundHelper.removeBackground(hist,*XValues_new,YValues_new,YErrors_new,id);
              }
            } catch (std::exception& ex)
            {
              g_log.error() << "Error in rebin function: " << ex.what() << std::endl;
              throw;
            }

            // Populate the output workspace X values
            outputWS->setX(hist,XValues_new);

            prog.report(name());
            PARALLEL_END_INTERUPT_REGION
          }
          PARALLEL_CHECK_INTERUPT_REGION
            outputWS->isDistribution(dist);

          // Now propagate any masking correctly to the output workspace
          // More efficient to have this in a separate loop because
          // MatrixWorkspace::maskBins blocks multi-threading
          for (int i=0; i <  histnumber; ++i)
          {
            if ( inputWS->hasMaskedBins(i) )  // Does the current spectrum have any masked bins?
            {
              this->propagateMasks(inputWS,outputWS,i);
            }
          }
          //Copy the units over too.
          for (int i=0; i < outputWS->axes(); ++i)
          {
            outputWS->getAxis(i)->unit() = inputWS->getAxis(i)->unit();
          }

          if ( ! isHist )
          {
            g_log.information() << "Rebin: Converting Data back to Data Points." << std::endl;
            Mantid::API::Algorithm_sptr ChildAlg = createChildAlgorithm("ConvertToPointData");
            ChildAlg->initialize();
            ChildAlg->setProperty<MatrixWorkspace_sptr>("InputWorkspace", outputWS);
            ChildAlg->execute();
            outputWS = ChildAlg->getProperty("OutputWorkspace");
          }

          if(remove_background)
          {
            const std::list<int> &failedBkgRemoalList= m_BackgroundHelper.getFailingSpectrsList();
            if(!failedBkgRemoalList.empty())
            {
              size_t nFailed = failedBkgRemoalList.size();
              if(nFailed == static_cast<size_t>(histnumber))
              {
                g_log.warning()<<" has not been able to remove any background while rebinning workspace "<<inputWS->getName()<<
                  "\n possible reasons: wrong instrument or units conversion mode\n";
              }
              else
              {
                g_log.debug()<<" has not removed background from  "<<nFailed<<" spectra\n";
                if(nFailed<20)
                {
                  g_log.debug()<<" Spectra numbers are: ";
                  for(auto it=failedBkgRemoalList.begin(); it !=failedBkgRemoalList.end();it++) g_log.debug()<<(*it);
                  g_log.debug()<<"\n";
                }
              }
            }
          }


          // Assign it to the output workspace property
          setProperty("OutputWorkspace",outputWS);


      } // END ---- Workspace2D

      return;
    }
    /*** method checks if removing background is requested and possible 
    and if yes sets up class parameter referring the workspace with flat background 

    @param inputWS the input workspace where removing background is expected
    @param eMode   the energy conversion mode, defined/requested for the instrument

    @returns background  workspace if removal is expected
    */
    API::MatrixWorkspace_const_sptr Rebin::checkRemoveBackgroundParameters(const API::MatrixWorkspace_sptr &inputWS,int &eMode, bool PreserveEvents)
    {
      API::MatrixWorkspace_const_sptr bkgWksp = getProperty("FlatBkgWorkspace");

      if (!bkgWksp)
        return bkgWksp;

      // can not remove background while preserving events
      if(PreserveEvents)
        return API::MatrixWorkspace_const_sptr();

      // source workspace has to have full instrument defined to perform background removal using this procedure. 
      auto pInstrument = inputWS->getInstrument();
      if(pInstrument)
      {
        if(!pInstrument->getSample())
        {
          g_log.warning()<<" Workspace: "<< inputWS->getName()<< " to rebin does not have properly defined instrument. Background removal will not be performed";
          return API::MatrixWorkspace_const_sptr();
        }

      }
      else
      {
        g_log.warning()<<" Workspace: "<< inputWS->getName()<< " to rebin does not have instrument attached. Background removal will not be performed";
        return API::MatrixWorkspace_const_sptr();
      }

      if(!(bkgWksp->getNumberHistograms() == 1 || inputWS->getNumberHistograms()==bkgWksp->getNumberHistograms()))
      {
        g_log.warning()<<" Background Workspace: "<< bkgWksp->getName()<< " should have the same number of spectra as source workspace or be a single histogram workspace\n";
        return API::MatrixWorkspace_const_sptr();
      }


      const std::string emodeStr = getProperty("EMode");
      eMode = static_cast<int>(Kernel::DeltaEMode().fromString(emodeStr));

      return bkgWksp;
    }
//-------------------------------------------------------------------------------------------------------------------------------
    /// Constructor
    BackgroundHelper::BackgroundHelper():
      m_WSUnit(),
      m_bgWs(),m_wkWS(),
      m_singleValueBackground(false),
      m_NBg(0),m_dtBg(1), //m_ErrSq(0),
      m_Emode(0),
      m_L1(0),m_Efix(0),
      m_Sample(),
      FailingSpectraList()
    {};
    BackgroundHelper::~BackgroundHelper()
    {
      this->deleteUnitsConverters();
    }

  /** The method deletes all units converter allocated*/
  void BackgroundHelper::deleteUnitsConverters()
  {
      for(size_t i=0;i<m_WSUnit.size();i++)
      {
        if(m_WSUnit[i])
        {
          delete m_WSUnit[i];
          m_WSUnit[i]=NULL;
        }
      }

  }

    /** Initialization method: 
    @param bkgWS    -- shared pointer to the workspace which contains background 
    @param sourceWS -- shared pointer to the workspace to remove background from
    @param emode    -- energy conversion mode used during internal units conversion (0 -- elastic, 1-direct, 2 indirect, as defined in Units conversion
    @param nThreads -- number of threads to be used for background removal
    */
    void BackgroundHelper::initialize(const API::MatrixWorkspace_const_sptr &bkgWS,const API::MatrixWorkspace_sptr &sourceWS,int emode,int nThreads)
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

      auto WSUnit = sourceWS->getAxis(0)->unit();
      if(!WSUnit)
        throw std::invalid_argument(" Source Workspace: "+sourceWS->getName()+" should have units");

      Geometry::IComponent_const_sptr source = sourceWS->getInstrument()->getSource();
      m_Sample = sourceWS->getInstrument()->getSample();
      if ((!source) || (!m_Sample)) 
        throw std::invalid_argument("Instrument on Source workspace:"+sourceWS->getName()+"is not sufficiently defined: failed to get source and/or sample");
      m_L1 = source->getDistance(*m_Sample);

      // just in case. 
      this->deleteUnitsConverters();
      // allocate the array of units converters to avoid units reallocation within a loop
      m_WSUnit.assign(nThreads,NULL);
      for(int i=0;i<nThreads;i++)
      {
        m_WSUnit[i]=WSUnit->clone();
      }

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
    * @param threadNum -- number of thread doing conversion (by default 0, single thread)
    */
    void BackgroundHelper::removeBackground(int nHist,const MantidVec &XValues,MantidVec &y_data,MantidVec &e_data,int threadNum)const
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
        // use thread-specific unit conversion class to avoid multithreading issues
        Kernel::Unit * unitConv = m_WSUnit[threadNum];
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



  } // namespace Algorithm
} // namespace Mantid
