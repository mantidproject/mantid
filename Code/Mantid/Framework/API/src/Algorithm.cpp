//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/MemoryManager.h"

#include "MantidKernel/Timer.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Strings.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/regex.hpp>

#include <Poco/StringTokenizer.h>

#include <iomanip>
#include <map>

using namespace Mantid::Kernel;


namespace Mantid
{
  namespace API
  {
    // Doxygen can't handle member specialization at the moment: https://bugzilla.gnome.org/show_bug.cgi?id=406027
    // so we have to ignore them
    ///@cond
    template <typename NumT>
    bool Algorithm::isEmpty(const NumT toCheck)
    {
      return static_cast<int>(toCheck) == EMPTY_INT();
    }

    template <>
    MANTID_API_DLL
    bool Algorithm::isEmpty(const double toCheck)
    {
      return std::abs( (toCheck - EMPTY_DBL())/(EMPTY_DBL()) ) < 1e-8  ;
    }

    // concrete instantiations
    template MANTID_API_DLL bool Algorithm::isEmpty<int> (const int);
    template MANTID_API_DLL bool Algorithm::isEmpty<int64_t> (const int64_t);
    template MANTID_API_DLL bool Algorithm::isEmpty<std::size_t> (const std::size_t);
    ///@endcond

    //=============================================================================================
    //================================== Constructors/Destructors =================================
    //=============================================================================================

    /// Initialize static algorithm counter
    size_t Algorithm::g_execCount=0;

    /// Constructor
    Algorithm::Algorithm() :
    PropertyManagerOwner(),m_progressObserver(*this, &Algorithm::handleChildProgressNotification),
      m_cancel(false),m_parallelException(false),g_log(Kernel::Logger::get("Algorithm")),
      m_executeAsync(this,&Algorithm::executeAsyncImpl),m_isInitialized(false),
      m_isExecuted(false),m_isChildAlgorithm(false), m_recordHistoryForChild(false),
      m_alwaysStoreInADS(false),m_runningAsync(false),
      m_running(false),m_rethrow(false),m_algorithmID(this),
      m_singleGroup(-1), m_groupSize(0), m_groupsHaveSimilarNames(false)
    {
    }

    /// Virtual destructor
    Algorithm::~Algorithm()
    {
      g_log.release();
      // Free up any memory available.
      Mantid::API::MemoryManager::Instance().releaseFreeMemory();
    }


    //=============================================================================================
    //================================== Simple Getters/Setters ===================================
    //=============================================================================================



    //---------------------------------------------------------------------------------------------
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

    //---------------------------------------------------------------------------------------------
    /// Set the Algorithm initialized state
    void Algorithm::setInitialized()
    {
      m_isInitialized = true;
    }

    /** Set the executed flag to the specified state
    // Public in Gaudi - don't know why and will leave here unless we find a reason otherwise
    //     Also don't know reason for different return type and argument.
    @param state :: New executed state
    */
    void Algorithm::setExecuted(bool state)
    {
      m_isExecuted = state;
    }

    //---------------------------------------------------------------------------------------------
    /** To query whether algorithm is a child.
    *  @returns true - the algorithm is a child algorithm.  False - this is a full managed algorithm.
    */
    bool Algorithm::isChild() const
    {
      return m_isChildAlgorithm;
    }

    /** To set whether algorithm is a child.
    *  @param isChild :: True - the algorithm is a child algorithm.  False - this is a full managed algorithm.
    */
    void Algorithm::setChild(const bool isChild)
    {
      m_isChildAlgorithm = isChild;
    }

    /**
     * Change the state of the history recording flag. Only applicable for
     * child algorithms. 
     * @param on :: The new state of the flag
     */
     void Algorithm::enableHistoryRecordingForChild(const bool on)
     {
       m_recordHistoryForChild = on;
     }

    /** Do we ALWAYS store in the AnalysisDataService? This is set to true
     * for python algorithms' child algorithms
     *
     * @param doStore :: always store in ADS
     */
    void Algorithm::setAlwaysStoreInADS(const bool doStore)
    {
      m_alwaysStoreInADS = doStore;
    }


    /** Set whether the algorithm will rethrow exceptions
     * @param rethrow :: true if you want to rethrow exception.
     */
    void Algorithm::setRethrows(const bool rethrow)
    {
      this->m_rethrow = rethrow;
    }

    /// True if the algorithm is running.
    bool Algorithm::isRunning() const
    {
      Poco::FastMutex::ScopedLock _lock(m_mutex);
      return m_running;
    }


    //---------------------------------------------------------------------------------------------
    /**  Add an observer to a notification
    @param observer :: Reference to the observer to add
    */
    void Algorithm::addObserver(const Poco::AbstractObserver& observer)const
    {
      m_notificationCenter.addObserver(observer);
    }

    /**  Remove an observer
    @param observer :: Reference to the observer to remove
    */
    void Algorithm::removeObserver(const Poco::AbstractObserver& observer)const
    {
      m_notificationCenter.removeObserver(observer);
    }

