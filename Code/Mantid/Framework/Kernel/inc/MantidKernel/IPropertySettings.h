#ifndef MANTID_KERNEL_IPROPERTYSETTINGS_H_
#define MANTID_KERNEL_IPROPERTYSETTINGS_H_
    
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class IPropertyManager;
class Property;

  /** Interface for modifiers to Property's that specify
    if they should be enabled or visible in a GUI.
    They are set on an algorithm via Algorithm::setPropertySettings()
    
    @author Janik Zikovsky
    @date 2011-08-26

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport IPropertySettings 
  {
  public:
    /// Constructor
    IPropertySettings()
    {}

    /// Destructor
    virtual ~IPropertySettings()
    { }

    /** Is the property to be shown as "enabled" in the GUI. Default true. */
    virtual bool isEnabled(const IPropertyManager * algo) const
    { UNUSED_ARG(algo); return true; }

    /** Is the property to be shown in the GUI? Default true. */
    virtual bool isVisible(const IPropertyManager * algo) const
    { UNUSED_ARG(algo); return true; }
    /** to verify if the properties, this one depends on have changed
        or other special condition occurs which needs the framework to react to */
    virtual bool isConditionChanged(const IPropertyManager * algo)const
    { UNUSED_ARG(algo); return false; }
    /** The function user have to overload it in his custom code to modify the property 
        according to the changes to other properties.
     *
     *  Currently it has been tested to modify the property values as function of other properties
     *
     *  Allowed property valies are obtrained from property's allowedValues function, and the purpose the  
     *  function interfaced here is to modify its output.
     *  
     *  allowedValues function on propertyWithValue class obtains its data from a validator, so in the case of 
     *  simple PropertyWithValue, this function has to replace the validator. 
     *  For WorkspaceProperty, which obtains its values from dataservice and filters them by validators, 
     *  a new validator has to be a new filter      */
    virtual void applyChanges(const IPropertyManager * , Property * const)
    {}
 
    //--------------------------------------------------------------------------------------------
    /// Make a copy of the present type of IPropertySettings
    virtual IPropertySettings* clone() = 0;

  protected:

  private:
      // non-copyable directly
      IPropertySettings(const IPropertySettings &){}
  };


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_IPROPERTYSETTINGS_H_ */
