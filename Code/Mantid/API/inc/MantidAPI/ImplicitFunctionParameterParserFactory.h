#ifndef IMPLICIT_FUNCTION_PARAMETER_PARSER_FACTORY_H
#define IMPLICIT_FUNCTION_PARAMETER_PARSER_FACTORY_H


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
#include "ImplicitFunctionParameterParser.h"

namespace Mantid
{
  namespace API
  {
    class EXPORT_OPT_MANTID_API ImplicitFunctionParameterParserFactoryImpl : public Kernel::DynamicFactory<ImplicitFunctionParameterParser>
    {
    public:
      
      virtual boost::shared_ptr<ImplicitFunctionParameterParser> create(const std::string& xmlString) const;

      std::auto_ptr<ImplicitFunctionParameterParser> createImplicitFunctionParameterParserFromXML(const std::string& configXML) const;

    private:
      friend struct Mantid::Kernel::CreateUsingNew<ImplicitFunctionParameterParserFactoryImpl>;

      /// Private Constructor for singleton class
      ImplicitFunctionParameterParserFactoryImpl();	
      /// Private copy constructor - NO COPY ALLOWED
      ImplicitFunctionParameterParserFactoryImpl(const ImplicitFunctionParameterParserFactoryImpl&);
      /// Private assignment operator - NO ASSIGNMENT ALLOWED
      ImplicitFunctionParameterParserFactoryImpl& operator = (const ImplicitFunctionParameterParserFactoryImpl&);
      ///Private Destructor
      virtual ~ImplicitFunctionParameterParserFactoryImpl();
    };

    ///Forward declaration of a specialisation of SingletonHolder for ImplicitFunctionFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
    #ifdef _WIN32
      // this breaks new namespace declaraion rules; need to find a better fix
      template class EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<ImplicitFunctionParameterParserFactoryImpl>;
    #endif /* _WIN32 */

    typedef EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<ImplicitFunctionParameterParserFactoryImpl> ImplicitFunctionParameterParserFactory;

  }
}

#endif