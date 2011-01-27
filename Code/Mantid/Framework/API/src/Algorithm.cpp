//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/DateAndTime.h"
#include <iomanip>

using namespace Mantid::Kernel;


namespace Mantid
{
  namespace API
  {

    unsigned int Algorithm::g_execCount=0;

    /// Constructor
    Algorithm::Algorithm() :
    PropertyManagerOwner(),m_progressObserver(*this, &Algorithm::handleChildProgressNotification),
      m_cancel(false),m_parallelException(false),g_log(Kernel::Logger::get("Algorithm")),
      m_executeAsync(this,&Algorithm::executeAsyncImpl),m_isInitialized(false),
      m_isExecuted(false),m_isChildAlgorithm(false),m_runningAsync(false),
      m_running(false),m_rethrow(false),m_algorithmID(0)
    {
    }

    /// Virtual destructor
    Algorithm::~Algorithm()
    {
      g_log.release();
    }


    /** Initialization method invoked by the framework. This method is responsible
    *  for any bookkeeping of initialization required by the framework itself.
    *  It will in turn invoke the init() method of the derived algorithm,
    *  and of any sub-algorithms which it creates.
    *  @throw runtime_error Thrown if algorithm or sub-algorithm cannot be initialised
    *
    */
    void Algorithm::initialize()
    {
      // Bypass the initialization if the algorithm has already been initialized.
      if (m_isInitialized) return;

      g_log.setName(this->name());
      try
      {
        // Invoke init() method of the derived class inside a try/catch clause
        try
        {
          this->init();
        }
        catch(std::runtime_error& ex)
        {
          g_log.error()<<"Error initializing main algorithm"<<ex.what();
          throw;
        }

        // Indicate that this Algorithm has been initialized to prevent duplicate attempts.
        setInitialized();
      }
      // Unpleasant catch-all! Along with this, Gaudi version catches GaudiException & std::exception
      // but doesn't really do anything except (print fatal) messages.
      catch (...)
      {
        // Gaudi: A call to the auditor service is here
        // (1) perform the printout
        g_log.fatal("UNKNOWN Exception is caught");
        throw;
      }

      // Only gets to here if everything worked normally
      return;
    }

