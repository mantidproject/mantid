#ifndef MANTID_API_SCRIPTWRITER_H_
#define MANTID_API_SCRIPTWRITER_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/DllExport.h"
#include "MantidAPI/ScriptWriterFactory.h"

namespace Mantid
{
  namespace API
  {
    //--------------------------------------------------------------------------
    // Forward declarations
    //--------------------------------------------------------------------------
    class WorkspaceHistory;

    /**
      Defines a ScriptWriter base class that serves to 
      requests for generating a script in a given language.

      The virtual write() method should be overridden
      in a concrete class for a given language.
      
      @author Martyn Gigg, Tessella Plc
      @data 07/04/2011
      
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
      
      File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
      Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class EXPORT_OPT_MANTID_API ScriptWriter 
    {
    public:
      /// Virtual destructor for base class
      virtual ~ScriptWriter() {}
      /// Create a script as a string
      virtual std::string write(const WorkspaceHistory& history) const = 0;
    };

    /* Used to register classes into the factory. creates a global object in an
     * anonymous namespace. The object itself does nothing, but the comma operator
     * is used in the call to its constructor to effect a call to the factory's
     * subscribe method.
     */
#define DECLARE_SCRIPTWRITER(classname) \
    namespace { \
  Mantid::Kernel::RegistrationHelper register_alg_##classname( \
      ((Mantid::API::ScriptWriterFactory::Instance().subscribe<classname>(#classname)) \
          , 0)); \
    }

  } // namespace Mantid
} // namespace API

#endif  /* MANTID_API_SCRIPTWRITER_H_ */
