/*
    Base class from which all concrete algorithm classes should be derived. 
    In order for a concrete algorithm class to do anything
    useful the methods init(), exec() and final() should be overridden.   

    @author Russell Taylor, Tessella Support Services plc
    @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
    @date 12/09/2007
    
    Copyright � 2007 ???RAL???

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

// Every Algorithm will have to register itself into the factory
// Later, this base class will be abstract so the next line will go, but it's there for testing right now
// The argument has to be the name of the class, it will also be the name in the factory
// TODO
//DECLARE_ALGORITHM(Algorithm)

namespace Mantid
{
  Algorithm::Algorithm()
  :
  m_outputWorkspace(0),
  m_name("unknown"),
  m_version("unknown"),
  m_isInitialized(false),
  m_isExecuted(false),
  m_isFinalized(false)
  {
    m_subAlgms = new std::vector<Algorithm *>();
  }


  // Constructor
  Algorithm::Algorithm( const std::string& name, //ISvcLocator *pSvcLocator,
                        const std::string& version)
    : 
    m_outputWorkspace(0),
    m_name(name),
    m_version(version),
    m_isInitialized(false),
    m_isExecuted(false),
    m_isFinalized(false)
  { 
    m_subAlgms = new std::vector<Algorithm *>();
  }
  
  Algorithm::~Algorithm()
  {
    delete m_subAlgms;
  }
  
  const std::string& Algorithm::name() const 
  {
    return m_name;
  }
  
  const std::string& Algorithm::version() const 
  {
    return m_version;
  }
  
  StatusCode Algorithm::initialize() 
  {
    MsgStream log ( msgSvc() , name() + ".initialize()" );
    
    // Bypass the initialization if the algorithm
    // has already been initialized.
    if ( m_isInitialized ) return StatusCode::SUCCESS;
    
    // Set the input and output workspaces
    std::string inputWorkspaceName;
    StatusCode status = getProperty("InputWorkspace", inputWorkspaceName);
    // If property not set print warning message and set pointer to null
    if ( status.isFailure() )
    {
      log << MSG::INFO << "Input workspace property not set" << endreq;
      m_inputWorkspace = 0;
    }
    else 
    {
      AnalysisDataService *data = AnalysisDataService::Instance();
      StatusCode status = data->retrieve(inputWorkspaceName, m_inputWorkspace);
      if ( status.isFailure() )
      {
        log << MSG::ERROR << "Input workspace doesn't exist" << endreq;
        return status;
      }
    }
    status = getProperty("OutputWorkspace", m_outputWorkspaceName);
    // If property not set print warning message and set pointer to null
    if ( status.isFailure() )
    {
      log << MSG::INFO << "Output workspace property not set" << endreq;
      m_outputWorkspaceName = "";
    }
    
    // Invoke initialize() method of the derived class inside a try/catch clause
    try 
    {
      // Invoke the initialize() method of the derived class
      StatusCode status = init();
      if( status.isFailure() ) return StatusCode::FAILURE;
  
      // Now initialize any sub-algorithms
      std::vector<Algorithm *>::iterator it;
      for (it = m_subAlgms->begin(); it != m_subAlgms->end(); it++) {
        status = (*it)->initialize();
        if( status.isFailure() ) {
          log << MSG::ERROR << " Error initializing one or several sub-algorithms"
              << endreq;
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
      MsgStream log ( msgSvc() , name() + ".initialize()" );
      log << MSG::FATAL << "UNKNOWN Exception is caught " << endreq;
      // Gaudi: 
    }
    
    // Only gets to here if an exception is encountered
    return StatusCode::FAILURE;
  }
  
  StatusCode Algorithm::execute() 
  {
    MsgStream log(0,"");
    
    // Return a failure if the algorithm hasn't been initialized
    if ( !isInitialized() ) return StatusCode::FAILURE;
    
    // Invoke exec() method of derived class and catch all uncaught exceptions
    try
    {
      // Call the concrete algorithm's exec method
      StatusCode status = exec();

      // Register the output workspace with the analysis data service
      AnalysisDataService *data = AnalysisDataService::Instance();
      if ( m_outputWorkspace )
      {
        StatusCode stat = data->add(m_outputWorkspaceName, m_outputWorkspace);
        if ( stat.isFailure() )
        {
          log << MSG::ERROR << "Unable to register output workspace" << endreq;
        }
      }
      else 
      {
        log << MSG::WARNING << "Output workspace has not been created" << endreq;
      }
      setExecuted(true);
      
      // NOTE THAT THERE IS NO EXECUTION OF SUB-ALGORITHMS HERE.
      // DON'T AT PRESENT KNOW WHERE THAT TAKES PLACE.
      
      if ( status.isFailure() )
      {
        // Gaudi calls the exception service error handler here, I will just return with failure.
      }
  
      return status;
    }
    // Gaudi also specifically catches GaudiException & std:exception.
    catch (...)
    {
      // Gaudi sets the executed flag to true here despite the exception. Not sure why.
      setExecuted(true);
  
      MsgStream log ( msgSvc() , name() + ".execute()" );
      log << MSG::FATAL << "UNKNOWN Exception is caught " << endreq;
  
      // Gaudi calls exception service 'handle' method here
    }
  
    // Gaudi has some stuff here where it tests for failure, increments the error counter
    // and then converts to success if less than the maximum. This is clearly related to
    // having an event loop, and thus we shouldn't want it. This is the only place it's used.
    
    // Only gets to here if an exception is encountered
    return StatusCode::FAILURE;
  }
  
  StatusCode Algorithm::finalize() 
  {
    // Bypass the finalization if the algorithm hasn't been initialized or
    // has already been finalized.
    if ( !isInitialized() || isFinalized() ) return StatusCode::FAILURE;
  
    // Invoke final() method of the derived class inside a try/catch clause
    try
    {
      StatusCode status = StatusCode::SUCCESS;
      
      // Finalize first any sub-algoithms (it can be done more than once)
      // Gaudi at some point had a bug if this wasn't done first.
      std::vector<Algorithm *>::iterator it;
      for (it = m_subAlgms->begin(); it != m_subAlgms->end(); it++) {
        status = (*it)->finalize();
        // The next test isn't in Gaudi, which ignores the outcome of finalizing sub-algorithms
        if( status.isFailure() ) {
          MsgStream log ( msgSvc() , name() + ".finalize()" );
          log << MSG::ERROR << " Error finalizing one or several sub-algorithms"
              << endreq;
          return status;
        }
      }
  
      // Invoke the final() method of the derived class
      status = final();
      if( status.isFailure() ) return status;
  
      // Release all sub-algorithms (uses IInterface release method in Gaudi instead of direct delete)
      for (it = m_subAlgms->begin(); it != m_subAlgms->end(); it++) {
        delete *it;
      }
      
      // Indicate that this Algorithm has been finalized to prevent duplicate attempts
      setFinalized( );
      return status;
      
    }
    // Unpleasant catch-all! Along with this, Gaudi version catches GaudiException & std::exception
    // but doesn't really do anything except (print fatal) messages.
    catch (...)
    {
      // (1) perform the printout
      MsgStream log ( msgSvc() , name() + ".finalize()" );
      log << MSG::FATAL << "UNKNOWN Exception is caught " << endreq;    
    }
    
    // Only gets to here if an exception is encountered
    return StatusCode::FAILURE;
  }
  
  bool Algorithm::isInitialized( ) const 
  {
    return m_isInitialized;
  }
  
  bool Algorithm::isExecuted() const 
  {
    return m_isExecuted;
  }
  
  bool Algorithm::isFinalized( ) const
  {
    return m_isFinalized;
  }
  
  std::vector<Algorithm*>* Algorithm::subAlgorithms( ) const 
  {
    return m_subAlgms;
  }  
  
  // IProperty implementation
  // Empty for now - requires Property manager
//  StatusCode Algorithm::setProperty(const Property& p) {
//          return m_propertyMgr->setProperty(p);
//  }
  StatusCode Algorithm::setProperty(const std::string& s) {
    m_properties[s] = "";
    return StatusCode::SUCCESS;
  }
  StatusCode Algorithm::setProperty(const std::string& n, const std::string& v) {
    m_properties[n] = v;
    return StatusCode::SUCCESS;
  }
//  StatusCode Algorithm::getProperty(Property* p) const {
//          return m_propertyMgr->getProperty(p);
//  }
//  const Property& Algorithm::getProperty( const std::string& name) const{
//          return m_propertyMgr->getProperty(name);
//  }
  StatusCode Algorithm::getProperty(const std::string& n, std::string& v ) const {
    // Check if key is in map & if not return with failure
    if (m_properties.find(n) == m_properties.end()) return StatusCode::FAILURE;

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

}