    /** The actions to be performed by the algorithm on a dataset. This method is
    *  invoked for top level algorithms by the application manager.
    *  This method invokes exec() method.
    *  For sub-algorithms either the execute() method or exec() method
    *  must be EXPLICITLY invoked by  the parent algorithm.
    *
    *  @throw runtime_error Thrown if algorithm or sub-algorithm cannot be executed
    */
    bool Algorithm::execute()
    {
      // Start by freeing up any memory available.
      Mantid::API::MemoryManager::Instance().releaseFreeMemory();

      m_notificationCenter.postNotification(new StartedNotification(this));
      Mantid::Kernel::DateAndTime start_time;
      // Return a failure if the algorithm hasn't been initialized
      if ( !isInitialized() )
      {
        g_log.error("Algorithm is not initialized:" + this->name());
        throw std::runtime_error("Algorithm is not initialised:" + this->name());
      }

      // no logging of input if a child algorithm
      if (!m_isChildAlgorithm) algorithm_info();

      // Check all properties for validity
      if ( !validateProperties() )
      {
        // Reset name on input workspaces to trigger attempt at collection from ADS
        const std::vector< Property*> &props = getProperties();
        for (unsigned int i = 0; i < props.size(); ++i)
        {
          IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty*>(props[i]);
          if (wsProp && !(wsProp->getWorkspace()))
          {
            // Setting it's name to the same one it alreasy had
            props[i]->setValue(props[i]->value());
          }
        }
        // Try the validation again
        if ( !validateProperties() ) throw std::runtime_error("Some invalid Properties found");

      }
      // get properties and check one of the input properties is a work space group
      // if it's a group call process group 
      // if not normal execution

      std::vector<Property*> Prop=getProperties();
      std::vector<Property*>::const_iterator itr;
      for (itr=Prop.begin();itr!=Prop.end();itr++)
      {
        const IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty*>(*itr);
        if (wsProp)
        {
          const Property *wsPropProp = dynamic_cast<Property*>(*itr);
          unsigned int direction = wsPropProp->direction();
          if (direction == Kernel::Direction::Input ||direction==Kernel::Direction::InOut)
          {
            std::string wsName=wsPropProp->value();
            // try
            //{
            //checking the input is a group
            try
            {	  //check if the pointer is valid, it won't be if it is a group
              Workspace_sptr wsSptr=wsProp->getWorkspace();
              if(!wsName.empty() && !wsSptr)
              {
                boost::shared_ptr<WorkspaceGroup> wsGrpSptr =
                  boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve(wsName));
                if(wsGrpSptr)
                {	 //this must be a group - test for that
                  g_log.debug()<<" one of the inputs is a workspace group - call processGroups"<<std::endl;

                  return processGroups(wsGrpSptr,Prop);


                }
              }

            }
            catch (std::invalid_argument&ex)
            {
              g_log.error()<< "Error in execution of algorithm "<< this->name()<<std::endl;
              g_log.error()<<ex.what()<<std::endl;
              m_notificationCenter.postNotification(new ErrorNotification(this,ex.what()));
              m_running = false;
              return false;
              if (m_isChildAlgorithm || m_runningAsync || m_rethrow)
              {
                m_runningAsync = false;
                throw;
              }


            }
            catch(Exception::NotFoundError&ex )
            {
              g_log.error()<< "Error in execution of algorithm "<< this->name()<<std::endl;
              g_log.error()<<ex.what()<<std::endl;
              m_notificationCenter.postNotification(new ErrorNotification(this,ex.what()));
              m_running = false;
              return false;
              if (m_isChildAlgorithm || m_runningAsync || m_rethrow)
              {
                m_runningAsync = false;
                throw;
              }


            }
            catch (std::runtime_error &ex)
            {
              g_log.error()<< "Error in execution of algorithm "<< this->name()<<std::endl;
              g_log.error()<<ex.what()<<std::endl;
              m_notificationCenter.postNotification(new ErrorNotification(this,ex.what()));
              m_running = false;
              return false;
              if (m_isChildAlgorithm || m_runningAsync || m_rethrow)
              {
                m_runningAsync = false;
                throw;
              }

            }
            catch(CancelException& ex)
            {
              g_log.error()<< "Error in execution of algorithm "<< this->name()<<std::endl;
              g_log.error()<<ex.what()<<std::endl;
              m_notificationCenter.postNotification(new ErrorNotification(this,ex.what()));
              m_running = false;
              return false;
              if (m_isChildAlgorithm || m_runningAsync || m_rethrow)
              {
                m_runningAsync = false;
                throw;
              }


            }
            catch(std::exception&ex)
            {
              g_log.error()<< "Error in execution of algorithm "<< this->name()<<std::endl;
              g_log.error()<<ex.what()<<std::endl;
              m_notificationCenter.postNotification(new ErrorNotification(this,ex.what()));
              m_running = false;
              return false;
              if (m_isChildAlgorithm || m_runningAsync || m_rethrow)
              {
                m_runningAsync = false;
                throw;
              }

            }

            //}

          }//end of if loop checking the direction
        }//end of if loop for checking workspace properties

      }// end of for loop for checking the properties for workspace groups

      // Invoke exec() method of derived class and catch all uncaught exceptions
      try
      {
        try
        {
          if (!m_isChildAlgorithm) m_running = true;
          start_time = Mantid::Kernel::DateAndTime::get_current_time();
          //count used for defining the algorithm execution order
          ++Algorithm::g_execCount;
          // Start a timer
          Timer timer;
          // Call the concrete algorithm's exec method
          this->exec();
          // Get how long this algorithm took to run
          const float duration = timer.elapsed();
          // need it to throw before trying to run fillhistory() on an algorithm which has failed
          // Put any output workspaces into the AnalysisDataService - if this is not a child algorithm
          if (!isChild())
          {
            fillHistory(start_time,duration,Algorithm::g_execCount);
            this->store();
          }

          // RJT, 19/3/08: Moved this up from below the catch blocks
          setExecuted(true);
          if (!m_isChildAlgorithm) 
          {
            g_log.notice() << name() << " successful, Duration "
              << std::fixed << std::setprecision(2) << duration << " seconds" << std::endl;
          }
          m_running = false;
        }
        catch(std::runtime_error& ex)
        {
          if (m_isChildAlgorithm || m_runningAsync || m_rethrow) throw;
          else
          {
            g_log.error()<< "Error in execution of algorithm "<< this->name()<<std::endl;
            g_log.error()<< ex.what()<<std::endl;
          }
          m_notificationCenter.postNotification(new ErrorNotification(this,ex.what()));
          m_running = false;
        }
        catch(std::logic_error& ex)
        {
          if (m_isChildAlgorithm || m_runningAsync || m_rethrow) throw;
          else
          {
            g_log.error()<< "Logic Error in execution of algorithm "<< this->name()<<std::endl;
            g_log.error()<< ex.what()<<std::endl;
          }
          m_notificationCenter.postNotification(new ErrorNotification(this,ex.what()));
          m_running = false;
        }
      }
      catch(CancelException& ex)
      {
        m_runningAsync = false;
        m_running = false;
        g_log.error() << this->name() << ": Execution terminated by user.\n";
        m_notificationCenter.postNotification(new ErrorNotification(this,ex.what()));
        throw;
      }
      // Gaudi also specifically catches GaudiException & std:exception.
      catch (std::exception& ex)
      {
        setExecuted(false);
        m_runningAsync = false;
        m_running = false;

        m_notificationCenter.postNotification(new ErrorNotification(this,ex.what()));
        g_log.error() << "Error in execution of algorithm " << this->name() << ":\n";
        g_log.error(ex.what());
        throw;
      }

