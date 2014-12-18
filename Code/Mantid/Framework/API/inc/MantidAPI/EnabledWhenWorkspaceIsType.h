#ifndef MANTID_API_ENABLEDWHENWORKSPACEISTYPE_H_
#define MANTID_API_ENABLEDWHENWORKSPACEISTYPE_H_
    
#include "MantidKernel/System.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/DataService.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidAPI/AnalysisDataService.h"

namespace Mantid
{
namespace API
{

  /** Show a property as enabled when the workspace pointed to by another
   * is of a given type
    
    @author Janik Zikovsky
    @date 2011-09-21

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
  template <typename T>
  class DLLExport EnabledWhenWorkspaceIsType : public Kernel::IPropertySettings
  {
  public:
    //--------------------------------------------------------------------------------------------
    /** Constructor
     * @param otherPropName :: Name of the OTHER property that we will check.
     * @param enabledSetting :: Set Enabled on this property to this value when the workspace is of type T. Default true.
     */
    EnabledWhenWorkspaceIsType(std::string otherPropName,
            bool enabledSetting = true)
    : IPropertySettings(),
      m_otherPropName(otherPropName), m_enabledSetting(enabledSetting)
    {
    }

    /// Destructor
    virtual ~EnabledWhenWorkspaceIsType()
    {}
    

    //--------------------------------------------------------------------------------------------
    /** Does the validator fulfill the criterion based on the
     * other property values?
     * @return true if fulfilled or if any problem was found (missing property, e.g.).
     */
    virtual bool fulfillsCriterion(const Kernel::IPropertyManager * algo) const
    {
      // Find the property
      if (algo == NULL) return true;
      Mantid::Kernel::Property * prop = NULL;
      try
      {
        prop = algo->getPointerToProperty(m_otherPropName);
      }
      catch (Mantid::Kernel::Exception::NotFoundError&)
      {
        return true; //Property not found. Ignore
      }
      if (!prop) return true;

      // Value of the other property
      std::string propValue = prop->value();
      if (propValue.empty())
        return true;

      Workspace_sptr ws;
      try
      {
        ws = Mantid::API::AnalysisDataService::Instance().retrieve(propValue);
      }
      catch (...)
      {
        return true;
      }
      // Does it cast to the desired type?
      boost::shared_ptr<T> castWS = boost::dynamic_pointer_cast<T>(ws);
      if (castWS)
        return m_enabledSetting;
      else
        return !m_enabledSetting;
    }

    //--------------------------------------------------------------------------------------------
    /// Return true/false based on whether the other property satisfies the criterion
    virtual bool isEnabled(const Kernel::IPropertyManager * algo) const
    {
      return fulfillsCriterion(algo);
    }

    //--------------------------------------------------------------------------------------------
    /// Return true always
    virtual bool isVisible(const Kernel::IPropertyManager * ) const
    {
      return true;
    }

    //--------------------------------------------------------------------------------------------
    /// Make a copy of the present type of validator
    virtual IPropertySettings * clone()
    {
      EnabledWhenWorkspaceIsType * out = new EnabledWhenWorkspaceIsType<T>(m_otherPropName, m_enabledSetting);
      return out;
    }
   protected:
    /// Name of the OTHER property that we will check.
    std::string m_otherPropName;
    /// Set Enabled to this.
    bool m_enabledSetting;
  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_ENABLEDWHENWORKSPACEISTYPE_H_ */