    //---------------------------------------------------------------------------------------------
    /** Sends ProgressNotification.
     * @param p :: Reported progress,  must be between 0 (just started) and 1 (finished)
     * @param msg :: Optional message string
     * @param estimatedTime :: Optional estimated time to completion
     * @param progressPrecision :: optional, int number of digits after the decimal point to show.
    */
    void Algorithm::progress(double p, const std::string& msg, double estimatedTime, int progressPrecision)
    {
      m_notificationCenter.postNotification(new ProgressNotification(this,p,msg, estimatedTime, progressPrecision));
    }

    //---------------------------------------------------------------------------------------------
    /// Function to return all of the categories that contain this algorithm
    const std::vector<std::string> Algorithm::categories() const
    {
      std::vector < std::string > res;
      Poco::StringTokenizer tokenizer(category(), categorySeparator(),
          Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_IGNORE_EMPTY);
      Poco::StringTokenizer::Iterator h = tokenizer.begin();

      for (; h != tokenizer.end(); ++h)
      {
        res.push_back(*h);
      }

      const DeprecatedAlgorithm * depo = dynamic_cast<const DeprecatedAlgorithm *>(this);
      if (depo != NULL)
      {
        res.push_back("Deprecated");
      }
      return res;
    }


    //=============================================================================================
    //================================== Initialization ===========================================
    //=============================================================================================

    //---------------------------------------------------------------------------------------------
    /** Initialization method invoked by the framework. This method is responsible
    *  for any bookkeeping of initialization required by the framework itself.
    *  It will in turn invoke the init() method of the derived algorithm,
    *  and of any Child Algorithms which it creates.
    *  @throw runtime_error Thrown if algorithm or Child Algorithm cannot be initialised
    *
    */
    void Algorithm::initialize()
    {
      // Bypass the initialization if the algorithm has already been initialized.
      if (m_isInitialized) return;

      g_log.setName(this->name());
      try
      {
        try
        {
          this->init();
        }
        catch(std::runtime_error& ex)
        {
          g_log.error() << "Error initializing " << this->name() << " algorithm: " << ex.what() << std::endl;
          throw;
        }

        // Indicate that this Algorithm has been initialized to prevent duplicate attempts.
        setInitialized();
      }
      catch(std::runtime_error& )
      {
        throw;
      }
      // Unpleasant catch-all! Along with this, Gaudi version catches GaudiException & std::exception
      // but doesn't really do anything except (print fatal) messages.
      catch (...)
      {
        // Gaudi: A call to the auditor service is here
        // (1) perform the printout
        g_log.fatal("UNKNOWN Exception is caught in initialize()");
        throw;
      }

      // Set the documentation. This virtual method is overridden by (nearly) all algorithms and gives documentation summary.
      initDocs();
    }

    //---------------------------------------------------------------------------------------------
    /** Perform validation of ALL the input properties of the algorithm.
     * This is to be overridden by specific algorithms.
     * It will be called in dialogs after parsing all inputs and setting the
     * properties, but BEFORE executing.
     *
     * @return a map where: Key = string name of the the property;
                Value = string describing the problem with the property.
     */
    std::map<std::string, std::string> Algorithm::validateInputs()
    {
      return std::map<std::string, std::string>();
    }

    //---------------------------------------------------------------------------------------------
    /** Go through the properties and cache the input/output
     * workspace properties for later use.
     */
    void Algorithm::cacheWorkspaceProperties()
    {
      // Cache the list of the in/out workspace properties
      m_inputWorkspaceProps.clear();
      m_outputWorkspaceProps.clear();
      m_pureOutputWorkspaceProps.clear();
      const std::vector<Property*>& props = this->getProperties();
      for (size_t i=0; i<props.size(); i++)
      {
        Property* prop = props[i];
        IWorkspaceProperty * wsProp = dynamic_cast<IWorkspaceProperty*>(prop);
        if (wsProp)
        {
          switch(prop->direction())
          {
          case Kernel::Direction::Input:
            m_inputWorkspaceProps.push_back(wsProp);
            break;
          case Kernel::Direction::InOut:
            m_inputWorkspaceProps.push_back(wsProp);
            m_outputWorkspaceProps.push_back(wsProp);
            break;
          case Kernel::Direction::Output:
            m_outputWorkspaceProps.push_back(wsProp);
            m_pureOutputWorkspaceProps.push_back(wsProp);
            break;
          default:
            throw std::logic_error("Unexpected property direction found for property " + prop->name() + " of algorithm " + this->name());
          }
        } // is a ws property
      } // each property
    }

    //=============================================================================================
    //================================== Execution ================================================
    //=============================================================================================