      catch (...)
      {
        // Gaudi sets the executed flag to true here despite the exception
        // This allows it to move to the next command or it just loops indefinitely.
        // we will set it to false (see Nick Draper) 6/12/07
        setExecuted(false);
        m_runningAsync = false;
        m_running = false;

        m_notificationCenter.postNotification(new ErrorNotification(this,"UNKNOWN Exception is caught "));
        g_log.error() << this->name() << ": UNKNOWN Exception is caught\n";
        throw;
        // Gaudi calls exception service 'handle' method here
      }

      // Gaudi has some stuff here where it tests for failure, increments the error counter
      // and then converts to success if less than the maximum. This is clearly related to
      // having an event loop, and thus we shouldn't want it. This is the only place it's used.

      m_notificationCenter.postNotification(new FinishedNotification(this,isExecuted()));
      // Only gets to here if algorithm ended normally

      return isExecuted();
    }

    /// Has the Algorithm already been initialized
    bool Algorithm::isInitialized() const
    {
      return m_isInitialized;
    }

    /// Has the Algorithm already been executed
    bool Algorithm::isExecuted() const
    {
      return m_isExecuted;
    }

    /** Create a sub algorithm.  A call to this method creates a child algorithm object.
    *  Using this mechanism instead of creating daughter
    *  algorithms directly via the new operator is prefered since then
    *  the framework can take care of all of the necessary book-keeping.
    *
    *  @param name           The concrete algorithm class of the sub algorithm
    *  @param startProgress  The percentage progress value of the overall algorithm where this child algorithm starts
    *  @param endProgress    The percentage progress value of the overall algorithm where this child algorithm ends
    *  @param enableLogging  Set to false to disable logging from the child algorithm
    *  @param version        The version of the child algorithm to create. By default gives the latest version.
    *  @returns Set to point to the newly created algorithm object
    */
    IAlgorithm_sptr Algorithm::createSubAlgorithm(const std::string& name, const double startProgress, const double endProgress, 
      const bool enableLogging, const int& version)
    {
      IAlgorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged(name,version);
      //set as a child
      alg->setChild(true);
      alg->setLogging(enableLogging);

      // Initialise the sub-algorithm
      try
      {
        alg->initialize();
      }
      catch (std::runtime_error&)
      {
        g_log.error() << "Unable to initialise sub-algorithm " << name << std::endl;
      }

      // If output workspaces are nameless, give them a temporary name to satisfy validator
      const std::vector< Property*> &props = alg->getProperties();
      for (unsigned int i = 0; i < props.size(); ++i)
      {
        if (props[i]->direction() == 1 && dynamic_cast<IWorkspaceProperty*>(props[i]) )
        {
          if ( props[i]->value().empty() ) props[i]->setValue("ChildAlgOutput");
        }
      }

      if (startProgress >= 0 && endProgress > startProgress && endProgress <= 1.)
      {
        alg->addObserver(m_progressObserver);
        m_startChildProgress = startProgress;
        m_endChildProgress = endProgress;
      }

      // Before we return the shared pointer, use it to create a weak pointer and keep that in a vector.
      // It will be used this to pass on cancellation requests
      // It must be protected by a critical block so that sub algorithms can run in parallel safely.
      IAlgorithm_wptr weakPtr(alg);
      PARALLEL_CRITICAL(Algorithm_StoreWeakPtr)
      {
	m_ChildAlgorithms.push_back(weakPtr);
      }

      return alg;
    }

