/*
    Base class from which all concrete algorithm classes should be derived. 
    In order for a concrete algorithm class to do anything
    useful the methods init(), exec() and final() should be overridden.   

    @author Russell Taylor, Tessella Support Services plc
    @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
    @date 12/09/2007
    
    Copyright &copy; 2007 ???RAL???

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
*/

#include "../inc/Algorithm.h"
#include "../inc/AnalysisDataService.h"
#include <iostream>
#include <iomanip>

namespace Mantid
{
namespace Kernel
{
  Logger& Algorithm::g_log = Logger::get("Algorithm");

  Algorithm::Algorithm() :
    m_inputWorkspace(0),
    m_outputWorkspace(0),
    m_name("unknown"),
    m_version("unknown"),
    m_isInitialized(false),
    m_isExecuted(false),
    m_isFinalized(false)
  {
     std::cout<<"Algorithm logger == "<<
	std::setbase(16)<<reinterpret_cast<long>(&Algorithm::g_log)
		<<std::endl;
     std::cout<<"Algorithm == "<<
	 std::setbase(16)<<reinterpret_cast<long>(this)
		<<std::endl;
	  
  }
  
  Algorithm::~Algorithm()
  {
      std::cout<<"Deleting Algorithm "<<std::hex
	<<reinterpret_cast<long>(this)<<std::endl;
  }
  
  const std::string& 
  Algorithm::name() const 
      /*! 
        The identifying name of the algorithm object. This is the name of a 
	particular instantiation of an algorithm object as opposed to the name 
	of the algorithm itself, e.g. "LinearTrackFit" may be the name of a 
	concrete algorithm class, whereas "ApproxTrackFit" and 
        "BestTrackFit" may be two instantiations of the class configured 
         to find tracks with different fit criteria.
         \return Name of Instance  
      */
  {
    return m_name;
  }
  
  const std::string& 
  Algorithm::version() const 
     /*!
         \return Version string
     */ 
  {
    return m_version;
  }
  
  StatusCode 
  createSubAlgorithm( const std::string& type, const std::string& name, 
                                    Algorithm*& pSubAlg )
   /*! Create a sub algorithm.  A call to this method creates a child algorithm object.
        Note that the returned pointer is to Algorithm 
	 (as opposed to IAlgorithm), and thus the methods of IProperty 
	 are also available for the direct setting of the sub-algorithm's
	 properties. Using this mechanism instead of creating daughter 
	algorithms directly via the new operator is prefered since then 
	the framework can take care of all of the necessary book-keeping.
	    
	 \param type :: The concrete algorithm class of the sub algorithm
	 \param name :: The name to be given to the sub algorithm
	 \param pSubAlg :: Set to point to the newly created algorithm object
         \retval  Success since nothing every happens.
    */
  {
      return StatusCode::SUCCESS;	  
  }
  
