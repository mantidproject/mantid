#ifndef MANTID_API_COLUMNFACTORY_H_
#define MANTID_API_COLUMNFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidKernel/System.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid
{
	
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}

namespace API
{
  class Column;
}
	
namespace API
{

/** @class ColumnFactoryImpl

    The ColumnFactory class is in charge of the creation of concrete
    instances of columns. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.
    
    @author Roman Tolchenov, Tessella Support Services plc
    @date 31/10/2008
    
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
*/

#ifdef _WIN32
#ifdef IN_MANTID_API
  #define ColumnFactory_DllExport __declspec( dllexport )
#else
  #define ColumnFactory_DllExport __declspec( dllimport )
#endif
#else
  #define ColumnFactory_DllExport
  #define ColumnFactory_DllImport
#endif

  class ColumnFactory_DllExport ColumnFactoryImpl : public Kernel::DynamicFactory<Column>
  {
  public:
	  ///Creates an instance of a column
      boost::shared_ptr<Column> create(const std::string& type) const;

      /*// Column factory specific function to subscribe columns, calls the dynamic factory subscribe function internally
	  template <class C>
      void subscribe()
	  {
          Kernel::DynamicFactory<Algorithm>::subscribe<C>();	
	  }*/

  private:
	friend struct Mantid::Kernel::CreateUsingNew<ColumnFactoryImpl>;

	/// Private Constructor for singleton class
    ColumnFactoryImpl();
	/// Private copy constructor - NO COPY ALLOWED
	ColumnFactoryImpl(const ColumnFactoryImpl&);
	/// Private assignment operator - NO ASSIGNMENT ALLOWED
	ColumnFactoryImpl& operator = (const ColumnFactoryImpl&);
	///Private Destructor
	virtual ~ColumnFactoryImpl();
	///static reference to the logger class
	Kernel::Logger& g_log;
  
  };
  
	///Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
	template class ColumnFactory_DllExport Mantid::Kernel::SingletonHolder<ColumnFactoryImpl>;
#endif /* _WIN32 */
	typedef ColumnFactory_DllExport Mantid::Kernel::SingletonHolder<ColumnFactoryImpl> ColumnFactory;
	
} // namespace API
} // namespace Mantid

#endif /*MANTID_API_COLUMNFACTORY_H_*/