    /**  Add an observer to a notification
    @param observer Reference to the observer to add
    */
    void Algorithm::addObserver(const Poco::AbstractObserver& observer)const
    {
      m_notificationCenter.addObserver(observer);
    }

    /**  Remove an observer
    @param observer Reference to the observer to remove
    */
    void Algorithm::removeObserver(const Poco::AbstractObserver& observer)const
    {
      m_notificationCenter.removeObserver(observer);
    }

    void Algorithm::cancel()const
    {
      //set myself to be cancelled
      m_cancel = true;

      //set any child algorithms to be cancelled as well
      std::vector<IAlgorithm_wptr>::const_iterator it;

      // Loop over the output workspaces and try to cancel them
      for (it = m_ChildAlgorithms.begin(); it != m_ChildAlgorithms.end(); ++it)
      {
        IAlgorithm_wptr weakPtr = *it;
        if (IAlgorithm_sptr sharedPtr = weakPtr.lock())
        {
          sharedPtr->cancel();
        }
      }
    }

    //----------------------------------------------------------------------
    // Protected Member Functions
    //----------------------------------------------------------------------

    /** Initialize using proxy algorithm.
    Call the main initialize method and then copy in the property values.
    @param proxy Initialising proxy algorithm
    */
    void Algorithm::initializeFromProxy(const AlgorithmProxy& proxy)
    {
      initialize();
      copyPropertiesFrom(proxy);
      m_algorithmID = proxy.getAlgorithmID();
      setLogging(proxy.isLogging());
    }

    /// Set the Algorithm initialized state
    void Algorithm::setInitialized()
    {
      m_isInitialized = true;
    }

    /** Set the executed flag to the specified state
    // Public in Gaudi - don't know why and will leave here unless we find a reason otherwise
    //     Also don't know reason for different return type and argument.
    @param state New executed state
    */
    void Algorithm::setExecuted(bool state)
    {
      m_isExecuted = state;
    }

    /** Sends ProgressNotification. 
    @param p Reported progress,  must be between 0 (just started) and 1 (finished)
    @param msg Optional message string
    */
    void Algorithm::progress(double p, const std::string& msg)
    {
      m_notificationCenter.postNotification(new ProgressNotification(this,p,msg));
    }

    void Algorithm::interruption_point()
    {
      // only throw exceptions if the code is not multi threaded otherwise you contravene the OpenMP standard
      // that defines that all loops must complete, and no exception can leave an OpenMP section
      // openmp cancel handling is performed using the ??, ?? and ?? macros in each algrothim
      IF_NOT_PARALLEL
        if (m_cancel) throw CancelException();
    }

    //----------------------------------------------------------------------
    // Private Member Functions
    //----------------------------------------------------------------------

    /** Fills History, Algorithm History and Algorithm Parameters
    *  @param start a date and time defnining the start time of the algorithm
    *  @param duration a double defining the length of duration of the algorithm
    *  @param  uexecCount an unsigned int for defining the excution order of algorithm
    */
    void Algorithm::fillHistory(Mantid::Kernel::DateAndTime start,double duration,unsigned int uexecCount)
    {
      // Create two vectors to hold a list of pointers to the input & output workspaces (InOut's go in both)
      std::vector<Workspace_sptr> inputWorkspaces, outputWorkspaces;
      findWorkspaceProperties(inputWorkspaces,outputWorkspaces);

      // Create the history object for this algorithm
      AlgorithmHistory algHistory(this,start,duration,uexecCount);

      std::vector<Workspace_sptr>::iterator outWS;
      std::vector<Workspace_sptr>::const_iterator inWS;
      // Loop over the output workspaces
      for (outWS = outputWorkspaces.begin(); outWS != outputWorkspaces.end(); ++outWS)
      {
        // Loop over the input workspaces, making the call that copies their history to the output ones
        // (Protection against copy to self is in WorkspaceHistory::copyAlgorithmHistory)
        for (inWS = inputWorkspaces.begin(); inWS != inputWorkspaces.end(); ++inWS)
        {
          (*outWS)->history().copyAlgorithmHistory( (*inWS)->getHistory() );
        }
        // Add the history for the current algorithm to all the output workspaces
        (*outWS)->history().addAlgorithmHistory(algHistory);
      }
    }