    //---------------------------------------------------------------------------------------------
    /** Go through the workspace properties of this algorithm
     * and lock the workspaces for reading or writing.
     *
     */
    void Algorithm::lockWorkspaces()
    {
      // Do not lock workspace for child algos
      if (this->isChild())
        return;

      if (!m_readLockedWorkspaces.empty() || !m_writeLockedWorkspaces.empty())
        throw std::logic_error("Algorithm::lockWorkspaces(): The workspaces have already been locked!");

      // First, Write-lock the output workspaces
      for (size_t i=0; i<m_outputWorkspaceProps.size(); i++)
      {
        Workspace_sptr ws = m_outputWorkspaceProps[i]->getWorkspace();
        if (ws)
        {
          // The workspace property says to do locking,
          // AND it has NOT already been write-locked
          if (m_outputWorkspaceProps[i]->isLocking()
              && std::find(m_writeLockedWorkspaces.begin(), m_writeLockedWorkspaces.end(), ws) == m_writeLockedWorkspaces.end())
          {
            // Write-lock it if not already
            g_log.debug() << "Write-locking " << ws->getName() << std::endl;
            ws->getLock()->writeLock();
            m_writeLockedWorkspaces.push_back(ws);
          }
        }
      }

      // Next read-lock the input workspaces
      for (size_t i=0; i<m_inputWorkspaceProps.size(); i++)
      {
        Workspace_sptr ws = m_inputWorkspaceProps[i]->getWorkspace();
        if (ws)
        {
          // The workspace property says to do locking,
          // AND it has NOT already been write-locked
          if (m_inputWorkspaceProps[i]->isLocking()
              && std::find(m_writeLockedWorkspaces.begin(), m_writeLockedWorkspaces.end(), ws) == m_writeLockedWorkspaces.end())
          {
            // Read-lock it if not already write-locked
            g_log.debug() << "Read-locking " << ws->getName() << std::endl;
            ws->getLock()->readLock();
            m_readLockedWorkspaces.push_back(ws);
          }
        }
      }
    }

    //---------------------------------------------------------------------------------------------
    /** Unlock any previously locked workspaces
     *
     */
    void Algorithm::unlockWorkspaces()
    {
      // Do not lock workspace for child algos
      if (this->isChild())
        return;

      for (size_t i=0; i<m_writeLockedWorkspaces.size(); i++)
      {
        Workspace_sptr ws = m_writeLockedWorkspaces[i];
        if (ws)
        {
          g_log.debug() << "Unlocking " << ws->getName() << std::endl;
          ws->getLock()->unlock();
        }
      }
      for (size_t i=0; i<m_readLockedWorkspaces.size(); i++)
      {
        Workspace_sptr ws = m_readLockedWorkspaces[i];
        if (ws)
        {
          g_log.debug() << "Unlocking " << ws->getName() << std::endl;
          ws->getLock()->unlock();
        }
      }

      // Don't double-unlock workspaces
      m_readLockedWorkspaces.clear();
      m_writeLockedWorkspaces.clear();
    }

