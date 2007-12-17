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
      m_isChildAlgorithm(false),
      m_name("unknown"),
      m_version("unknown"),
      m_subAlgms(),
      m_isInitialized(false),
      m_isExecuted(false),
      m_isFinalized(false) //,
      //  m_propertyMgr()
    {
    }

    /// Virtual destructor
    Algorithm::~Algorithm()
    {
    }

    /** The identifying name of the algorithm object. This is the name of a 
    *  particular instantiation of an algorithm object as opposed to the name 
    *  of the algorithm itself, e.g. "LinearTrackFit" may be the name of a 
    *  concrete algorithm class, whereas "ApproxTrackFit" and 
    *  "BestTrackFit" may be two instantiations of the class configured 
    *  to find tracks with different fit criteria.
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
      if (m_isInitialized) return;

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
        try
        {
          // Now initialize any sub-algorithms
          std::vector<Algorithm*>::iterator it;
          for (it = m_subAlgms.begin(); it != m_subAlgms.end(); it++)
          {
            (*it)->initialize();
          }
        }
        catch(std::runtime_error& ex)
        {
          g_log.error()<<"Error initializing one or several sub-algorithms"<<ex.what();
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

        // Put any output workspaces into the AnalysisDataService - if this is not a child algorithm
        if (!isChild()) {this->store();}

        setExecuted(true);

        // NOTE THAT THERE IS NO EXECUTION OF SUB-ALGORITHMS HERE.
        // THIS HAS TO BE EXPLICITLY DONE BY THE PARENT ALGORITHM'S exec() METHOD.				
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

    /** System finalization. This method invokes the finalize() method of a 
    * concrete algorithm and the finalize() methods of all of that algorithm's 
    * sub algorithms. 
    *
    *  @throw runtime_error Thrown if algorithm or sub-algorithm cannot be finalised
    */
    void Algorithm::finalize()
    {
      // Bypass the finalization if the algorithm hasn't been initialized or
      // has already been finalized.
      if ( !isInitialized() || isFinalized() )
      {
        g_log.error("algorithm hasn't been initialized or has already been finalized:");
        throw std::runtime_error("algorithm hasn't been initialized or has already been finalized");
      }

      // Invoke final() method of the derived class inside a try/catch clause
      try
      {
        // Finalize first any sub-algorithms (it can be done more than once)
        // Gaudi at some point had a bug if this wasn't done first.
        try
        {
          std::vector<Algorithm *>::iterator it;
          for (it = m_subAlgms.begin(); it != m_subAlgms.end(); it++)
          {
            (*it)->finalize();
          }
        }
        catch(std::runtime_error& ex)
        {
          g_log.error()<<"Error finalizing one or several sub-algorithms: "<<ex.what();
          throw;
        }

        try
        {
          // Invoke the final() method of the derived class
          this->final();
        }
        catch(std::runtime_error& ex)
        {
          g_log.error()<<"Error finalizing main algorithm: "<<m_name<<ex.what();
          throw;
        }

        // Release all sub-algorithms (uses IInterface release method in Gaudi instead of direct delete)
        // if it managed to finalise them
        for (std::vector<Algorithm *>::iterator it = m_subAlgms.begin(); it != m_subAlgms.end(); it++)
        {
          delete (*it);
        }
        m_subAlgms.clear();

        // Indicate that this Algorithm has been finalized to prevent duplicate attempts
        setFinalized( );

      }
      // Unpleasant catch-all! Along with this, Gaudi version catches GaudiException & std::exception
      // but doesn't really do anything except (print fatal) messages.

      catch (...)
      {
        // (1) perform the printout
        g_log.fatal("UNKNOWN Exception is caught ");
        /*
        std::vector<Algorithm *>::iterator it;	    
        for (it = m_subAlgms.begin(); it != m_subAlgms.end(); it++) 
        {
        delete (*it);
        }
        m_subAlgms.clear();
        */
        throw;
      }
      // Only gets to here if algorithm finished normally
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

    /// Has the Algorithm already been finalized?
    bool Algorithm::isFinalized() const
    {
      return m_isFinalized;
    }

    /** Create a sub algorithm.  A call to this method creates a child algorithm object.
    *  Note that the returned pointer is to Algorithm (as opposed to IAlgorithm), and 
    *  thus the methods of IProperty are also available for the direct setting of the 
    *  sub-algorithm's properties. Using this mechanism instead of creating daughter 
    *  algorithms directly via the new operator is prefered since then 
    *  the framework can take care of all of the necessary book-keeping.
    *
    *  @param name    The concrete algorithm class of the sub algorithm
    *  @returns Set to point to the newly created algorithm object
    * 		
    */
    Algorithm* Algorithm::createSubAlgorithm(const std::string& name)
    {
      /// @todo This method needs implementing now that we have an algorithm factory
      return NULL;
    }

    // IProperty implementation
    // Delegate to the property manager
    void Algorithm::setProperty(const std::string &name, const std::string &value)
    {
      PropertyManager::setProperty(name, value);
    }
    //
    //bool Algorithm::existsProperty(const std::string& name) const
    //{
    //  return m_propertyMgr.existsProperty(name);
    //}
    //
    std::string Algorithm::getPropertyValue(const std::string &name) const
    {
      return PropertyManager::getPropertyValue(name);
    }
    //
    //Kernel::Property* Algorithm::getProperty(const std::string &name) const
    //{
    //  return m_propertyMgr.getProperty(name);
    //}
    //
    //const std::vector< Kernel::Property*>& Algorithm::getProperties() const
    //{
    //  return m_propertyMgr.getProperties();
    //}

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

    /// Set the Algorithm finalized state
    void Algorithm::setFinalized()
    {
      m_isFinalized = true;
    }

    /** Register a property with the property manager.
    *  Delegated to the PropertyManager method.
    *  @param p A pointer to the property instance to register
    */
    //void Algorithm::declareProperty(Kernel::Property *p)
    //{
    //  m_propertyMgr.declareProperty(p);
    //}

    /** Specialised version of declareProperty template method. Deals with case when the value argument
    *  is passed as a quote-enclosed string.
    *  Delegated to the PropertyManager method.
    *  @param name The name of the property
    *  @param value The initial value to assign to the property
    *  @param validator Pointer to the (optional) validator.
    *  @param doc The (optional) documentation string
    */
    //void Algorithm::declareProperty(const std::string &name, const char* value,
    //    Kernel::IValidator<std::string> *validator, const std::string &doc)
    //{
    //  m_propertyMgr.declareProperty(name, value, validator, doc);
    //}

    //----------------------------------------------------------------------
    // Private Member Functions
    //----------------------------------------------------------------------

    /** Check all properties for validity.
    *  Delegated to the PropertyManager method
    *  @return True if all the declared properties have valid values
    */
    //bool Algorithm::validateProperties() const
    //{
    //  return m_propertyMgr.validateProperties();
    //}

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

    /**  To query whether algorithm is a child.
    *  @returns true - the algorithm is a child algorithm.  False - this is a full manamged algorithm.
    */
    bool Algorithm::isChild() const
    {
      return m_isChildAlgorithm;
    }

    /**  To query whether algorithm is a child.
    *  @param isChild True - the algorithm is a child algorithm.  False - this is a full manamged algorithm.
    */
    void Algorithm::setChild(const bool isChild)
    {
      m_isChildAlgorithm = isChild;
    }

  } // namespace API
} // namespace Mantid