    /** Populate lists of the input & output workspace properties.
    *  (InOut workspaces go in both lists)
    *  @param inputWorkspaces  A reference to a vector for the input workspaces
    *  @param outputWorkspaces A reference to a vector for the output workspaces
    */
    void Algorithm::findWorkspaceProperties(std::vector<Workspace_sptr>& inputWorkspaces,
      std::vector<Workspace_sptr>& outputWorkspaces) const
    {
      // Loop over properties looking for the workspace properties and putting them in the right list
      const std::vector<Property*>& algProperties = getProperties();
      std::vector<Property*>::const_iterator it;
      for (it = algProperties.begin(); it != algProperties.end(); ++it)
      {
        const IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty*>(*it);
        if (wsProp)
        {
          const Property *wsPropProp = dynamic_cast<Property*>(*it);
          // Check we actually have a workspace, it may have been optional
          Workspace_sptr workspace = wsProp->getWorkspace();
          if( !workspace ) continue;
          unsigned int direction = wsPropProp->direction();
          if (direction == Direction::Input || direction == Direction::InOut)
          {
            inputWorkspaces.push_back(workspace);
          }
          if (direction == Direction::Output || direction == Direction::InOut)
          {
            outputWorkspaces.push_back(workspace);
          }
        }
      }
    }

    /// puts out algorithm parameter information to the logger
    void Algorithm::algorithm_info() const
    {
      // Make use of the AlgorithmHistory class, which holds all the info we want here
      AlgorithmHistory AH(this);
      g_log.information() << AH;
    }

    /** Stores any output workspaces into the AnalysisDataService
    *  @throw std::runtime_error If unable to successfully store an output workspace
    */
    void Algorithm::store()
    {
      const std::vector< Property*> &props = getProperties();
      for (unsigned int i = 0; i < props.size(); ++i)
      {
        IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty*>(props[i]);
        if (wsProp)
        {
          try
          {
            wsProp->store();
          }
          catch (std::runtime_error&)
          {
            g_log.error("Error storing output workspace in AnalysisDataService");
            throw;
          }
        }
      }
    }

    /** To query whether algorithm is a child.
    *  @returns true - the algorithm is a child algorithm.  False - this is a full managed algorithm.
    */
    bool Algorithm::isChild() const
    {
      return m_isChildAlgorithm;
    }

    /** To set whether algorithm is a child.
    *  @param isChild True - the algorithm is a child algorithm.  False - this is a full managed algorithm.
    */
    void Algorithm::setChild(const bool isChild)
    {
      m_isChildAlgorithm = isChild;
    }

    void Algorithm::setRethrows(const bool rethrow)
    {
      this->m_rethrow = rethrow;
    }

    /**
    * Asynchronous execution
    */
    Poco::ActiveResult<bool> Algorithm::executeAsync()
    { 
      return m_executeAsync(Poco::Void());
    }

