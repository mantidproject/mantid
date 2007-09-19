#ifndef IALGORITHM_H_
#define IALGORITHM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "IProperty.h"

namespace Mantid
{
// Declaration of the interface ID ( interface id, major version, minor version)
// RJT: Have not yet imported the code for this (in IInterface.h in Gaudi)
//static const InterfaceID IID_IAlgorithm("IAlgorithm", 3 , 0); 

/** @class IAlgorithm IAlgorithm.h Kernel/IAlgorithm.h

    IAlgorithm is the interface implemented by the Algorithm base class.
    Concrete algorithms, derived from the Algorithm base class are controlled 
    via this interface.

    @author Russell Taylor, Tessella Support Services plc
    @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
    @date 11/09/2007
    
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
  class IAlgorithm : virtual public IProperty {
  public:
    // Retrieve interface ID
    //    static const InterfaceID& interfaceID() { return IID_IAlgorithm; }

    /** The version of the algorithm
     */
    virtual const std::string& version() const = 0;
    
    /// Virtual destructor (always needed for abstract classes)
    virtual ~IAlgorithm() {};	  
      
    /** Initialization method invoked by the framework. This method is responsible
        for any bookkeeping of initialization required by the framework itself.
        It will in turn invoke the initialize() method of the derived algorithm,
        and of any sub-algorithms which it creates.
    */
    virtual StatusCode initialize() = 0;

    /** System execution. This method invokes the execute() method of a concrete algorithm.
     */
    virtual StatusCode execute() = 0;

    /** System finalization. This method invokes the finalize() method of a concrete
        algorithm and the finalize() methods of all of that algorithm's sub algorithms.
     */
    virtual StatusCode finalize() = 0;

    /// check if the algorithm is initialized properly
    virtual bool isInitialized() const = 0; 
    /// check if the algorithm is finalized properly 
    virtual bool isFinalized() const = 0; 
    /// check if th ealgorithm is already executed for the current event
    virtual bool isExecuted() const = 0;

  };
}

#endif /*IALGORITHM_H_*/
