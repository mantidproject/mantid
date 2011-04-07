#ifndef MANTID_API_SCRIPTWRITERFACTORY_H_
#define MANTID_API_SCRIPTWRITERFACTORY_H_
    
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/DllExport.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid
{
  namespace API
  {
    //--------------------------------------------------------------------------
    // Forward declarations
    //--------------------------------------------------------------------------
    class ScriptWriter;

    /**
      A factory for script writers objects to avoid dependencies on higher up 
      libraries

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
    class EXPORT_OPT_MANTID_API ScriptWriterFactoryImpl : public Kernel::DynamicFactory<ScriptWriter>
    {
    private:
      /// Allow the singleton holder to create an object
      friend struct Mantid::Kernel::CreateUsingNew<ScriptWriterFactoryImpl>;
      /// Private Constructor for singleton class
      ScriptWriterFactoryImpl() {};	
      /// Private copy constructor - NO COPY ALLOWED
      ScriptWriterFactoryImpl(const ScriptWriterFactoryImpl&);
      /// Private assignment operator - NO ASSIGNMENT ALLOWED
      ScriptWriterFactoryImpl& operator = (const ScriptWriterFactoryImpl&);
      ///Private Destructor
      virtual ~ScriptWriterFactoryImpl() {};
    };

    ///Forward declaration of a specialisation of SingletonHolder 
    ///for ScriptWriterFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
    // this breaks new namespace declaraion rules; need to find a better fix
    template class EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<ScriptWriterFactoryImpl>;
#endif /* _WIN32 */

typedef EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<ScriptWriterFactoryImpl> ScriptWriterFactory;

    
  } // namespace Mantid
} // namespace API

#endif  /* MANTID_API_SCRIPTWRITERFACTORY_H_ */
