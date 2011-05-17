#ifndef MANTID_API_WORKSPACEPROPERTY_H_
#define MANTID_API_WORKSPACEPROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include <boost/shared_ptr.hpp>
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <iostream>
#include <string>

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

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    template <typename TYPE = MatrixWorkspace>
    class WorkspaceProperty : public Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >, public IWorkspaceProperty
    {
    public:
      /** Constructor.
      *  Sets the property and workspace names but initialises the workspace pointer to null.
      *  @param name :: The name to assign to the property
      *  @param wsName :: The name of the workspace
      *  @param direction :: Whether this is a Direction::Input, Direction::Output or Direction::InOut (Input & Output) workspace
      *  @param validator :: The (optional) validator to use for this property
      *  @throw std::out_of_range if the direction argument is not a member of the Direction enum (i.e. 0-2)
      */
      WorkspaceProperty( const std::string &name, const std::string &wsName, const unsigned int direction,
          Kernel::IValidator<boost::shared_ptr<TYPE> > *validator = new Kernel::NullValidator<boost::shared_ptr<TYPE> > ) :
        Kernel::PropertyWithValue <boost::shared_ptr<TYPE> >( name, boost::shared_ptr<TYPE>( ), validator, direction ),
        m_workspaceName( wsName ), m_initialWSName( wsName ), m_optional(false)
      {
      }

      /** Constructor.
      *  Sets the property and workspace names but initialises the workspace pointer to null.
      *  @param name :: The name to assign to the property
      *  @param wsName :: The name of the workspace
      *  @param direction :: Whether this is a Direction::Input, Direction::Output or Direction::InOut (Input & Output) workspace
      *  @param optional :: A boolean indicating whether the property is mandatory or not. Only matters 
      *                     for input properties
      *  @param validator :: The (optional) validator to use for this property
      *  @throw std::out_of_range if the direction argument is not a member of the Direction enum (i.e. 0-2)
      */
      explicit WorkspaceProperty(const std::string &name, const std::string &wsName, const unsigned int direction, bool optional,
          Kernel::IValidator<boost::shared_ptr<TYPE> > *validator = new Kernel::NullValidator<boost::shared_ptr<TYPE> > ) :
        Kernel::PropertyWithValue <boost::shared_ptr<TYPE> >( name, boost::shared_ptr<TYPE>( ), validator, direction ),
        m_workspaceName( wsName ), m_initialWSName( wsName ), m_optional(optional)
      {
      }

      /// Copy constructor, the default name stored in the new object is the same as the default name from the original object
      WorkspaceProperty( const WorkspaceProperty& right ) :
      Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >( right ),
      m_workspaceName( right.m_workspaceName ), m_initialWSName( right.m_initialWSName ), m_optional(right.m_optional)
      {    
      }

      /// Copy assignment operator. Only copies the value (i.e. the pointer to the workspace)
      WorkspaceProperty& operator=( const WorkspaceProperty& right )
      {
        if ( &right == this ) return *this;
        Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >::operator=( right );
        return *this;
      }

      /** Bring in the PropertyWithValue assignment operator explicitly (avoids VSC++ warning)
       * @param value :: The value to set to
       * @return assigned PropertyWithValue
       */
      virtual boost::shared_ptr<TYPE>& operator=( const boost::shared_ptr<TYPE>& value )
      {
        return Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >::operator=( value );
      }

      //--------------------------------------------------------------------------------------
      ///Add the value of another property
      virtual WorkspaceProperty& operator+=( Kernel::Property const * )
      {
        throw Kernel::Exception::NotImplementedError("+= operator is not implemented for WorkspaceProperty.");
        return *this;
      }

      /// 'Virtual copy constructor'
      Kernel::Property* clone() { return new WorkspaceProperty<TYPE>(*this); }

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

      /** Get the value the property was initialised with -its default value
      *  @return The default value
      */
      virtual std::string getDefault() const
      {
        return m_initialWSName;
      }

      /** Set the name of the workspace.
      *  Also tries to retrieve it from the AnalysisDataService.
      *  @param value :: The new name for the workspace
      *  @return 
      */
      virtual std::string setValue( const std::string& value )
      {
        m_workspaceName = value;
        // Try and get the workspace from the ADS, but don't worry if we can't
        try {
          Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >::m_value =
            boost::dynamic_pointer_cast<TYPE>(AnalysisDataService::Instance().retrieve(m_workspaceName));
        }
        catch (Kernel::Exception::NotFoundError &)
        {
          // Set to null property if not found
          this->clear();
          //the workspace name is not reset here, however.
        }

        return isValid();
      }

      /** Checks whether the entered workspace is valid.
      *  To be valid, in addition to satisfying the conditions of any validators,
      *  an output property must not have an empty name and an input one must point to
      *  a workspace of the correct type.
      *  @returns A user level description of the problem or "" if it is valid.
      */
      std::string isValid() const 
      {
        //start with the no error condition
        std::string error = "";

        // If an output workspace it must have a name, although it might not exist in the ADS yet
        if ( this->direction() == Kernel::Direction::Output ) 
        {
          if ( !this->value().empty() )
          {
            //it has a name and that is enough so return the success
            return "";
          }
          else
          {
            if( m_optional ) return "";
            //Return a user level error
            error = "Enter a name for the Output workspace";
            //the debug message has more detail to put it in context
            g_log.debug() << "Problem validating workspace: " << error << std::endl;
            return error;
          }
        }

        // If an input (or inout) workspace, must point to something, although it doesn't have to have a name
        // unless it's optional
        if ( this->direction() == Kernel::Direction::Input || this->direction() == Kernel::Direction::InOut )
        {
          // Workspace groups will not have a value since they are not of type TYPE
          if ( !Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >::m_value ) 
          {
            Mantid::API::Workspace_sptr wksp;
            try 
            {
              wksp = AnalysisDataService::Instance().retrieve(m_workspaceName);
            }
            catch( Kernel::Exception::NotFoundError &)
            {
              if( m_workspaceName.empty() ) 
              {
                if( m_optional )
                {
                  error = "";
                }
                else
                {
                  error = "Enter a name for the Input/InOut workspace";
                }
              }
              else
              {
                error = "Workspace \"" + this->value() + "\" was not found in the Analysis Data Service";
              }
              if( !error.empty() )
                g_log.debug() << "Problem validating workspace: " << error << "." << std::endl;
              return error;
            }

            //At this point we have a valid pointer to a Workspace so we need to test whether it is a group
            if( boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(wksp) )
            {
              g_log.debug() << " Input WorkspaceGroup found " <<std::endl;
            }
            else
            {
              error = "Workspace " + this->value() + " is not of the correct type";
              g_log.debug() << "Problem validating workspace: " << error << ".  \""
                  << m_workspaceName << "\" is not of type " << Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >::type() << std::endl;
            }
            return error;
          }
        }
        // Call superclass method to access any attached validators (which do their own logging)
        return Kernel::PropertyWithValue<boost::shared_ptr<TYPE> >::isValid();
      }

      /** Indicates if the object is still pointing to the same workspace, using the worksapce name
      *  @return true if the value is the same as the initial value or false otherwise
      */
      bool isDefault() const
      {
        return  m_initialWSName == m_workspaceName;
      }

      /** Returns the current contents of the AnalysisDataService for input workspaces.
       *  For output workspaces, an empty set is returned
       *  @return set of objects in AnalysisDataService
       */
      virtual std::set<std::string> allowedValues() const
      {
        if ( this->direction() == 0 || this->direction() == 2 )
        {
          // If an input workspace, get the list of workspaces currently in the ADS
          if (m_optional)
          {
            std::set<std::string> vals = AnalysisDataService::Instance().getObjectNames();
            vals.insert("");
            return vals;
          }
          else
          {
            return AnalysisDataService::Instance().getObjectNames();
          }
        }
        else
        {
          // For output workspaces, just return an empty set
          return std::set<std::string>();
        }
      }

      /// Create a history record
      /// @return A populated PropertyHistory for this class
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
        if ( ! this->operator()() && m_optional ) return result;
        if ( this->direction() ) // Output or InOut
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

      /** Attempts to retreive the data from the ADS
      *  if the data is not foung the internal pointer is set to null.
      */
      void retrieveWorkspaceFromADS()
      {
        // Try and get the workspace from the ADS, but don't worry if we can't
        try {
          Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >::m_value =
            boost::dynamic_pointer_cast<TYPE>(AnalysisDataService::Instance().retrieve(m_workspaceName));
        }
        catch (Kernel::Exception::NotFoundError &)
        {
          // Set to null property if not found
          this->clear();
        }
      }

      /// The name of the workspace (as used by the AnalysisDataService)
      std::string m_workspaceName;
      /// The name of the workspace that the this this object was created for
      std::string m_initialWSName;
      /// A flag indicating whether the property should be considered optional. Only matters for input workspaces
      bool m_optional;

      /// for access to logging streams
      static Kernel::Logger& g_log;
    };

    template <typename TYPE>
    Kernel::Logger& WorkspaceProperty<TYPE>::g_log = Kernel::Logger::get("WorkspaceProperty");

  } // namespace API
} // namespace Mantid

#endif /*MANTID_API_WORKSPACEPROPERTY_H_*/
