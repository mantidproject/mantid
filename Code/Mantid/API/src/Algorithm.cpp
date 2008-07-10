//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

#include "MantidAPI/AnalysisDataService.h"

namespace Mantid
{
  namespace API
  {

    // Get a reference to the logger
    Kernel::Logger& Algorithm::g_log = Kernel::Logger::get("Algorithm");

    /// Constructor
    Algorithm::Algorithm() :
    PropertyManager(),
      m_isInitialized(false),
      m_isExecuted(false),
      m_isChildAlgorithm(false)
    {
    }

    /// Virtual destructor
    Algorithm::~Algorithm()
    {
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
      if (m_isInitialized)
        return;

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
        // Gaudi:
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
      clock_t start,end;
      time_t start_time;
      // Return a failure if the algorithm hasn't been initialized
      if ( !isInitialized() )
      {
        g_log.error("Algorithm is not initialized:" + this->name());
        throw std::runtime_error("Algorithm is not initialised:" + this->name());
      }

      // Check all properties for validity
      if ( !validateProperties() )
      {
        throw std::runtime_error("Some invalid Properties found");
      }

      // Invoke exec() method of derived class and catch all uncaught exceptions
      try
      {
        try
        {
          time(&start_time);
          start = clock();
          // no logging of input if a child algorithm
          if (!m_isChildAlgorithm) algorithm_info();
          // Call the concrete algorithm's exec method
          this->exec();
          end = clock();
          // need it to throw before trying to run fillhistory() on an algorithm which has failed
          // Put any output workspaces into the AnalysisDataService - if this is not a child algorithm
          if (!isChild())
          {
            fillHistory(start_time,double(end - start)/CLOCKS_PER_SEC);
            this->store();
          }

          // RJT, 19/3/08: Moved this up from below the catch blocks
          setExecuted(true);
		  if (!m_isChildAlgorithm)
			  g_log.information()<< "Algorithm successful, Duration "<< double(end - start)/CLOCKS_PER_SEC << " seconds" << std::endl;

        }
        catch(std::runtime_error& ex)
        {
          g_log.error()<< "Error in Execution of algorithm "<< this->name()<<std::endl;
          g_log.error()<< ex.what()<<std::endl;
          if (m_isChildAlgorithm) throw;
        }
        catch(std::logic_error& ex)
        {
          g_log.error()<< "Logic Error in Execution of algorithm "<< this->name()<<std::endl;
          g_log.error()<< ex.what()<<std::endl;
          if (m_isChildAlgorithm) throw;
        }

      }

      // Gaudi also specifically catches GaudiException & std:exception.
      catch (...)
      {
        // Gaudi sets the executed flag to true here despite the exception
        // This allows it to move to the next command or it just loops indefinitely.
        // we will set it to false (see Nick Draper) 6/12/07
        setExecuted(false);

        g_log.error("UNKNOWN Exception is caught ");
        throw;
        // Gaudi calls exception service 'handle' method here
      }

      // Gaudi has some stuff here where it tests for failure, increments the error counter
      // and then converts to success if less than the maximum. This is clearly related to
      // having an event loop, and thus we shouldn't want it. This is the only place it's used.


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
    *  @param name    The concrete algorithm class of the sub algorithm
    *  @returns Set to point to the newly created algorithm object
    */
    Algorithm_sptr Algorithm::createSubAlgorithm(const std::string& name)
    {
      Algorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged(name);
      //set as a child
      alg->setChild(true);

      // Initialise the sub-algorithm
      try
      {
        alg->initialize();
      }
      catch (std::runtime_error& err)
      {
        g_log.error() << "Unable to initialise sub-algorithm " << name << std::endl;
      }

      return alg;
    }

    // IAlgorithm property methods. Pull in PropertyManager implementation.
    void Algorithm::setPropertyValue(const std::string &name, const std::string &value)
    {
      PropertyManager::setPropertyValue(name, value);
    }

    // IAlgorithm property methods. Pull in PropertyManager implementation.
    void Algorithm::setPropertyOrdinal(const int &index, const std::string &value)
    {
      PropertyManager::setPropertyOrdinal(index, value);
    }


    std::string Algorithm::getPropertyValue(const std::string &name) const
    {
      return PropertyManager::getPropertyValue(name);
    }

    //----------------------------------------------------------------------
    // Protected Member Functions
    //----------------------------------------------------------------------

    /// Set the Algorithm initialized state
    void Algorithm::setInitialized()
    {
      m_isInitialized = true;
    }

    /// Set the executed flag to the specified state
    // Public in Gaudi - don't know why and will leave here unless we find a reason otherwise
    //     Also don't know reason for different return type and argument.
    void Algorithm::setExecuted(bool state)
    {
      m_isExecuted = state;
    }

    //----------------------------------------------------------------------
    // Private Member Functions
    //----------------------------------------------------------------------

    /** Fills History, Algorithm History and Algorithm Parameters
    *  @param start a date and time defnining the start time of the algorithm
    *  @param duration a double defining the length of duration of the algorithm
    */
    void Algorithm::fillHistory(dateAndTime start,double duration)
    {
      //std::vector<AlgorithmParameter>* algParameters = new std::vector<AlgorithmParameter>;
      //references didn't work as not initialised
      std::vector<WorkspaceHistory>  inW_History,outW_History,inoutW_History,OUT_inoutW_History;
      Workspace_sptr out_work, in_work, inout_work;
      std::vector<AlgorithmParameter> algParameters;
      bool iflag(false) ,ioflag(false),oflag(false);

      const std::vector<Property*>& algProperties = getProperties();

      for (unsigned int i = 0; i < algProperties.size(); ++i)
      {
        IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty*>(algProperties[i]);
        if (wsProp)
        {
          const unsigned int i = wsProp->direction();
          if( (i == Mantid::Kernel::Direction::InOut) )
          {
            ioflag=true;
            inout_work = wsProp->getWorkspace();
            inoutW_History.push_back( inout_work->getWorkspaceHistory());
          }

          else if( (i == Mantid::Kernel::Direction::Input) )
          {
            iflag=true;
            in_work = wsProp->getWorkspace();
            inW_History.push_back( in_work->getWorkspaceHistory());
          }
          else if( (i == Mantid::Kernel::Direction::Output) )
          {
            oflag=true;
            out_work = wsProp->getWorkspace();
            outW_History.push_back(out_work->getWorkspaceHistory());
          }
        }
      }
/*
fill each output with each input history
fill each inout with each OTHER inout history
fill each inout with each input history
*/

      if(iflag)
      {
        //loop over input workspaces to fill output workspace history with constituent input histories
        for (unsigned int j=0; j<inW_History.size();j++)
        {
          std::vector<AlgorithmHistory>& in_algH = inW_History[j].getAlgorithms();
          if(  in_algH.size() != 0)
          {
            //loop over number of output workspaces
            for (unsigned int i=0; i<outW_History.size();i++)
            {
              // copy each algorithmhistory from each input workspace into the out history
              for(unsigned int k=0; k<in_algH.size();k++)
              {
                std::vector<AlgorithmHistory>& out_algH = outW_History[i].getAlgorithms();
                out_algH.push_back(in_algH[k]);
              }
            }
          }
        }
      }

      if(ioflag)
      {
        //loop over inout workspaces to fill output workspace history with constituent inout histories
        for (unsigned int j=0; j<inoutW_History.size();j++)
        {
          std::vector<AlgorithmHistory>& inout_algH = inoutW_History[j].getAlgorithms();
          if(  inout_algH.size() != 0)
          {
            //loop over number of output workspaces
            for (unsigned int i=0; i<outW_History.size();i++)
            {
              // copy each algorithmhistory from each inout workspace into the out history
              for(unsigned int k=0; k<inout_algH.size();k++)
              {
                std::vector<AlgorithmHistory>& out_algH = outW_History[i].getAlgorithms();
                out_algH.push_back(inout_algH[k]);
              }
            }
          }
        }
      }


      if(ioflag)
      {
        //loop over inout workspaces to fill inout workspace history other inout histories
        for (unsigned int j=0; j<inoutW_History.size();j++)
        {
          std::vector<AlgorithmHistory>& inout_algH = inoutW_History[j].getAlgorithms();
          if(  inout_algH.size() != 0)
          {
            for (unsigned int i=0; i<inoutW_History.size();i++)
            {
              // don't want to copy the history of an inout workspace into itself
              if(i!=j)
              {
                // copy each algorithmhistory from each inout workspace into each inout history (except for the same one)
                for(unsigned int k=0; k<inout_algH.size();k++)
                {
                  std::vector<AlgorithmHistory>& out_algH = inoutW_History[i].getAlgorithms();
                  out_algH.push_back(inout_algH[k]);
                }
              }
            }
          }
        }
      }
      if(iflag && ioflag)
      {
        //loop over input workspaces to fill inout workspace history with constituent input histories
        for (unsigned int j=0; j<inW_History.size();j++)
        {
          std::vector<AlgorithmHistory>& in_algH = inW_History[j].getAlgorithms();
          if(  in_algH.size() != 0)
          {
            //loop over number of output workspaces
            for (unsigned int i=0; i<inoutW_History.size();i++)
            {
              // copy each algorithmhistory from each input workspace into the out history
              for(unsigned int k=0; k<in_algH.size();k++)
              {
                std::vector<AlgorithmHistory>& out_algH = inoutW_History[i].getAlgorithms();
                out_algH.push_back(in_algH[k]);
              }
            }
          }
        }
      }


      //

      int no_of_props = algProperties.size();
      for (int i=0; i < no_of_props; i++)
      {
        IWorkspaceProperty* wsProp = dynamic_cast<IWorkspaceProperty*>(algProperties[i]);
        // could not do dynamic cast on const AP so do a cast on alProperties instead
        const Property* AP=algProperties[i];
        if (AP && wsProp)
        {
          algParameters.push_back(AlgorithmParameter(AP->name(),
            AP->value(),AP->type(),AP->isDefault(),wsProp->direction()));
        }
        else if(AP && !wsProp)
        {
          algParameters.push_back(AlgorithmParameter(AP->name(),
            AP->value(),AP->type(),AP->isDefault(),Mantid::Kernel::Direction::None));
        }
      }

      if(oflag)
      {
        // get the reference  to output workspace without it going out of scope, and it's not a copy
        std::vector<AlgorithmHistory>& OalgHistory = (outW_History.front()).getAlgorithms();
        //increment output workspace history with new algorithm properties
        OalgHistory.push_back(AlgorithmHistory(this->name(),this->version(),start,duration,algParameters));
        WorkspaceHistory& Ohist = out_work->getWorkspaceHistory();
        std::vector<AlgorithmHistory>& Oalg = Ohist.getAlgorithms();
        Oalg=OalgHistory;
      }

      if(ioflag)
      {
        // get the reference  to input workspace without it going out of scope, and it's not a copy
        std::vector<AlgorithmHistory>& IOalgHistory = (inoutW_History.front()).getAlgorithms();
        //increment output workspace history with new algorithm properties
        IOalgHistory.push_back(AlgorithmHistory(this->name(),this->version(),start,duration,algParameters));
        WorkspaceHistory& IOhist = inout_work->getWorkspaceHistory();
        std::vector<AlgorithmHistory>& IOalg = IOhist.getAlgorithms();
        IOalg=IOalgHistory;
      }
    }

    /** puts out algorithm parameter information to the logger
    */
	void Algorithm::algorithm_info()
	{
		g_log.information()<<"Algorithm Name " << this->name() <<" Version "<<this->version()<<std::endl;
		const std::vector<Property*>& algProperties = getProperties();
		int no_of_props = algProperties.size();
		for (int i=0; i < no_of_props; i++)
		{
			const Property* AP=algProperties[i];
			g_log.information() << "Name: " << AP->name() << ", Value: " << AP->value()
			                    << ", Default: "<< (AP->isDefault()?"Yes":"No") <<std::endl;
		}
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
          catch (std::runtime_error& e)
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


  } // namespace API
} // namespace Mantid
