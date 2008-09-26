#ifndef MANTID_API_WORKSPACEPROPERTY_H_
#define MANTID_API_WORKSPACEPROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include <boost/shared_ptr.hpp>

#include <iostream>

namespace Mantid
{
namespace API
{
/** A property class for workspaces. Inherits from PropertyWithValue, with the value being
    a pointer to the workspace type given to the WorkspaceProperty constructor. This kind
    of property also holds the name of the workspace (as used by the AnalysisDataService)
    and an indication of whether it is an input or output to an algorithm (or both).

    The pointers to the workspaces are fetched from the ADS when the properties are validated
    (i.e. when the PropertyManager::validateProperties() method calls isValid() ).
    Pointers to output workspaces are also fetched, if they exist, and can then be used within
    an algorithm. (An example of when this might be useful is if the user wants to write the
    output into the same workspace as is used for input - this avoids creating a new workspace
    and the overwriting the old one at the end.)

    @author Russell Taylor, Tessella Support Services plc
    @date 10/12/2007

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
template <typename TYPE>
class WorkspaceProperty : public Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >, public IWorkspaceProperty
{
public:
  /** Constructor.
   *  Sets the property and workspace names but initialises the workspace pointer to null.
   *  @param name The name to assign to the property
   *  @param wsName The name of the workspace
   *  @param direction Whether this is a Direction::Input, Direction::Output or Direction::InOut (Input & Output) workspace
   *  @param validator The (optional) validator to use for this property
   *  @throw std::out_of_range if the direction argument is not a member of the Direction enum (i.e. 0-2)
   */
  WorkspaceProperty( const std::string &name, const std::string &wsName, const unsigned int direction,
                     Kernel::IValidator<boost::shared_ptr<TYPE> > *validator = new Kernel::NullValidator<boost::shared_ptr<TYPE> > ) :
    Kernel::PropertyWithValue <boost::shared_ptr<TYPE> >( name, boost::shared_ptr<TYPE>( ), validator, direction ),
    m_workspaceName( wsName )
  {
  }

  /// Copy constructor
  WorkspaceProperty( const WorkspaceProperty& right ) :
    Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >( right ),
    m_workspaceName( right.m_workspaceName )
  {
  }

  /// Copy assignment operator. Only copies the value (i.e. the pointer to the workspace)
  WorkspaceProperty& operator=( const WorkspaceProperty& right )
  {
    if ( &right == this ) return *this;
    Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >::operator=( right );
    return *this;
  }

  // Unhide the base class assignment operator
  using Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >::operator=;

  /// Virtual destructor
  virtual ~WorkspaceProperty()
  {
  }

  /** Get the name of the workspace
   *  @return The workspace's name
   */
  virtual std::string value() const
  {
    return m_workspaceName;
  }

  /** Set the name of the workspace
   *  @param value The new name for the workspace
   */
  virtual bool setValue( const std::string& value )
  {
    if ( ! value.empty() )
    {
      m_workspaceName = value;
      Kernel::PropertyWithValue<boost::shared_ptr<TYPE> >::m_isDefault = false;
      return true;
    }
    // Setting an empty workspace name is not allowed
    return false;
  }

  /** Checks whether the property is valid.
   *  To be valid, in addition to satisfying the conditions of any validators,
   *  a property must not have an empty name and must exist in the AnalysisDataService
   *  if it is an input workspace (Direction::Input or Direction::InOut).
   *  This method also fetches the pointer to an output workspace, if it exists in the ADS.
   *  @returns True if the property is valid, otherwise false.
   */
  virtual const bool isValid() const
  {
    // Assume that any declared WorkspaceProperty must have a name set (i.e. is not an optional property)
    if ( m_workspaceName.empty() ) return false;

    try {
        Workspace_sptr ws = m_value;
        if (ws.get() == NULL)
        {
            ws = AnalysisDataService::Instance().retrieve(m_workspaceName);
        }
        // Check retrieved workspace is the type that it should be
        Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >::m_value = boost::dynamic_pointer_cast<TYPE>(ws);
        if ( ! Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >::m_value ) return false;
    } catch (Kernel::Exception::NotFoundError&) {
        // Only concerned with failing to find workspace in ADS if it's an input type
        if ( !this->operator()() &&
             (( Kernel::PropertyWithValue<boost::shared_ptr<TYPE> >::direction() == 0 ) ||
              ( Kernel::PropertyWithValue<boost::shared_ptr<TYPE> >::direction() == 2 )) )
        {
          return false;
        }
        else
        {
          // Fall through
        }
    }

    // Call superclass method to access any attached validators
    return Kernel::PropertyWithValue<boost::shared_ptr<TYPE> >::isValid();
  }

  /** Returns the current contents of the AnalysisDataService for input workspaces.
   *  For output workspaces, an empty vector is returned
   */
  virtual const std::vector<std::string> allowedValues() const
  {
    if ( ( Kernel::PropertyWithValue<boost::shared_ptr<TYPE> >::direction() == 0 ) ||
         ( Kernel::PropertyWithValue<boost::shared_ptr<TYPE> >::direction() == 2 ) )
    {
      // If an input workspace, get the list of workspaces currently in the ADS
      return AnalysisDataService::Instance().getObjectNames();
    }
    else
    {
      // For output workspaces, just return an empty vector
      return std::vector<std::string>();
    }
  }

  virtual const Kernel::PropertyHistory createHistory() const
  {
    return Kernel::PropertyHistory(this->name(),this->value(),this->type(),this->isDefault(),Kernel::PropertyWithValue<boost::shared_ptr<TYPE> >::direction());
  }

  /** If this is an output workspace, store it into the AnalysisDataService
   *  @return True if the workspace is an output workspace and has been stored
   *  @throw std::runtime_error if unable to store the workspace successfully
   */
  virtual bool store()
  {
    bool result = false;

    if ( Kernel::PropertyWithValue<boost::shared_ptr<TYPE> >::direction() )
    {
      // Check that workspace exists
      if ( ! this->operator()() ) throw std::runtime_error("WorkspaceProperty doesn't point to a workspace");
      // Note use of addOrReplace rather than add
      API::AnalysisDataService::Instance().addOrReplace(m_workspaceName, this->operator()() );
      result = true;
    }
    //always clear the internal pointer after storing
    clear();

    return result;
  }

  Workspace_sptr getWorkspace() const
  {
    return this->operator()();
  }

private:
  /// Reset the pointer to the workspace
  void clear()
  {
    Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >::m_value = boost::shared_ptr<TYPE>();
  }

  /// The name of the workspace (as used by the AnalysisDataService)
  std::string m_workspaceName;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_WORKSPACEPROPERTY_H_*/
