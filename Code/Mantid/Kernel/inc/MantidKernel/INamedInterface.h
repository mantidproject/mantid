#ifndef MANTID_KERNEL_INAMEDINTERFACE_H_
#define MANTID_KERNEL_INAMEDINTERFACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/IInterface.h"
#include <string>

namespace Mantid
{
namespace Kernel
{
  // Interface ID
  // static const InterfaceID IID_INamedInterface("INamedInterface",0,0);

/** @class INamedInterface INamedInterface.h Kernel/INamedInterface.h

    INamedInterface extends IInterface with a method to give all implementing interfaces
    a unique name.

    @author Russell Taylor, Tessella Support Services plc
    @author Based on the Gaudi class of the same name (see http://proj-gaudi.web.cern.ch/proj-gaudi/)
    @date 12/09/2007
    
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
  class DLLExport INamedInterface : virtual public IInterface {
  public:
    /// Retrieve the interface ID
    // static const InterfaceID& interfaceID() { return IID_INamedInterface; }	
	  
    /// Retrieve the name of the instance.
//    virtual const std::string& name() const = 0;
    
    /// Virtual destructor (always needed for abstract classes)
    virtual ~INamedInterface() {};
  };

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_INAMEDINTERFACE_H_*/
