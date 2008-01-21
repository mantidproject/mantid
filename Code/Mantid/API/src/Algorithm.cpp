//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/IStorable.h"

namespace Mantid
{
  namespace API
  {

    // Get a reference to the logger
    Kernel::Logger& Algorithm::g_log = Kernel::Logger::get("Algorithm");

    /// Constructor
    Algorithm::Algorithm() :
    PropertyManager(),
      m_name("unknown"), 
      m_version("unknown"), 
      m_isInitialized(false), 
      m_isExecuted(false), 
      m_isChildAlgorithm(false)
    {
    }

    /// Virtual destructor
    Algorithm::~Algorithm()
    {
    }

    /** The identifying name of the algorithm object. 
    * 
    *  @return Name of Instance
    */
    const std::string& Algorithm::name() const
    {
      return m_name;
    }

    /** The version of the algorithm
    * 
    *  @return Version string
    */
    const std::string& Algorithm::version() const
    {
      return m_version;
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
    void Algorithm::execute()
    {
      // Return a failure if the algorithm hasn't been initialized
      if ( !isInitialized() )
      {
        g_log.error("Algorithm is not initialized:" + m_name);
        throw std::runtime_error("Algorithm is not initialised:" + m_name);
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
          // Call the concrete algorithm's exec method
          this->exec();
        }
        catch(std::runtime_error& ex)
        {
          g_log.error()<< "Error in Execution of algorithm "<< m_name<<std::endl;
          if (m_isChildAlgorithm) throw;
        }
        catch(std::logic_error& ex)
        {
          g_log.error()<< "Logic Error in Execution of algorithm "<< m_name<<std::endl;
          if (m_isChildAlgorithm) throw;
        }


        // Put any output workspaces into the AnalysisDataService - if this is not a child algorithm
        if (!isChild())
        { this->store();}

        setExecuted(true);	
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
      return;
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
      AlgorithmManager* algManager = AlgorithmManager::Instance(); 
      Algorithm_sptr alg = algManager->createUnmanaged(name);
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

    /** Stores any output workspaces into the AnalysisDataService
    *  @throw std::runtime_error If unable to successfully store an output workspace
    */
    void Algorithm::store()
    {
      const std::vector< Property*> &props = getProperties();
      for (unsigned int i = 0; i < props.size(); ++i)
      {
        Kernel::IStorable *wsProp = dynamic_cast<Kernel::IStorable*>(props[i]);
        if (wsProp)
        {
          try
          {
            // Store the workspace. Will only be 1 output workspace so stop looping if true
            if ( wsProp->store() ) break;
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