    //---------------------------------------------------------------------------------------------
    /** The actions to be performed by the algorithm on a dataset. This method is
    *  invoked for top level algorithms by the application manager.
    *  This method invokes exec() method.
    *  For Child Algorithms either the execute() method or exec() method
    *  must be EXPLICITLY invoked by  the parent algorithm.
    *
    *  @throw runtime_error Thrown if algorithm or Child Algorithm cannot be executed
    *  @return true if executed successfully.
    */
    bool Algorithm::execute()
    {
      AlgorithmManager::Instance().notifyAlgorithmStarting(this->getAlgorithmID());
      {
        DeprecatedAlgorithm * depo = dynamic_cast<DeprecatedAlgorithm *>(this);
        if (depo != NULL)
          g_log.error(depo->deprecationMsg(this));
      }
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

      // Cache the workspace in/out properties for later use
      cacheWorkspaceProperties();

      // no logging of input if a child algorithm (except for python child algos)
      if (!m_isChildAlgorithm || m_alwaysStoreInADS) logAlgorithmInfo();

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
            // Setting it's name to the same one it already had
            props[i]->setValue(props[i]->value());
          }
        }
        // Try the validation again
        if ( !validateProperties() )
        {
           m_notificationCenter.postNotification(new ErrorNotification(this,"Some invalid Properties found"));
           throw std::runtime_error("Some invalid Properties found");
        }
      }

      // ----- Perform validation of the whole set of properties -------------
      std::map<std::string, std::string> errors = this->validateInputs();
      if (!errors.empty())
      {
        // Log each issue
        for (auto it = errors.begin(); it != errors.end(); it++)
          g_log.error() << "Invalid value for " << it->first << ": " << it->second << std::endl;
        // Throw because something was invalid
        m_notificationCenter.postNotification(new ErrorNotification(this,"Some invalid Properties found"));
        throw std::runtime_error("Some invalid Properties found");
      }

      // ----- Check for processing groups -------------
      // default true so that it has the right value at the check below the catch block should checkGroups throw
      bool callProcessGroups = true;
      try
      {
        // Checking the input is a group. Throws if the sizes are wrong
        callProcessGroups = this->checkGroups();
        if (callProcessGroups)
        {
          // This calls this->execute() again on each member of the group.
          return processGroups();
        }
      }
      catch(std::exception& ex)
      {
        g_log.error()<< "Error in execution of algorithm "<< this->name()<<std::endl;
        g_log.error()<<ex.what()<<std::endl;
        m_notificationCenter.postNotification(new ErrorNotification(this,ex.what()));
        m_running = false;
        if (m_isChildAlgorithm || m_runningAsync || m_rethrow)
        {
          m_runningAsync = false;
          throw;
        }
      }
      // If checkGroups() threw an exception but there ARE group workspaces
      // (means that the group sizes were incompatible)
      if (callProcessGroups)
        return false;

      // Read or write locks every input/output workspace
      this->lockWorkspaces();

      // Invoke exec() method of derived class and catch all uncaught exceptions
      try
      {
        try
        {
          if (!isChild()) 
          { 
            Poco::FastMutex::ScopedLock _lock(m_mutex);
            m_running = true;
          }
          if(!isChild() || m_recordHistoryForChild)
          {
            // count used for defining the algorithm execution order
            // If history is being recorded we need to count this as a separate algorithm
            // as the history compares histories by their execution number
            ++Algorithm::g_execCount;
          }

          start_time = Mantid::Kernel::DateAndTime::getCurrentTime();
          // Start a timer
          Timer timer;
          // Call the concrete algorithm's exec method
          this->exec();
          // Check for a cancellation request in case the concrete algorithm doesn't
          interruption_point();			
          // Get how long this algorithm took to run
          const float duration = timer.elapsed();

          // need it to throw before trying to run fillhistory() on an algorithm which has failed
          if(!isChild() || m_recordHistoryForChild)
            fillHistory(start_time,duration,Algorithm::g_execCount);

          // Put any output workspaces into the AnalysisDataService - if this is not a child algorithm
          if (!isChild() || m_alwaysStoreInADS)
            this->store();

          // RJT, 19/3/08: Moved this up from below the catch blocks
          setExecuted(true);
          if (!m_isChildAlgorithm || m_alwaysStoreInADS)
          {
            g_log.notice() << name() << " successful, Duration "
              << std::fixed << std::setprecision(2) << duration << " seconds" << std::endl;
          }
          else
            g_log.debug() << name() << " finished with isChild = " << isChild() << std::endl;
          m_running = false;
        }
        catch(std::runtime_error& ex)
        {
          this->unlockWorkspaces();
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
          this->unlockWorkspaces();
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
        this->unlockWorkspaces();
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
        this->unlockWorkspaces();
        throw;
      }

      catch (...)
      {
        // Execution failed
        setExecuted(false);
        m_runningAsync = false;
        m_running = false;

        m_notificationCenter.postNotification(new ErrorNotification(this,"UNKNOWN Exception is caught in exec()"));
        g_log.error() << this->name() << ": UNKNOWN Exception is caught in exec()\n";
        this->unlockWorkspaces();
        throw;
      }

      // Unlock the locked workspaces
      this->unlockWorkspaces();

      m_notificationCenter.postNotification(new FinishedNotification(this,isExecuted()));
      // Only gets to here if algorithm ended normally
      // Free up any memory available.
      Mantid::API::MemoryManager::Instance().releaseFreeMemory();
      return isExecuted();
    }

    //---------------------------------------------------------------------------------------------
    /** Execute as a Child Algorithm.
     * This runs execute() but catches errors so as to log the name
     * of the failed Child Algorithm, if it fails.
     */
    void Algorithm::executeAsChildAlg()
    {
      bool executed = false;
      try
      {
        executed = execute();
      }
      catch (std::runtime_error&)
      {
        g_log.error() << "Unable to successfully run ChildAlgorithm " << this->name() << std::endl;
        throw;
      }

      if ( ! executed )
      {
        g_log.error() << "Unable to successfully run ChildAlgorithm " << this->name() << std::endl;
        throw std::runtime_error("Unable to successfully run ChildAlgorithm " + this->name());
      }
    }

    //---------------------------------------------------------------------------------------------
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

    //---------------------------------------------------------------------------------------------
    /** Create a Child Algorithm.  A call to this method creates a child algorithm object.
    *  Using this mechanism instead of creating daughter
    *  algorithms directly via the new operator is prefered since then
    *  the framework can take care of all of the necessary book-keeping.
    *
    *  @param name ::           The concrete algorithm class of the Child Algorithm
    *  @param startProgress ::  The percentage progress value of the overall algorithm where this child algorithm starts
    *  @param endProgress ::    The percentage progress value of the overall algorithm where this child algorithm ends
    *  @param enableLogging ::  Set to false to disable logging from the child algorithm
    *  @param version ::        The version of the child algorithm to create. By default gives the latest version.
    *  @return shared pointer to the newly created algorithm object
    */
    Algorithm_sptr Algorithm::createChildAlgorithm(const std::string& name, const double startProgress, const double endProgress,
      const bool enableLogging, const int& version)
    {
      Algorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged(name,version);
      //set as a child
      alg->setChild(true);
      alg->setLogging(enableLogging);

      // Initialise the Child Algorithm
      try
      {
        alg->initialize();
      }
      catch (std::runtime_error& exc)
      {
        g_log.error() << "Unable to initialise Child Algorithm " << name << std::endl;
        g_log.error() << exc.what() << "\n";
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
      // It must be protected by a critical block so that Child Algorithms can run in parallel safely.
      IAlgorithm_wptr weakPtr(alg);
      PARALLEL_CRITICAL(Algorithm_StoreWeakPtr)
      {
        m_ChildAlgorithms.push_back(weakPtr);
      }

      return alg;
    }

    //=============================================================================================
    //================================== Algorithm History ========================================
    //=============================================================================================


    /**
     * Serialize this object to a string. The format is
     * AlgorithmName.version(prop1=value1,prop2=value2,...)
     * @returns This object serialized as a string 
     */
    std::string Algorithm::toString() const
    {
      std::ostringstream writer;
      writer << name() << "." << this->version() 
        << "("
        << Kernel::PropertyManagerOwner::asString(false)
        << ")";
      return writer.str();
    }


    //--------------------------------------------------------------------------------------------
    /** Construct an object from a history entry.
     *
     * This creates the algorithm and sets all of its properties using the history.
     *
     * @param history :: AlgorithmHistory object
     * @return a shared pointer to the created algorithm.
     */
    IAlgorithm_sptr Algorithm::fromHistory(const AlgorithmHistory & history)
    {
      // Hand off to the string creator
      std::ostringstream stream;
      stream << history.name() << "." << history.version()
       << "(";
      const std::vector<Kernel::PropertyHistory>& props = history.getProperties();
      const size_t numProps(props.size());
      for( size_t i = 0 ; i < numProps; ++i )
      {
        const Kernel::PropertyHistory & prop = props[i];
        if( !prop.isDefault() )
        {
          stream << prop.name() << "=" << prop.value();
        }
        if( i < numProps - 1 ) stream << ",";
      }
      stream << ")";
      return Algorithm::fromString(stream.str());
    }


    //--------------------------------------------------------------------------------------------
    /** De-serializes the algorithm from a string
     *
    * @param input :: An input string in the format. The format is
    *        AlgorithmName.version(prop1=value1,prop2=value2,...). If .version is not found the
    *        highest found is used.
    * @return A pointer to a managed algorithm object
    */
    IAlgorithm_sptr Algorithm::fromString(const std::string & input)
    {
      static const boost::regex nameExp("(^[[:alnum:]]*)");
      // Double back slash gets rid of the compiler warning about unrecognized escape sequence
      static const boost::regex versExp("\\.([[:digit:]]+)\\(*");
      // Property regex. Simply match the brackets and split
      static const boost::regex propExp("\\((.*)\\)");
      // Name=Value regex
      static const boost::regex nameValExp("(.*)=(.*)");
      // Property name regex
      static const boost::regex propNameExp(".*,([[:word:]]*)");
      // Empty dividers
       static const boost::regex emptyExp(",[ ,]*,");
      // Trailing commas
      static const boost::regex trailingCommaExp(",$");

      boost::match_results<std::string::const_iterator> what;
      if( boost::regex_search(input, what, nameExp, boost::match_not_null) )
      {
        const std::string algName = what[1];
        int version = -1;  // Highest version
        if( boost::regex_search(input, what, versExp, boost::match_not_null) )
        {
          try
          {
            version = boost::lexical_cast<int, std::string>(what.str(1));
          }
          catch(boost::bad_lexical_cast&)
          {
          }
        }
        IAlgorithm_sptr alg = AlgorithmManager::Instance().create(algName, version);
        if(  boost::regex_search(input, what, propExp, boost::match_not_null) )
        {
          std::string _propStr = what[1];

          // Cleanup: Remove empty dividers (multiple commas)
          std::string __propStr = regex_replace(_propStr, emptyExp, "," );
          // Cleanup: We might still be left with a trailing comma, remove it
          std::string propStr = regex_replace(__propStr, trailingCommaExp, "" );

          boost::match_flag_type flags = boost::match_not_null;
          std::string::const_iterator start, end;
          start = propStr.begin();
          end = propStr.end();
          // Accumulate them first so that we can set some out of order
          std::map<std::string, std::string> propNameValues;
          while(boost::regex_search(start, end, what, nameValExp, flags))
          {
            std::string nameValue = what.str(1);
            std::string value = what.str(2);

            if(  boost::regex_search(what[1].first, what[1].second,
                 what, propNameExp, boost::match_not_null) )
            {
              const std::string name = what.str(1);
              propNameValues[name] = value;
              end = what[1].first-1;
            } else {
              // The last property-value pair
              propNameValues[nameValue] = value;
              break;
            }
            // update flags:
            flags |= boost::match_prev_avail;
            flags |= boost::match_not_bob;
          }

          // Some algorithms require Filename to be set first do that here
          auto it = propNameValues.find("Filename");
          if(it != propNameValues.end())
          {
            alg->setPropertyValue(it->first, it->second);
            propNameValues.erase(it);
          }
          for(auto cit = propNameValues.begin(); cit != propNameValues.end(); ++cit)
          {
            alg->setPropertyValue(cit->first, cit->second);
          }
        }
        return alg;
      }
      else
      {
        throw std::runtime_error("Cannot create algorithm, invalid string format.");
      }
    }

    //-------------------------------------------------------------------------
    /** Initialize using proxy algorithm.
     * Call the main initialize method and then copy in the property values.
     * @param proxy :: Initialising proxy algorithm  */
    void Algorithm::initializeFromProxy(const AlgorithmProxy& proxy)
    {
      initialize();
      copyPropertiesFrom(proxy);
      m_algorithmID = proxy.getAlgorithmID();
      setLogging(proxy.isLogging());
      setChild(proxy.isChild());
    }


    /** Fills History, Algorithm History and Algorithm Parameters
    *  @param start :: a date and time defnining the start time of the algorithm
    *  @param duration :: a double defining the length of duration of the algorithm
    *  @param  uexecCount an unsigned int for defining the excution order of algorithm
    */
    void Algorithm::fillHistory(Mantid::Kernel::DateAndTime start,double duration,std::size_t uexecCount)
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
          (*outWS)->history().addHistory( (*inWS)->getHistory() );
        }
        // Add the history for the current algorithm to all the output workspaces
        (*outWS)->history().addHistory(algHistory);
      }
    }

    /** Populate lists of the input & output workspace properties.
    *  (InOut workspaces go in both lists)
    *  @param inputWorkspaces ::  A reference to a vector for the input workspaces
    *  @param outputWorkspaces :: A reference to a vector for the output workspaces
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

    /** Sends out algorithm parameter information to the logger */
    void Algorithm::logAlgorithmInfo() const
    {
      g_log.notice() << name() << " started";
      if (this->isChild())
        g_log.notice() << " (child)";
      g_log.notice() << std::endl;
      // Make use of the AlgorithmHistory class, which holds all the info we want here
      AlgorithmHistory AH(this);
      g_log.information() << AH;
    }



    //=============================================================================================
    //================================== WorkspaceGroup-related ===================================
    //=============================================================================================

    /** Check the input workspace properties for groups.
     *
     * If there are more than one input workspace properties, then:
     *  - All inputs should be groups of the same size
     *    - In this case, algorithms are processed in order
     *  - OR, only one input should be a group, the others being size of 1
     *
     * If the property itself is a WorkspaceProperty<WorkspaceGroup> then
     * this returns false
     *
     * Returns true if processGroups() should be called.
     * It also sets up some other members.
     *
     * Override if it is needed to customize the group checking.
     *
     * @throw std::invalid_argument if the groups sizes are incompatible.
     * @throw std::invalid_argument if a member is not found
     *
     * This method (or an override) must NOT THROW any exception if there are no input workspace groups
     */
    bool Algorithm::checkGroups()
    {
      size_t numGroups = 0;
      bool processGroups = false;

      // Unroll the groups or single inputs into vectors of workspace
      m_groups.clear();
      m_groupWorkspaces.clear();
      for (size_t i=0; i<m_inputWorkspaceProps.size(); i++)
      {
        auto * prop = dynamic_cast<Property *>(m_inputWorkspaceProps[i]);
        auto * wsGroupProp = dynamic_cast<WorkspaceProperty<WorkspaceGroup> *>(prop);
        std::vector<Workspace_sptr> thisGroup;

        Workspace_sptr ws = m_inputWorkspaceProps[i]->getWorkspace();
        WorkspaceGroup_sptr wsGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);

        // Workspace groups are NOT returned by IWP->getWorkspace() most of the time
        // because WorkspaceProperty is templated by <MatrixWorkspace>
        // and WorkspaceGroup does not subclass <MatrixWorkspace>
        if (!wsGroup && !prop->value().empty())
        {
          // So try to use the name in the AnalysisDataService
          try {
          wsGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(prop->value());
          }
          catch (Exception::NotFoundError&) { /* Do nothing */ }
        }

        // Found the group either directly or by name?
        // If the property is of type WorkspaceGroup then don't unroll
        if (wsGroup && !wsGroupProp)
        {
          numGroups++;
          processGroups = true;
          std::vector<std::string> names = wsGroup->getNames();
          for (size_t j=0; j<names.size(); j++)
          {
            Workspace_sptr memberWS = AnalysisDataService::Instance().retrieve(names[j]);
            if (!memberWS)
              throw std::invalid_argument("One of the members of " + wsGroup->name() + ", " + names[j] + " was not found!.");
            thisGroup.push_back(memberWS);
          }
        }
        else
        {
          // Single Workspace. Treat it as a "group" with only one member
          if (ws)
            thisGroup.push_back(ws);
        }

        // Add to the list of groups
        m_groups.push_back(thisGroup);
        m_groupWorkspaces.push_back(wsGroup);
      }

      // No groups? Get out.
      if (numGroups == 0)
        return processGroups;

      // ---- Confirm that all the groups are the same size -----
      // Index of the single group
      m_singleGroup = -1;
      // Size of the single or of all the groups
      m_groupSize = 1;
      m_groupsHaveSimilarNames = true;
      for (size_t i=0; i<m_groups.size(); i++)
      {
        std::vector<Workspace_sptr> & thisGroup = m_groups[i];
        // We're ok with empty groups if the workspace property is optional
        if (thisGroup.empty() && !m_inputWorkspaceProps[i]->isOptional())
          throw std::invalid_argument("Empty group passed as input");
        if (!thisGroup.empty())
        {
          // Record the index of the single group.
          WorkspaceGroup_sptr wsGroup = m_groupWorkspaces[i];
          if (wsGroup && (numGroups == 1))
            m_singleGroup = int(i);

          // For actual groups (>1 members)
          if (thisGroup.size() > 1)
          {
            // Check for matching group size
            if (m_groupSize > 1)
              if (thisGroup.size() != m_groupSize)
                throw std::invalid_argument("Input WorkspaceGroups are not of the same size.");

            // Are ALL the names similar?
            if (wsGroup)
              m_groupsHaveSimilarNames = m_groupsHaveSimilarNames && wsGroup->areNamesSimilar();

            // Save the size for the next group
            m_groupSize = thisGroup.size();
          }
        }
      } // end for each group

      // If you get here, then the groups are compatible
      return processGroups;
    }



    //--------------------------------------------------------------------------------------------
    /** Process WorkspaceGroup inputs.
     *
     * This should be called after checkGroups(), which sets up required members.
     * It goes through each member of the group(s), creates and sets an algorithm
     * for each and executes them one by one.
     *
     * If there are several group input workspaces, then the member of each group
     * is executed pair-wise.
     *
     * @return true - if all the workspace members are executed.
     */
    bool Algorithm::processGroups()
    {
      std::vector<WorkspaceGroup_sptr> outGroups;

      // ---------- Create all the output workspaces ----------------------------
      for (size_t owp=0; owp<m_pureOutputWorkspaceProps.size(); owp++)
      {
        Property * prop = dynamic_cast<Property *>(m_pureOutputWorkspaceProps[owp]);
        // Do not observe ADS notifications while constructing the group
        WorkspaceGroup_sptr outWSGrp = WorkspaceGroup_sptr(new WorkspaceGroup(false));
        outGroups.push_back(outWSGrp);
        // Put the GROUP in the ADS
        AnalysisDataService::Instance().addOrReplace(prop->value(), outWSGrp );
      }

      // Go through each entry in the input group(s)
      for (size_t entry=0; entry<m_groupSize; entry++)
      {
        // use create Child Algorithm that look like this one
        Algorithm_sptr alg_sptr = this->createChildAlgorithm(this->name(),-1,-1,this->isLogging(),this->version());
        // Don't make the new algorithm a child so that it's workspaces are stored correctly
        alg_sptr->setChild(false);

        IAlgorithm* alg = alg_sptr.get();
        // Set all non-workspace properties
        this->copyNonWorkspaceProperties(alg, int(entry)+1);

        std::string outputBaseName = "";

        // ---------- Set all the input workspaces ----------------------------
        for (size_t iwp=0; iwp<m_groups.size(); iwp++)
        {
          std::vector<Workspace_sptr> & thisGroup = m_groups[iwp];
          if (!thisGroup.empty())
          {
            // By default (for a single group) point to the first/only workspace
            Workspace_sptr ws = thisGroup[0];

            if ((m_singleGroup == int (iwp)) || m_singleGroup < 0)
            {
              // Either: this is the single group
              // OR: all inputs are groups
              // ... so get then entry^th workspace in this group
              ws = thisGroup[entry];
            }
            // Append the names together
            if (!outputBaseName.empty()) outputBaseName += "_";
            outputBaseName += ws->name();

            // Set the property using the name of that workspace
            Property * prop = dynamic_cast<Property *>(m_inputWorkspaceProps[iwp]);
            alg->setPropertyValue(prop->name(), ws->name());
          } // not an empty (i.e. optional) input
        } // for each InputWorkspace property

        std::vector<std::string> outputWSNames(m_pureOutputWorkspaceProps.size());
        // ---------- Set all the output workspaces ----------------------------
        for (size_t owp=0; owp<m_pureOutputWorkspaceProps.size(); owp++)
        {
          Property * prop = dynamic_cast<Property *>(m_pureOutputWorkspaceProps[owp]);

          // Default name = "in1_in2_out"
          std::string outName = outputBaseName + "_" + prop->value();
          // Except if all inputs had similar names, then the name is "out_1"
          if (m_groupsHaveSimilarNames)
            outName = prop->value() + "_" + Strings::toString(entry+1);

          // Set in the output
          alg->setPropertyValue(prop->name(), outName);

          outputWSNames[owp] = outName;
        } // for each OutputWorkspace property

        // ------------ Execute the algo --------------
        if (!alg->execute())
          throw std::runtime_error("Execution of " + this->name() + " for group entry " + Strings::toString(entry+1) + " failed.");

        // ------------ Fill in the output workspace group ------------------
        // this has to be done after execute() because a workspace must exist 
        // when it is added to a group
        for (size_t owp=0; owp<m_pureOutputWorkspaceProps.size(); owp++)
        {
          // And add it to the output group
          outGroups[owp]->add( outputWSNames[owp] );
        }

      } // for each entry in each group

      // Finish up
      for (size_t i=0; i<outGroups.size(); i++)
      {
        // Go back to observing ADS in each group.
        outGroups[i]->observeADSNotifications(true);
        //outGroups[i]->updated();
      }

      // We finished successfully.
      setExecuted(true);
      m_notificationCenter.postNotification(new FinishedNotification(this,isExecuted()));

      return true;
    }

    //--------------------------------------------------------------------------------------------
    /** Copy all the non-workspace properties from this to alg
     *
     * @param alg :: other IAlgorithm
     * @param periodNum :: number of the "period" = the entry in the group + 1
     */
    void Algorithm::copyNonWorkspaceProperties(IAlgorithm * alg, int periodNum)
    {
      if (!alg) throw std::runtime_error("Algorithm not created!");
      std::vector<Property*> props = this->getProperties();
      for (size_t i=0; i < props.size(); i++)
      {
        Property * prop = props[i];
        if (prop)
        {
          IWorkspaceProperty* wsProp = dynamic_cast<IWorkspaceProperty*>(prop);
          // Copy the property using the string
          if (!wsProp)
            this->setOtherProperties(alg, prop->name(), prop->value(), periodNum);
        }
      }
    }


    //--------------------------------------------------------------------------------------------
    /** Virtual method to set the non workspace properties for this algorithm.
     * To be overridden by specific algorithms when needed.
     *
     *  @param alg :: pointer to the algorithm
     *  @param propertyName :: name of the property
     *  @param propertyValue :: value  of the property
     *  @param periodNum :: period number
     */
    void Algorithm::setOtherProperties(IAlgorithm * alg, const std::string & propertyName, const std::string & propertyValue, int periodNum)
    {
      (void) periodNum; //Avoid compiler warning
      if(alg)
        alg->setPropertyValue(propertyName, propertyValue);
    }


    //--------------------------------------------------------------------------------------------
    /** To query the property is a workspace property
    *  @param prop :: pointer to input properties
    *  @returns true if this is a workspace property
    */
    bool Algorithm::isWorkspaceProperty( const Kernel::Property* const prop) const
    {
      if(!prop)
      {
        return false;
      }
      const IWorkspaceProperty * const wsProp = dynamic_cast<const IWorkspaceProperty*>(prop);
      return (wsProp ? true : false);   
    }

    //=============================================================================================
    //================================== Asynchronous Execution ===================================
    //=============================================================================================
    namespace
    {
      /**
      * A object to set the flag marking asynchronous running correctly 
      */
      struct AsyncFlagHolder
      {
        /** Constructor
        * @param A :: reference to the running flag
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

    //--------------------------------------------------------------------------------------------
    /**
    * Asynchronous execution
    */
    Poco::ActiveResult<bool> Algorithm::executeAsync()
    {
      return m_executeAsync(Poco::Void());
    }

    /**Callback when an algorithm is executed asynchronously
     * @param i :: Unused argument
     * @return true if executed successfully.
    */
    bool Algorithm::executeAsyncImpl(const Poco::Void&)
    {
      AsyncFlagHolder running(m_runningAsync);
      return this->execute();
    }

    /** Handles and rescales child algorithm progress notifications.
    *  @param pNf :: The progress notification from the child algorithm.
    */
    void Algorithm::handleChildProgressNotification(const Poco::AutoPtr<ProgressNotification>& pNf)
    {
      double p = m_startChildProgress + (m_endChildProgress - m_startChildProgress)*pNf->progress;

      progress(p,pNf->message);

    }

    //--------------------------------------------------------------------------------------------
    /**
     * Cancel an algorithm
     */
    void Algorithm::cancel()
    {
      Poco::FastMutex::ScopedLock _lock(m_mutex);
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

    //--------------------------------------------------------------------------------------------
    /** This is called during long-running operations,
     * and check if the algorithm has requested that it be cancelled.
     */
    void Algorithm::interruption_point()
    {
      Poco::FastMutex::ScopedLock _lock(m_mutex);
      // only throw exceptions if the code is not multi threaded otherwise you contravene the OpenMP standard
      // that defines that all loops must complete, and no exception can leave an OpenMP section
      // openmp cancel handling is performed using the ??, ?? and ?? macros in each algrothim
      IF_NOT_PARALLEL
        if (m_cancel) throw CancelException();
    }



  } // namespace API

  //---------------------------------------------------------------------------
  // Specialized templated PropertyManager getValue definitions for Algorithm types
  //---------------------------------------------------------------------------
  namespace Kernel
  {
    /**
     * Get the value of a given property as the declared concrete type
     * @param name :: The name of the property
     * @returns A pointer to an algorithm
     */
    template<> MANTID_API_DLL
      API::IAlgorithm_sptr IPropertyManager::getValue<API::IAlgorithm_sptr>(const std::string &name) const
    {
      PropertyWithValue<API::IAlgorithm_sptr>* prop =
        dynamic_cast<PropertyWithValue<API::IAlgorithm_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type";
        throw std::runtime_error(message);
      }
    }
  
    /**
     * Get the value of a given property as the declared concrete type (const version)
     * @param name :: The name of the property
     * @returns A pointer to an algorithm
     */
    template<> MANTID_API_DLL
      API::IAlgorithm_const_sptr IPropertyManager::getValue<API::IAlgorithm_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<API::IAlgorithm_sptr>* prop =
        dynamic_cast<PropertyWithValue<API::IAlgorithm_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type";
        throw std::runtime_error(message);
      }
    }
  }

} // namespace Mantid
