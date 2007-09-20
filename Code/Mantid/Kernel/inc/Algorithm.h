#ifndef ALGORITHM_H_
#define ALGORITHM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "IAlgorithm.h"
#include "MsgStream.h"

#include <vector>
//#include <ext/hash_map>
#include <map>

#ifndef PACKAGE_VERSION
 #define PACKAGE_VERSION "unknown"
#endif

namespace Mantid
{
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
    
    Copyright © 2007 ???RAL???

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
  class Algorithm : virtual public IAlgorithm
  {
  public:
	  
	  /** Constructor
	   *  @param name    The algorithm object's name
	   *  @param svcloc  A pointer to a service location service (RJT: not yet)
	   */
	  Algorithm( const std::string& name, //ISvcLocator *svcloc, 
	             const std::string& version=PACKAGE_VERSION );
	  
	  /// Virtual destructor
	  virtual ~Algorithm();

	  /** The identifying name of the algorithm object. This is the name of a 
	   *  particular instantiation of an algorithm object as opposed to the name 
	   *  of the algorithm itself, e.g. "LinearTrackFit" may be the name of a 
	   *  concrete algorithm class,
	   *  whereas "ApproxTrackFit" and "BestTrackFit" may be two instantiations 
	   *  of the class configured to find tracks with different fit criteria. 
	   */
	  virtual const std::string& name() const;

	  // IAlgorithm methods
	  
	  virtual const std::string& version() const;
	  
	  /** Initialization method invoked by the framework. This method is responsible
	   *  for any bookkeeping of initialization required by the framework itself.
	   *  It will in turn invoke the initialize() method of the derived algorithm,
	   * and of any sub-algorithms which it creates. 
	   */
	  virtual StatusCode initialize();
	  
	  /** The actions to be performed by the algorithm on a dataset. This method is
	   *  invoked for top level algorithms by the application manager.
	   *  This method invokes exec() method. 
	   *  For sub-algorithms either the execute() method or exec() method 
	   *  must be EXPLICITLY invoked by  the parent algorithm.
	   */
	  virtual StatusCode execute();

	  /** System finalization. This method invokes the finalize() method of a 
	   *  concrete algorithm and the finalize() methods of all of that algorithm's 
	   *  sub algorithms. 
	   */ 
	  virtual StatusCode finalize();	  
	  
    /// Has the Algorithm already been initialized?
    // Protected in Gaudi version
	  virtual bool isInitialized() const;

    /// Has this algorithm been executed since the last reset?
    virtual bool isExecuted() const;
	  
    /// Has the Algorithm already been finalized?
    // Protected in Gaudi version
    virtual bool isFinalized() const;

    /** Create a sub algorithm. 
	   *  A call to this method creates a child algorithm object.
	   *  Note that the returned pointer is to Algorithm 
	   *  (as opposed to IAlgorithm), and thus the methods of IProperty 
	   *  are also available for the direct setting of the sub-algorithm's
	   *  properties. Using this mechanism instead of creating daughter 
	   *  algorithms directly via the new operator is prefered since then 
	   *  the framework may take care of all of the necessary book-keeping.
	   * 
	   *  @param type The concrete algorithm class of the sub algorithm
	   *  @param name The name to be given to the sub algorithm
	   *  @param pSubAlg Set to point to the newly created algorithm object
	   */
    // Need Algorithm manager/factory before this can be implemented.
	  StatusCode createSubAlgorithm( const std::string& type, 
	                                 const std::string& name, Algorithm*& pSubAlg );
	  
	  /// List of sub-algorithms. Returns a pointer to a vector of (sub) Algorithms
	  std::vector<Algorithm*>* subAlgorithms() const;
	  
	  /// Implementation of IProperty::setProperty 
//	  virtual StatusCode setProperty( const Property& p );
	  /// Implementation of IProperty::setProperty
	  virtual StatusCode setProperty( const std::string& s );
	  /// Implementation of IProperty::setProperty
	  virtual StatusCode setProperty( const std::string& n, const std::string& v);
	  /// Implementation of IProperty::getProperty
//	  virtual StatusCode getProperty(Property* p) const;
	  /// Implementation of IProperty::getProperty 
//	  virtual const Property& getProperty( const std::string& name) const;
	  /// Implementation of IProperty::getProperty
	  virtual StatusCode getProperty( const std::string& n, std::string& v ) const;
	  /// Implementation of IProperty::getProperties
//	  virtual const std::vector<Property*>& getProperties( ) const;  
	  
  protected:
	  
	  // Equivalents of Gaudi's initialize, execute & finalize methods
	  // Can these be made pure virtual???
	  /// the default (empty) implementation of the init() method
	  virtual StatusCode init () { return StatusCode::SUCCESS ; }
	  /// the default (empty) implementation of the exec() method
	  virtual StatusCode exec () { return StatusCode::SUCCESS ; }
	  /// the default (empty) implementation of the final() method
	  virtual StatusCode final() { return StatusCode::SUCCESS ; }
	  

	  /// Set the Algorithm initialized state
	  void setInitialized();

	  /// Set the executed flag to the specified state
	  // Public in Gaudi - don't know why and will leave here unless we find a reason otherwise
	  //     Also don't know reason for different return type and argument.
	  void setExecuted( bool state );

	  /// Set the Algorithm finalized state
	  void setFinalized();
	  
  private:

	  /// Private Copy constructor: NO COPY ALLOWED
	  Algorithm(const Algorithm& a);                 

	  /// Private asignment operator: NO ASSIGNMENT ALLOWED
	  Algorithm& operator=(const Algorithm& rhs);

	  std::string m_name;            ///< Algorithm's name for identification
	  std::string m_version;         ///< Algorithm's version
	  std::vector<Algorithm *>* m_subAlgms; ///< Sub algorithms
	  	  
	  bool        m_isInitialized;    ///< Algorithm has been initialized flag
    bool        m_isExecuted;       ///< Algorithm is executed flag
	  bool        m_isFinalized;      ///< Algorithm has been finalized flag
	  
	  // RJT: Dummy method so that I don't have to change code before our Message Service exists.
	  int msgSvc() {return 0;}

	  // RJT: Temporary way of storing properties for algorithms, in absence of Property class.
	  // N.B. hash_map is not in the standard stl, hence the wierd namespace.
//	  __gnu_cxx::hash_map< std::string, std::string > m_properties;
	  std::map< std::string, std::string > m_properties;

  };
}

#endif /*ALGORITHM_H_*/
