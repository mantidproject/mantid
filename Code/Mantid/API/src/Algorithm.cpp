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
    m_inputWorkspace(0),
    m_outputWorkspace(0),
    m_name("unknown"),
    m_version("unknown"),
    m_isInitialized(false),
    m_isExecuted(false),
    m_isFinalized(false),
    m_propertyMgr()
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
   * 
   *  @return A StatusCode object indicating whether the operation was successful
   */
  Kernel::StatusCode Algorithm::initialize() 
  {
    // Bypass the initialization if the algorithm
    // has already been initialized.
    if ( m_isInitialized ) return Kernel::StatusCode::SUCCESS;
        
    // Declare the Input/OutputWorkspace properties - common to all algorithms
    declareProperty("InputWorkspace","");
    declareProperty("OutputWorkspace","");
    
    // Invoke initialize() method of the derived class inside a try/catch clause
    try 
    {
      // Invoke the initialize() method of the derived class
      Kernel::StatusCode status = init();
      if( status.isFailure() ) return Kernel::StatusCode::FAILURE;
  
      // Now initialize any sub-algorithms
      std::vector<Algorithm*>::iterator it;
      for (it = m_subAlgms.begin(); it != m_subAlgms.end(); it++) 
      {
        status = (*it)->initialize();
        if( status.isFailure() ) 
        {
          g_log.error("Error initializing one or several sub-algorithms");
          return status;        
        }
      }
      
      // Indicate that this Algorithm has been initialized to prevent duplicate attempts.
      setInitialized();
      return Kernel::StatusCode::SUCCESS;
    }
    // Unpleasant catch-all! Along with this, Gaudi version catches GaudiException & std::exception
    // but doesn't really do anything except (print fatal) messages.
    catch (...)
    {
      // Gaudi: A call to the auditor service is here
      // (1) perform the printout
      g_log.fatal("UNKNOWN Exception is caught ");
      // Gaudi: 
    }
    
    // Only gets to here if an exception is encountered
    return Kernel::StatusCode::FAILURE;
  }
  
  /** The actions to be performed by the algorithm on a dataset. This method is
    *  invoked for top level algorithms by the application manager.
    *  This method invokes exec() method. 
    *  For sub-algorithms either the execute() method or exec() method 
    *  must be EXPLICITLY invoked by  the parent algorithm.
    * 
    *  @return A StatusCode object indicating whether the operation was successful
    */
  Kernel::StatusCode Algorithm::execute() 
  {
    // Return a failure if the algorithm hasn't been initialized
    if ( !isInitialized() ) return Kernel::StatusCode::FAILURE;

    // Set the input and output workspaces
    std::string inputWorkspaceName;
   
    m_outputWorkspace = 0;
    m_inputWorkspace = 0;
    m_outputWorkspaceName = "";
    AnalysisDataService* ADS = AnalysisDataService::Instance();
    
    try {
      inputWorkspaceName = getPropertyValue("InputWorkspace");
      Kernel::StatusCode status = ADS->retrieve(inputWorkspaceName, m_inputWorkspace);
      if (status.isFailure() )
      {
        g_log.information("Input workspace doesn't exist");
      }
    } catch (Kernel::Exception::NotFoundError e) {
      g_log.information("Input workspace property not set ");     
    }
        
    // Invoke exec() method of derived class and catch all uncaught exceptions
    try
    {
      // Call the concrete algorithm's exec method
      Kernel::StatusCode status = exec();
      
      // Register the output workspace with the analysis data service
      if ( m_outputWorkspace )
      {
        // Output Workspace:
        try {
          m_outputWorkspaceName = getPropertyValue("OutputWorkspace");
          status = ADS->addOrReplace(m_outputWorkspaceName, m_outputWorkspace);
          if ( status.isFailure() ) g_log.error("Algorithm: Unable to register output workspace");
        } catch (Kernel::Exception::NotFoundError e) {
          g_log.information("Output workspace property not set");
        }
      }
        
      setExecuted(true);
      
      // NOTE THAT THERE IS NO EXECUTION OF SUB-ALGORITHMS HERE.
      // THIS HAS TO BE EXPLICITLY DONE BY THE PARENT ALGORITHM'S exec() METHOD.
      
      return status;
    }
    // Gaudi also specifically catches GaudiException & std:exception.
    catch (...)
    {
      // Gaudi sets the executed flag to true here despite the exception. 
       // This allows it to move to the next command or it just loops indefinately.
      setExecuted(true);
  
      g_log.error("UNKNOWN Exception is caught ");
  
      // Gaudi calls exception service 'handle' method here
    }
  
    // Gaudi has some stuff here where it tests for failure, increments the error counter
    // and then converts to success if less than the maximum. This is clearly related to
    // having an event loop, and thus we shouldn't want it. This is the only place it's used.
    
    // Only gets to here if an exception is encountered
    return Kernel::StatusCode::FAILURE;
  }
  
  /** System finalization. This method invokes the finalize() method of a 
   * concrete algorithm and the finalize() methods of all of that algorithm's 
   * sub algorithms. 
   *
   *  @return A StatusCode object indicating whether the operation was successful
   */ 
  Kernel::StatusCode Algorithm::finalize() 
  {
    // Bypass the finalization if the algorithm hasn't been initialized or
    // has already been finalized.
    if ( !isInitialized() || isFinalized() ) return Kernel::StatusCode::FAILURE;
  
    // Invoke final() method of the derived class inside a try/catch clause
    try
    {
      Kernel::StatusCode status(Kernel::StatusCode::SUCCESS,true);

      // Finalize first any sub-algoithms (it can be done more than once)
      // Gaudi at some point had a bug if this wasn't done first.
    
      std::vector<Algorithm *>::iterator it;
      for (it = m_subAlgms.begin(); it != m_subAlgms.end(); it++) 
      {  
        status = (*it)->finalize();
        if( status.isFailure() ) 
        {
          g_log.error(" Error finalizing one or several sub-algorithms:"+(*it)->name());
          break;
        }
      }		
  
      // Invoke the final() method of the derived class
      Kernel::StatusCode Fstatus= final();
      if( Fstatus.isFailure() ) status=Fstatus;
  
      // Release all sub-algorithms (uses IInterface release method in Gaudi instead of direct delete)
      for (it = m_subAlgms.begin(); it != m_subAlgms.end(); it++) 
      {
        delete (*it);
      }
      m_subAlgms.clear();
      
      // Indicate that this Algorithm has been finalized to prevent duplicate attempts
      setFinalized( );
      return status;
      
    }
    // Unpleasant catch-all! Along with this, Gaudi version catches GaudiException & std::exception
    // but doesn't really do anything except (print fatal) messages.
    catch (...)
    {
      // (1) perform the printout
      g_log.error("UNKNOWN Exception is caught ");   

      std::vector<Algorithm *>::iterator it;	    
      for (it = m_subAlgms.begin(); it != m_subAlgms.end(); it++) 
      {
        delete (*it);
      }
      m_subAlgms.clear();
    }
    
    // Only gets to here if an exception is encountered
    return Kernel::StatusCode::FAILURE;
  }
  
  /// Has the Algorithm already been initialized
  bool Algorithm::isInitialized( ) const 
  {
    return m_isInitialized;
  }
  
  /// Has the Algorithm already been executed
  bool Algorithm::isExecuted() const 
  {
    return m_isExecuted;
  }
  
  /// Has the Algorithm already been finalized?
  bool Algorithm::isFinalized( ) const
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
   *  @param type    The concrete algorithm class of the sub algorithm
   *  @param name    The name to be given to the sub algorithm
   *  @param pSubAlg Set to point to the newly created algorithm object
   * 
   *  @return        Success since nothing ever happens.
   */
  Kernel::StatusCode Algorithm::createSubAlgorithm( const std::string& type, const std::string& name, 
                                 Algorithm*& pSubAlg )
  {
    /// @todo This method needs implementing now that we have an algorithm factory
    return Kernel::StatusCode::SUCCESS;   
  }
  
  // IProperty implementation
  // Delegate to the property manager
  void Algorithm::setProperty( const std::string &name, const std::string &value )
  {
    m_propertyMgr.setProperty(name, value);
  }
  
  bool Algorithm::existsProperty( const std::string& name ) const
  {
    return m_propertyMgr.existsProperty(name);
  }
  
  std::string Algorithm::getPropertyValue( const std::string &name ) const
  {
    return m_propertyMgr.getPropertyValue(name);
  }
  
  Kernel::Property* Algorithm::getProperty( const std::string &name ) const
  {
    return m_propertyMgr.getProperty(name);
  }
  
  std::vector< Kernel::Property* > Algorithm::getProperties() const
  {
    return m_propertyMgr.getProperties();
  }
  
  //----------------------------------------------------------------------
  // Protected Member Functions
  //----------------------------------------------------------------------
  
  /// Set the Algorithm initialized state
  void Algorithm::setInitialized( ) 
  {
    m_isInitialized = true;
  }
  
  /// Set the executed flag to the specified state
  // Public in Gaudi - don't know why and will leave here unless we find a reason otherwise
  //     Also don't know reason for different return type and argument.
  void Algorithm::setExecuted( bool state ) 
  {
    m_isExecuted = state;
  }
  
  /// Set the Algorithm finalized state
  void Algorithm::setFinalized( )
  {
    m_isFinalized = true;
  }

  /** Register a property with the property manager.
   *  Delegated to the PropertyManager method
   *  @param p A pointer to the property instance to register
   */
  void Algorithm::declareProperty( Kernel::Property *p )
  {
    m_propertyMgr.declareProperty(p);
  }
  
} // namespace API
} // namespace Mantid
