#ifndef IMPLICIT_FUNCTION_FACTORY
#define IMPLICIT_FUNCTION_FACTORY

/** @class ImplicitFunctionFactory ImplicitFunctionFactory.h Kernel/ImplicitFunctionFactory.h

This dynamic factory implementation generates concrete instances of ImplicitFunctions.

    @author Owen Arnold, Tessella Support Services plc
    @date 27/10/2010

Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include "DllExport.h"
#include "ImplicitFunction.h"
#include "ImplicitFunctionParserFactory.h"


namespace Mantid
{
  namespace API
  {
    class EXPORT_OPT_MANTID_API ImplicitFunctionFactoryImpl : public Kernel::DynamicFactory<ImplicitFunction>
    {
    public:

      virtual boost::shared_ptr<ImplicitFunction> create(const std::string& className) const;

      virtual ImplicitFunction* createUnwrapped(Poco::XML::Element* processXML) const;

      virtual ImplicitFunction* createUnwrapped(const std::string& processXML) const;

      friend struct Mantid::Kernel::CreateUsingNew<ImplicitFunctionFactoryImpl>;

      /// Private Constructor for singleton class
      ImplicitFunctionFactoryImpl();	
      /// Private copy constructor - NO COPY ALLOWED
      ImplicitFunctionFactoryImpl(const ImplicitFunctionFactoryImpl&);
      /// Private assignment operator - NO ASSIGNMENT ALLOWED
      ImplicitFunctionFactoryImpl& operator = (const ImplicitFunctionFactoryImpl&);
      ///Private Destructor
      virtual ~ImplicitFunctionFactoryImpl();
    };

    ///Forward declaration of a specialisation of SingletonHolder for ImplicitFunctionFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
    #ifdef _WIN32
      // this breaks new namespace declaraion rules; need to find a better fix
      template class EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<ImplicitFunctionFactoryImpl>;
    #endif /* _WIN32 */

    typedef EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<ImplicitFunctionFactoryImpl> ImplicitFunctionFactory;

  }
}

#endif