    /** To Process workspace groups.
    *  @param ingrpws_sptr input workspacegroup pointer to iterate through all members
    *  @param  props a vector holding the input properties
    *  @returns true - if all the workspace members are executed.
    */
    bool Algorithm::processGroups(WorkspaceGroup_sptr ingrpws_sptr,const std::vector<Mantid::Kernel::Property*>& props)
    {
      int nPeriod=1;
      int execPercentage=0;
      bool bgroupPassed=true;
      bool bgroupFailed=false;

      WorkspaceGroup_sptr wsgrp1_sptr; 
      WorkspaceGroup_sptr wsgrp2_sptr;
      bool bnewGoup1=true;
      bool bnewGoup2=true;
      std::string prevPropName("");

      //getting the input workspace group names
      const std::vector<std::string> inputWSNames=ingrpws_sptr->getNames();
      int nSize=inputWSNames.size();
      //size is one if only group header.
      //return if atleast one meber is not there in group to process
      if(nSize<1)
      {	throw std::runtime_error("Input WorkspaceGroup has no members to process");
      }

      int execTotal=0;
      //removing the header count from the totalsize
      execTotal=(nSize)*10;
      m_notificationCenter.postNotification(new StartedNotification(this));

      IAlgorithm* alg = API::FrameworkManager::Instance().createAlgorithm(this->name() ,"",this->version());
      if(!alg) {g_log.error()<<"createAlgorithm returned null pointer "<<std::endl;return false;}

      //now check the input and output workspaces are of same name.
      bool bequal=isInputequaltoOutPut(props);

      std::string ingrpwsName;
      getInputGroupWorkspaceName(props,ingrpwsName);
      //check the workspace are of similar names (similar if they are of names like group_1,group_2)
      bool bSimilarNames=isGroupWorkspacesofSimilarNames(ingrpwsName,inputWSNames);

      std::vector<std::string>::const_iterator wsItr=inputWSNames.begin();
      for(;wsItr!=inputWSNames.end();++wsItr)
      {	//set  properties
        std::vector<Mantid::Kernel::Property*>::const_iterator itr;
        for (itr=props.begin();itr!=props.end();itr++)
        {
          int outWSCount=0;

          if(isWorkspaceProperty(*itr) )
          {		
            if(isInputWorkspaceProperty(*itr))
            {

              if(!setInputWSProperties(alg,*itr,*wsItr))
              {
                throw std::runtime_error("Invalid value found for the property " +(*itr)->name()+ " when executing the algorithm"+this->name());
              }

            }
            if(isOutputWorkspaceProperty(*itr))
            {
              ++outWSCount;
              //create a group and pass that to setOutputWSProperties 
              if(outWSCount==1)
              { 
                if( bnewGoup1)
                {
                  wsgrp1_sptr= WorkspaceGroup_sptr(new WorkspaceGroup);
                  bnewGoup1=false;
                }
                if(!setOutputWSProperties(alg,*itr,nPeriod,*wsItr,wsgrp1_sptr,bSimilarNames,bequal))
                {
                  throw std::runtime_error("Invalid value found for the property " +(*itr)->name()+ " when executing the algorithm"+this->name());
                }

              }
              if(outWSCount==2)
              {
                if( bnewGoup2)
                {
                  wsgrp2_sptr= WorkspaceGroup_sptr(new WorkspaceGroup);
                  bnewGoup2=false;
                }
                if(!setOutputWSProperties(alg,*itr,nPeriod,*wsItr,wsgrp2_sptr,bSimilarNames,bequal))
                {
                  throw std::runtime_error("Invalid value found for the property " +(*itr)->name()+ " when executing the algorithm"+this->name());
                }
              }

            }//end of isOutputWorkspaceProperty

          }// end of isWorkspaceProperty
          else
          {
            this->setOtherProperties(alg,(*itr)->name(),(*itr)->value(),nPeriod);
          }
        }//end of for loop for setting properties
        //resetting the previous properties at the end of each execution 
        prevPropName="";
        // execute the algorithm 
        bool bStatus = false;
        if ( alg->validateProperties() ) 
        {bStatus = alg->execute();
        }
        // status of each execution is checking 
        bgroupPassed=bgroupPassed&&bStatus;
        bgroupFailed=bgroupFailed||bStatus;
        execPercentage+=10;
        progress(double((execPercentage)/execTotal));
        //if a workspace execution fails
        if(!bStatus)
        {  
          throw std::runtime_error("execution failed for the input workspace "+(*wsItr));
        }
        //increment count for outworkpsace name
        nPeriod++;

      }//end of for loop for input workspace group members processing
      //if all passed 
      if(bgroupPassed)
      {setExecuted(true);
      }

      m_notificationCenter.postNotification(new FinishedNotification(this,isExecuted()));
      return bgroupPassed;
    }

