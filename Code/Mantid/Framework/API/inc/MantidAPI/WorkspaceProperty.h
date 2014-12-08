#ifndef MANTID_API_WORKSPACEPROPERTY_H_
#define MANTID_API_WORKSPACEPROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/IWorkspaceProperty.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <string>

namespace Mantid
{
  namespace API
  {
    // -------------------------------------------------------------------------
    // Forward decaration
    // -------------------------------------------------------------------------
    class MatrixWorkspace;

    /// Enumeration for a mandatory/optional property
    struct PropertyMode
    {
      enum Type { Mandatory, Optional };
    };
    /// Enumeration for locking behaviour
    struct LockMode
    {
      enum Type { Lock, NoLock };
    };

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

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
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
      explicit WorkspaceProperty( const std::string &name, const std::string &wsName, const unsigned int direction,
                                  Kernel::IValidator_sptr validator = Kernel::IValidator_sptr(new Kernel::NullValidator)) :
        Kernel::PropertyWithValue <boost::shared_ptr<TYPE> >( name, boost::shared_ptr<TYPE>( ), validator, direction ),
        m_workspaceName( wsName ), m_initialWSName( wsName ), m_optional(PropertyMode::Mandatory), m_locking(LockMode::Lock)
      {
      }

      /** Constructor.
      *  Sets the property and workspace names but initialises the workspace pointer to null.
      *  @param name :: The name to assign to the property
      *  @param wsName :: The name of the workspace
      *  @param direction :: Whether this is a Direction::Input, Direction::Output or Direction::InOut (Input & Output) workspace
      *  @param optional :: If true then the property is optional
      *  @param validator :: The (optional) validator to use for this property
      *  @throw std::out_of_range if the direction argument is not a member of the Direction enum (i.e. 0-2)
      */
      explicit WorkspaceProperty( const std::string &name, const std::string &wsName, const unsigned int direction, 
                                  const PropertyMode::Type optional,
                                  Kernel::IValidator_sptr validator = Kernel::IValidator_sptr(new Kernel::NullValidator) ) :
        Kernel::PropertyWithValue <boost::shared_ptr<TYPE> >( name, boost::shared_ptr<TYPE>( ), validator, direction ),
        m_workspaceName( wsName ), m_initialWSName( wsName ), m_optional(optional), m_locking(LockMode::Lock)
      {
      }

      /** Constructor.
      *  Sets the property and workspace names but initialises the workspace pointer to null.
      *  @param name :: The name to assign to the property
      *  @param wsName :: The name of the workspace
      *  @param direction :: Whether this is a Direction::Input, Direction::Output or Direction::InOut (Input & Output) workspace
      *  @param optional :: A boolean indicating whether the property is mandatory or not. Only matters
      *                     for input properties
      *  @param locking :: A boolean indicating whether the workspace should read or
      *                    write-locked when an algorithm begins. Default=true.
      *  @param validator :: The (optional) validator to use for this property
      *  @throw std::out_of_range if the direction argument is not a member of the Direction enum (i.e. 0-2)
      */
      explicit WorkspaceProperty(const std::string &name, const std::string &wsName, const unsigned int direction, 
                                 const PropertyMode::Type optional, const LockMode::Type locking,
                                 Kernel::IValidator_sptr validator = Kernel::IValidator_sptr(new Kernel::NullValidator)) :
        Kernel::PropertyWithValue <boost::shared_ptr<TYPE> >( name, boost::shared_ptr<TYPE>( ), validator, direction ),
        m_workspaceName( wsName ), m_initialWSName( wsName ), m_optional(optional), m_locking(locking)
      {
      }

