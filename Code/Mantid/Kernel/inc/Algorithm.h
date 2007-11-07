#ifndef MANTID_KERNEL_ALGORITHM_H_
#define MANTID_KERNEL_ALGORITHM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "System.h"
#include "IAlgorithm.h"
#include "AlgorithmManager.h"
#include "WorkspaceFactory.h"
#include "Logger.h"

#include <vector>
//#include <ext/hash_map>
#include <map>

#ifndef PACKAGE_VERSION
 #define PACKAGE_VERSION "unknown"
#endif

namespace Mantid
{
namespace Kernel
{

// Forward declaration
class Workspace;

/** @class Algorithm Algorithm.h Kernel/Algorithm.h

    Base class from which all concrete algorithm classes should be derived. 
    In order for a concrete algorithm class to do anything
    useful the methods init(), exec() and final() should be overridden.
    
    Further text from Gaudi file.......
    The base class provides utility methods for accessing 
    standard services (event data service etc.); for declaring
    properties which may be configured by the job options 
    service; and for creating sub algorithms.
    The only base class functionality which may be used in the
    constructor of a concrete algorithm is the declaration of 
    member variables as properties. All other functionality, 
    i.e. the use of services and the creation of sub-algorithms,
    may be used only in initialise() and afterwards (see the 
    Gaudi user guide).    

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
  class DLLExport Algorithm : virtual public IAlgorithm
  {
   public:
	  
     Algorithm();
     virtual ~Algorithm();

	 
      virtual const std::string& name() const;

	  // IAlgorithm methods
	  
       virtual const std::string& version() const;
       virtual StatusCode initialize();
       virtual StatusCode execute();
       virtual StatusCode finalize();	  
	  
    
    // Protected in Gaudi version
	virtual bool isInitialized() const;
        virtual bool isExecuted() const;
	 virtual bool isFinalized() const;

    
    // Need Algorithm manager/factory before this can be implemented.
	StatusCode createSubAlgorithm(const std::string&, 
				const std::string&, Algorithm*& );
	  
	  /// List of sub-algorithms. Returns a pointer to a vector of (sub) Algorithms
	const std::vector<Algorithm*>& subAlgorithms() const { return m_subAlgms; }
	std::vector<Algorithm*>& subAlgorithms()  { return m_subAlgms; }
	  
	
//	  virtual StatusCode setProperty( const Property& p );
	  virtual StatusCode setProperty( const std::string&);
	  /// Implementation of IProperty::setProperty
	  virtual StatusCode setProperty( const std::string& n, const std::string& v);
//	  virtual StatusCode getProperty(Property* p) const;
//	  virtual const Property& getProperty( const std::string& name) const;
	  virtual StatusCode getProperty( const std::string& n, std::string& v ) const;
//	  virtual const std::vector<Property*>& getProperties( ) const;  
	  
  protected:
	  
	  // Equivalents of Gaudi's initialize, execute & finalize methods
	  /// Virtual method - must be overridden by concrete algorithm
	  virtual StatusCode init () = 0;
	  /// Virtual method - must be overridden by concrete algorithm
	  virtual StatusCode exec () = 0;
	  /// Virtual method - must be overridden by concrete algorithm
	  virtual StatusCode final() = 0;
	  

	  /// Set the Algorithm initialized state
	  void setInitialized();

	  /// Set the executed flag to the specified state
	  // Public in Gaudi - don't know why and will leave here unless we find a reason otherwise
	  //     Also don't know reason for different return type and argument.
	  void setExecuted( bool state );

	  /// Set the Algorithm finalized state
	  void setFinalized();
	  
	  /// Workspace containing input data. Its name should be set via a property called "InputWorkspace"
	  Workspace* m_inputWorkspace;     
	  Workspace* m_outputWorkspace;  /// OutputWorkspace :: Created by the concreate Algorithm
	  
  private:

	  /// Private Copy constructor: NO COPY ALLOWED
	  Algorithm(const Algorithm& a);                 

	  /// Private asignment operator: NO ASSIGNMENT ALLOWED
	  Algorithm& operator=(const Algorithm& rhs);

	  std::string m_name;            ///< Algorithm's name for identification
	  std::string m_version;         ///< Algorithm's version
	  std::vector<Algorithm *> m_subAlgms; ///< Sub algorithms [

	  ///static refenence to the logger class
	  static Logger& g_log;
	  	  
	  bool        m_isInitialized;    ///< Algorithm has been initialized flag
	  bool        m_isExecuted;       ///< Algorithm is executed flag
	  bool        m_isFinalized;      ///< Algorithm has been finalized flag

	  /** Name of workspace in which result should be placed. 
	   *  Its name should be set via a property called "OutputWorkspace"
	   *  Only the concrete algorithm can actually create the output workspace.
	   */
	  std::string m_outputWorkspaceName;

	  /// Temporary way of storing properties for algorithms, in the current absence of a Property class.
	  // N.B. hash_map is not in the standard stl, hence the wierd namespace.
//	  __gnu_cxx::hash_map< std::string, std::string > m_properties;
	  std::map<std::string, std::string > m_properties;

  };
  
} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_ALGORITHM_H_*/