    /**This method checks input and output groupworkspace for an algorithm is of same name.
    *  @param props a list of properties for the algorithm
    *  @param ingroupwsName  input groupworkspace
    */ 
    void Algorithm::getInputGroupWorkspaceName(const std::vector<Mantid::Kernel::Property*>& props,std::string& ingroupwsName)
    {
      std::vector<Mantid::Kernel::Property*>::const_iterator itr;
      for (itr=props.begin();itr!=props.end();++itr)
      {  	       
        if(isInputWorkspaceProperty(*itr))
        { 
          try
          {
            if(WorkspaceGroup_sptr wsGrpSptr=boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve((*itr)->value())))
            {
              ingroupwsName=(*itr)->value();
              break;
            }

          }
          catch(Kernel::Exception::NotFoundError&)
          {
            throw std::runtime_error("Workspace "+ (*itr)->value()+ "not loaded");
          }

        }
      }
    }

    /** This method checks input and output groupworkspace for an algorithm is of same name.
    *  @param props a list of properties for the algorithm
    *  @returns true if the input and output groupworkspaces are of same names
    */ 
    bool Algorithm::isInputequaltoOutPut(const std::vector<Mantid::Kernel::Property*>& props)
    {
      std::vector<Mantid::Kernel::Property*>::const_iterator citr;
      std::string inputwsName;std::string outputwsName;
      for(citr=props.begin();citr!=props.end();++citr)
      {

        if(isInputWorkspaceProperty(*citr))
        {
          inputwsName=(*citr)->value();
        }
        if(isOutputWorkspaceProperty(*citr))
        {
          outputwsName=(*citr)->value();
        }

      }
      return (!inputwsName.compare(outputwsName)? true:false);
    }


    /**This method checks the member workspace are of similar names in a group workspace .
    *  @param ingroupwsName input group workspace name
    *  @param grpmembersNames a list of group member names
    *  @returns true if workspaces are of similar names
    */ 
    bool Algorithm::isGroupWorkspacesofSimilarNames(const std::string& ingroupwsName,const std::vector<std::string>& grpmembersNames)
    {
      if(grpmembersNames.empty()) return false;

      bool bsimilar(true);
      //check all the members are of similar names
      std::vector<std::string>::const_iterator citr;
      for(citr=grpmembersNames.begin();citr!=grpmembersNames.end();++citr)
      {
        bool b;
        std::size_t pos=(*citr).find_last_of("_");
        if(pos==std::string::npos)
        {
          b=false;
        }
        std::string commonpart((*citr).substr(0,pos));
        if(!ingroupwsName.compare(commonpart))
        {
          b=true;

        }
        else
        {
          b=false;
        }
        bsimilar= bsimilar&&b;
      }
      return(bsimilar?true:false);

    }
    /**virtual method to set the non workspace properties for this algorithm.
    *  @param alg pointer to the algorithm
    *  @param propertyName name of the property
    *  @param propertyValue value  of the property
    *  @param nPeriod period number
    */ 
    void Algorithm::setOtherProperties(IAlgorithm* alg,const std::string & propertyName,const std::string &propertyValue,int nPeriod)
    {
      (void) nPeriod; //Avoid compiler warning
      if(alg) alg->setPropertyValue(propertyName,propertyValue);
    }

    /** Setting input workspace properties for an algorithm,for handling workspace groups.
    *  @param pAlg  pointer to algorithm
    *  @param  prop  pointer to a vector holding the input properties
    *  @param  inMemberWSName input workspace name
    *  @returns true - if property is set .
    */
    bool  Algorithm::setInputWSProperties(IAlgorithm* pAlg,Mantid::Kernel::Property* prop,const std::string& inMemberWSName )
    {
      if(!pAlg) return false;
      if(!prop)return false;
      try
      {
        pAlg->setPropertyValue(prop->name(), inMemberWSName);
      }
      catch(std::invalid_argument&)
      {
        return false;
      }
      return true;

    }
    /** setting output workspace properties for an algorithm,
    *  This used for processing workspace groups and this  method is called when
    *  the input and out workspaces are not same
    *  @param pAlg      pointer to algorithm
    *  @param prop      pointer to the input properties
    *  @param nPeriod   period number 
    *  @param inmemberwsName outputworksapce name
    *  @param outwsgrp_sptr shared pointer for workspacegroup
    *  @param bSimilarNames -true if the input member workspaces are of similar names
    *  @param bequal -true if both the input and output group workspaces are same
    *  @returns true if property is set 
    */
    bool Algorithm::setOutputWSProperties(IAlgorithm* pAlg,Mantid::Kernel::Property* prop,const int nPeriod,const std::string& inmemberwsName,
      WorkspaceGroup_sptr& outwsgrp_sptr,bool bSimilarNames,bool bequal)
    {
      if(!prop) return false;
      if(!outwsgrp_sptr) return false;
      //output group workspace name
      std::string outgrpwsName=prop->value();
      // period number
      std::stringstream suffix;
      suffix<<nPeriod;
      //member workspace name
      std::string outmemberwsName;
      //if input and ouput group workspace name same
      if(bequal)
      {
        outmemberwsName=inmemberwsName;               //InputPut GroupWs ="NewGroup",//OutPut GroupWs ="NewGroup"
        // input member ws1= "Bob", // output member ws1="Bob_NewGroup" 
        //input member ws2 = "Sally",// output member ws2="Sally_NewGroup" 
      }
      else
      {	
        if(bSimilarNames) //if group are of similar names
        {
          outmemberwsName=outgrpwsName+"_"+suffix.str(); // input group ws ="Group"  ,//OutPut GroupWs ="NewGroup"
          //input member ws1="Group_1",//output member ws1="NewGroup_1"
          //input member ws2="Group_2",//output member ws2="NewGroup_2" 
        }	
        else
        {
          outmemberwsName= inmemberwsName+"_"+outgrpwsName;//InputPut GroupWs ="Group",//OutPut GroupWs ="NewGroup"
          // input member ws1 = "Bob", // output member ws1="Bob_NewGroup" 
          //input member ws2 = "Sally",// output member ws2="Sally_NewGroup" 
        }
      }
      if(nPeriod==1)
      {//group workspace is saving to ADS
        AnalysisDataService::Instance().addOrReplace(outgrpwsName,outwsgrp_sptr );
      } 
      unsigned int direction = prop->direction();
      if ((direction == Kernel::Direction::Output||direction== Kernel::Direction::InOut)) 
      {
        try
        {
          pAlg->setPropertyValue(prop->name(), outmemberwsName);
        }
        catch(std::invalid_argument&)
        {
          return false;
        }
      }
      //adding  member workspace to group vector
      g_log.information()<< outmemberwsName<<" adding to group workspace"<<std::endl;
      outwsgrp_sptr->add(outmemberwsName);
      return true;
    }

    /** To query the property is a workspace property
    *  @param prop pointer to input properties
    *  @returns true if this is a workspace property
    */
    bool Algorithm::isWorkspaceProperty( const Kernel::Property* const prop) const
    {
      if(!prop)
      {
        return false;
      }
      const IWorkspaceProperty * const wsProp = dynamic_cast<const IWorkspaceProperty* const>(prop);
      return (wsProp ? true : false);	
    }

    /** checks the property is a input workspace property
    * @param prop pointer to the input properties
    *  @returns true if this is a input workspace property
    */
    bool Algorithm::isInputWorkspaceProperty(const Kernel::Property* const prop) const
    {	
      if(!prop)
      {
        return false;
      }
      const IWorkspaceProperty * const wsProp = dynamic_cast<const IWorkspaceProperty* const>(prop);
      if(!wsProp) return false;
      unsigned int direction = prop->direction();
      return ((direction == Kernel::Direction::Input || direction==Kernel::Direction::InOut)?true :false);

    }
    /** checks the property is a output workspace property
    * @param prop pointer to input  properties
    * @returns true if this is a output workspace property
    */
    bool Algorithm::isOutputWorkspaceProperty(const Kernel::Property* const prop) const
    {
      if(!prop)
      {
        return false;
      }
      const IWorkspaceProperty * const wsProp = dynamic_cast<const IWorkspaceProperty* const>(prop);
      if(!wsProp) return false;
      unsigned int direction = prop->direction();
      return((direction == Kernel::Direction::Output || direction==Kernel::Direction::InOut)? true :false);
    }


    namespace
    {
      /**
      * A object to set the flag marking asynchronous running correctly 
      */
      struct AsyncFlagHolder
      {
        /** Constructor
        * @param A reference to the running flag
        */
        AsyncFlagHolder(bool & running_flag) : m_running_flag(running_flag)
        {
          m_running_flag = true;
        }
        ///Destructor
        ~AsyncFlagHolder()
        {
          m_running_flag = false;
        }
      private:
        ///Default constructor
        AsyncFlagHolder();
        ///Running flag
        bool & m_running_flag;
      };

    }

    /**
    * Callback when an algorithm is executed asynchronously
    */
    bool Algorithm::executeAsyncImpl(const Poco::Void&)
    {
      AsyncFlagHolder running(m_runningAsync);
      return this->execute();
    }

    /** Handles and rescales child algorithm progress notifications.
    *  @param pNf The progress notification from the child algorithm.
    */
    void Algorithm::handleChildProgressNotification(const Poco::AutoPtr<ProgressNotification>& pNf)
    {
      double p = m_startChildProgress + (m_endChildProgress - m_startChildProgress)*pNf->progress;
      progress(p,pNf->message);
    }

  } // namespace API
} // namespace Mantid