      /// Copy constructor, the default name stored in the new object is the same as the default name from the original object
      WorkspaceProperty( const WorkspaceProperty& right ) :
      Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >( right ),
      m_workspaceName( right.m_workspaceName ), m_initialWSName( right.m_initialWSName ), m_optional(right.m_optional), m_locking(right.m_locking)
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
        std::string wsName = value->name();
        if ( this->direction() == Kernel::Direction::Input && !wsName.empty() )
        {
          m_workspaceName = wsName;
        }
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
      WorkspaceProperty<TYPE>* clone() const { return new WorkspaceProperty<TYPE>(*this); }

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
            AnalysisDataService::Instance().retrieveWS<TYPE>(m_workspaceName);
        }
        catch (Kernel::Exception::NotFoundError &)
        {
          // Set to null property if not found
          this->clear();
          //the workspace name is not reset here, however.
        }

        return isValid();
      }

      /** Set a value from a data item
       *  @param value :: A shared pointer to a DataItem. If it is of the correct
       *  type it will set validated, if not the property's value will be cleared. 
       *  @return 
       */
      virtual std::string setDataItem(const boost::shared_ptr<Kernel::DataItem> value)
      {
        boost::shared_ptr<TYPE> typed = boost::dynamic_pointer_cast<TYPE>(value);
        if(typed)
        {
          std::string wsName = typed->name();
          if ( this->direction() == Kernel::Direction::Input && !wsName.empty() )
          {
            m_workspaceName = wsName;
          }
          Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >::m_value = typed;
        }
        else
        {
          this->clear();
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
          return isValidOutputWs();
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
              // Check to see if the workspace is not logged with the ADS because it is optional.
              return isOptionalWs();
            }

            //At this point we have a valid pointer to a Workspace so we need to test whether it is a group
            if( boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(wksp) )
            {
              return isValidGroup(boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(wksp));
            }
            else
            {
              error = "Workspace " + this->value() + " is not of the correct type";
            }
            return error;
          }
        }
        // Call superclass method to access any attached validators (which do their own logging)
        return Kernel::PropertyWithValue<boost::shared_ptr<TYPE> >::isValid();
      }

      /** Indicates if the object is still pointing to the same workspace, using the workspace name
      *  @return true if the value is the same as the initial value or false otherwise
      */
      bool isDefault() const
      {
        return  m_initialWSName == m_workspaceName;
      }

      /** Is the workspace property optional
       * @return true if the workspace can be blank   */
      bool isOptional() const
      {
        return (m_optional == PropertyMode::Optional);
      }
      /** Does the workspace need to be locked before starting an algorithm?
       * @return true (default) if the workspace will be locked */
      bool isLocking() const
      {
        return (m_locking == LockMode::Lock);
      }

      /** Returns the current contents of the AnalysisDataService for input workspaces.
       *  For output workspaces, an empty set is returned
       *  @return set of objects in AnalysisDataService
       */
      virtual std::vector<std::string> allowedValues() const
      {
        if ( this->direction() == Kernel::Direction::Input || this->direction() == Kernel::Direction::InOut )
        {
          // If an input workspace, get the list of workspaces currently in the ADS
          std::set<std::string> vals = AnalysisDataService::Instance().getObjectNames();
          if (isOptional()) // Insert an empty option
          {
            vals.insert("");
          }
          // Copy-construct a temporary workspace property to test the validity of each workspace
          WorkspaceProperty<TYPE> tester(*this);
          std::set<std::string>::iterator it;
          for (it = vals.begin(); it != vals.end();)
          {
            // Remove any workspace that's not valid for this algorithm
            if (!tester.setValue(*it).empty()) 
            { 
              vals.erase(it++); //Post-fix so that it erase the previous when returned
            }
            else ++it;
          }
          return std::vector<std::string>(vals.begin(), vals.end());
        }
        else
        {
          // For output workspaces, just return an empty set
          return std::vector<std::string>();
        }
      }

      /// Create a history record
      /// @return A populated PropertyHistory for this class
      virtual const Kernel::PropertyHistory createHistory() const
      {
        std::string wsName = m_workspaceName;
        bool isdefault = this->isDefault();

        if ((wsName.empty() || this->hasTemporaryValue()) && this->operator()())
        {
          //give the property a temporary name in the history
          std::ostringstream os;
          os << "__TMP" << this->operator()().get();
          wsName = os.str();
          isdefault = false;
        }
        return Kernel::PropertyHistory(this->name(), wsName, this->type(), isdefault, this->direction());
      }

      /** If this is an output workspace, store it into the AnalysisDataService
      *  @return True if the workspace is an output workspace and has been stored
      *  @throw std::runtime_error if unable to store the workspace successfully
      */
      virtual bool store()
      {
        bool result = false;
        if ( ! this->operator()() && isOptional() ) return result;
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

      /** Checks whether the entered workspace group is valid.
      *  To be valid *all* members of the group have to be valid.
      *  @param wsGroup :: the WorkspaceGroup of which to check the validity
      *  @returns A user level description of the problem or "" if it is valid.
      */
      std::string isValidGroup(boost::shared_ptr<WorkspaceGroup> wsGroup) const
      {
        g_log.debug() << " Input WorkspaceGroup found " <<std::endl;

        std::vector<std::string> wsGroupNames = wsGroup->getNames();
        std::vector<std::string>::iterator it = wsGroupNames.begin();

        std::string error;

        // Cycle through each workspace in the group ...
        for( ; it != wsGroupNames.end(); ++it )
        {
          std::string memberWsName = *it;
          boost::shared_ptr<Workspace> memberWs = AnalysisDataService::Instance().retrieve(memberWsName);

          // Table Workspaces are ignored
          if ("TableWorkspace" == memberWs->id())
          {
            error = "Workspace " + memberWsName + " is of type TableWorkspace and will therefore be ignored as part of the GroupedWorkspace.";
            
            g_log.debug() << error << std::endl;
          }
          else
          {
            // ... and if it is a workspace of incorrect type, exclude the group by returning an error.
            if( NULL == boost::dynamic_pointer_cast<TYPE>(memberWs) )
            {
              error = "Workspace " + memberWsName + " is not of type " + Kernel::PropertyWithValue< boost::shared_ptr<TYPE> >::type() + ".";

              g_log.debug() << error << std::endl;

              return error;
            }
            // If it is of the correct type, it may still be invalid. Check.
            else
            {
              Mantid::API::WorkspaceProperty<TYPE> memberWsProperty(*this);
              std::string memberError = memberWsProperty.setValue(memberWsName);
              if( !memberError.empty() )
                return memberError; // Since if this member is invalid, then the whole group is invalid.
            }
          }
        }

        return ""; // Since all members of the group are valid.
      }

      /** Checks whether the entered output workspace is valid.
      *  To be valid the only thing it needs is a name that is allowed by the ADS, @see AnalysisDataServiceImpl
      *  @returns A user level description of the problem or "" if it is valid.
      */
      std::string isValidOutputWs() const
      {
        std::string error("");
        const std::string value = this->value();
        if( !value.empty() )
        {
          // Will the ADS accept it
          error = AnalysisDataService::Instance().isValid(value);
        }
        else
        {
          if( isOptional() ) error = ""; // Optional ones don't need a name
          else error = "Enter a name for the Output workspace";
        }
        return error;
      }

      /** Checks whether the entered workspace (that by this point we've found is not in the ADS)
      *  is actually an optional workspace and so still valid.
      *  @returns A user level description of the problem or "" if it is valid.
      */
      std::string isOptionalWs() const
      {
        std::string error;

        if( m_workspaceName.empty() )
        {
          if( isOptional() )
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

        return error;
      }

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
            AnalysisDataService::Instance().retrieveWS<TYPE>(m_workspaceName);
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
      PropertyMode::Type m_optional;
      /** A flag indicating whether the workspace should be read or write-locked
       * when an algorithm begins. Default=true. */
      LockMode::Type m_locking;

      /// for access to logging streams
      static Kernel::Logger g_log;
    };

    template <typename TYPE>
    Kernel::Logger WorkspaceProperty<TYPE>::g_log("WorkspaceProperty");

  } // namespace API
} // namespace Mantid

#endif /*MANTID_API_WORKSPACEPROPERTY_H_*/
