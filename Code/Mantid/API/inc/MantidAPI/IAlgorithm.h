#ifndef MANTID_KERNEL_IALGORITHM_H_
#define MANTID_KERNEL_IALGORITHM_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidKernel/IProperty.h"

namespace Mantid
{
namespace API
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
    
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.    
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
  class DLLExport IAlgorithm : virtual public Kernel::IProperty 
  {
  public:
    // Retrieve interface ID
    //    static const InterfaceID& interfaceID() { return IID_IAlgorithm; }

    /// The version of the algorithm
    virtual const std::string& version() const = 0;
    
    /// Virtual destructor (always needed for abstract classes)
    virtual ~IAlgorithm() {};	  
      
    /** Initialization method invoked by the framework. This method is responsible
     *  for any bookkeeping of initialization required by the framework itself.
     *  It will in turn invoke the init() method of the derived algorithm,
     *  and of any sub-algorithms which it creates.
     * 	 
	 */
    virtual void initialize() = 0;

    /** System execution. This method invokes the exec() method of a concrete algorithm.
     * 
     */
    virtual void execute() = 0;

    /** System finalization. This method invokes the final() method of a concrete
     *  algorithm and the final() methods of all of that algorithm's sub algorithms.
     * 
     */
    virtual void finalize() = 0; 

    /// Check whether the algorithm is initialized properly
    virtual bool isInitialized() const = 0; 
    /// Check whether the algorithm is finalized properly 
    virtual bool isFinalized() const = 0; 
    /// Check whether the algorithm has already been executed
    virtual bool isExecuted() const = 0;
  };

} // namespace API
} // namespace Mantid

#endif /*MANTID_KERNEL_IALGORITHM_H_*/