  StatusCode 
  Algorithm::initialize() 
   /** Initialization method invoked by the framework. This method is responsible
	   *  for any bookkeeping of initialization required by the framework itself.
	   *  It will in turn invoke the initialize() method of the derived algorithm,
	   * and of any sub-algorithms which it creates. 
	   */
  {
    // Bypass the initialization if the algorithm
    // has already been initialized.
    if ( m_isInitialized ) 
	return StatusCode::SUCCESS;
        
    // Invoke initialize() method of the derived class inside a try/catch clause
    try 
    {
      // Invoke the initialize() method of the derived class
      StatusCode status = init();
      if( status.isFailure() ) 
	 return StatusCode::FAILURE;
  
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
      return StatusCode::SUCCESS;
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
    return StatusCode::FAILURE;
  }
  
  StatusCode Algorithm::execute() 
   /** The actions to be performed by the algorithm on a dataset. This method is
	   *  invoked for top level algorithms by the application manager.
	   *  This method invokes exec() method. 
	   *  For sub-algorithms either the execute() method or exec() method 
	   *  must be EXPLICITLY invoked by  the parent algorithm.
	   */
  {
    // Return a failure if the algorithm hasn't been initialized
    if ( !isInitialized() ) 
	return StatusCode::FAILURE;

    // Set the input and output workspaces
    std::string inputWorkspaceName;
    StatusCode status = getProperty("InputWorkspace", inputWorkspaceName);
     m_outputWorkspace = 0;
     m_inputWorkspace = 0;
     m_outputWorkspaceName = "";
     AnalysisDataService* ADS = AnalysisDataService::Instance();
    if ( status.isFailure() )
       g_log.information("Input workspace property not set");
     else
        {
 
            status = ADS->retrieve(inputWorkspaceName, m_inputWorkspace);
             if (status.isFailure() )
               {
                   g_log.error("Input workspace doesn't exist");
		    return status;
                }
	}
   // Output Workspace:
    status = getProperty("OutputWorkspace", m_outputWorkspaceName);
    if ( status.isFailure() )
       g_log.information("Output workspace property not set");
  
    
    // Invoke exec() method of derived class and catch all uncaught exceptions
    try
      {
         // Call the concrete algorithm's exec method
         StatusCode status = exec();

      // Register the output workspace with the analysis data service
      if ( m_outputWorkspace )
        {
          status = ADS->add(m_outputWorkspaceName, m_outputWorkspace);
           if ( status.isFailure() ) 
              g_log.error("Unable to register output workspace");
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
    return StatusCode::FAILURE;
  }
  
  StatusCode 
  Algorithm::finalize() 
      /*! 
         System finalization. This method invokes the finalize() method of a 
	 concrete algorithm and the finalize() methods of all of that algorithm's 
	 sub algorithms. 
	*/ 
  {
    // Bypass the finalization if the algorithm hasn't been initialized or
    // has already been finalized.
    if ( !isInitialized() || isFinalized() ) 
	return StatusCode::FAILURE;
  
    // Invoke final() method of the derived class inside a try/catch clause
    try
    {
       StatusCode status(StatusCode::SUCCESS,true);

      // Finalize first any sub-algoithms (it can be done more than once)
      // Gaudi at some point had a bug if this wasn't done first.
    
	std::vector<Algorithm *>::iterator it;
        for (it = m_subAlgms.begin(); it != m_subAlgms.end(); it++) 
	{  
             status = (*it)->finalize();
        // The next test isn't in Gaudi, which ignores the outcome of finalizing sub-algorithms
	// THIS IS BECAUSE subAlgms MUST BE DELETED!!!!
             if( status.isFailure() ) 
		{
                  g_log.error(" Error finalizing one or several sub-algorithms:"+(*it)->name());
		  break;
                }
	}		
  
      // Invoke the final() method of the derived class
      StatusCode Fstatus= final();
      if( Fstatus.isFailure() ) 
	  status=Fstatus;
  
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
	 delete (*it);
	m_subAlgms.clear();
    }
    
    // Only gets to here if an exception is encountered
    return StatusCode::FAILURE;
  }
  
  bool 
  Algorithm::isInitialized( ) const 
  /// Has the Algorithm already been initialized?s
  {
    return m_isInitialized;
  }
  
  bool 
  Algorithm::isExecuted() const 
   /// Has the Algorithm already been executed
  {
    return m_isExecuted;
  }
  
  bool 
  Algorithm::isFinalized( ) const
  /// Has the Algorithm already been finalized?
  {
    return m_isFinalized;
  }
  
  // IProperty implementation
  // Empty for now - requires Property manager
  //  StatusCode Algorithm::setProperty(const Property& p) {
  //          return m_propertyMgr->setProperty(p);
  //  }
  StatusCode Algorithm::setProperty(const std::string& s) 
    {
      if (s.empty())
         return StatusCode::FAILURE;
    m_properties[s] = "";
    return StatusCode::SUCCESS;
  }
  StatusCode Algorithm::setProperty(const std::string& n, const std::string& v) 
  { 
      if (n.empty())
         return StatusCode::FAILURE;
    m_properties[n] = v;
    return StatusCode::SUCCESS;
  }
//  StatusCode Algorithm::getProperty(Property* p) const {
//          return m_propertyMgr->getProperty(p);
//  }
//  const Property& Algorithm::getProperty( const std::string& name) const{
//          return m_propertyMgr->getProperty(name);
//  }
  StatusCode 
  Algorithm::getProperty(const std::string& n, std::string& v ) const 
  {
    // Check if key is in map & if not return with failure
    if (m_properties.find(n) == m_properties.end()) 
	return StatusCode::FAILURE;

    // Retrieve the value corresponding to the key
    v = m_properties.find(n)->second;
    return StatusCode::SUCCESS;    
  }
//  const std::vector<Property*>& Algorithm::getProperties( ) const {
//          return m_propertyMgr->getProperties();
//  }

  
  /**
   ** Protected Member Functions
   **/
  
  void Algorithm::setInitialized( ) 
  {
    m_isInitialized = true;
  }
  
  // Note slight difference to above & below methods, not sure if there's any significance.
  void Algorithm::setExecuted( bool state ) 
  {
    m_isExecuted = state;
  }
  
  void Algorithm::setFinalized( )
  {
    m_isFinalized = true;
  }

} // namespace Kernel
} // namespace Mantid
