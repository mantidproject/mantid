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

  /** Interface for modifiers to Property's that specify
   * if they should be enabled or in a GUI.
    
    @author Janik Zikovsky
    @date 2011-08-26

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport IPropertySettings 
  {
  public:
    IPropertySettings()
    : m_propertyManager(NULL)
    {}

    /// Constructor
    IPropertySettings(const IPropertyManager * propManager)
    : m_propertyManager(propManager)
    {}

    /// Destructor
    virtual ~IPropertySettings()
    { }

    /** Is the property to be shown as "enabled" in the GUI. Default true. */
    virtual bool isEnabled() const
    { return true; }

    /** Is the property to be shown in the GUI? Default true. */
    virtual bool isVisible() const
    { return true; }
    /** to verify if the properties, this one depends on are changed
        or other special condition occurs which needs the framework to react to */
    virtual bool isConditionChanged()const
    {return false;}
    //------------------------------------------------------------------------------------------------------------
    /** Set the property manager (i.e. algorithm) containing the other properties to use to validate
     * @param propertyManager :: pointer  */
    void setPropertyManager(const IPropertyManager * propertyManager)
    {
      m_propertyManager = propertyManager;
    }

    //--------------------------------------------------------------------------------------------
    /// Make a copy of the present type of IPropertySettings
    virtual IPropertySettings* clone() = 0;

  protected:

    /** Pointer to the property manager (i.e. algorithm) containing the other properties to use to validate */
    const IPropertyManager * m_propertyManager;

  };


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_IPROPERTYSETTINGS_H_ */
