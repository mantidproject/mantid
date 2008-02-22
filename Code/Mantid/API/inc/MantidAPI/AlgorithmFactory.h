#ifndef MANTID_KERNEL_ALGORITHMFACTORY_H_
#define MANTID_KERNEL_ALGORITHMFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/SingletonHolder.h"

#ifdef IN_MANTID_API
#define EXPORT_OPT_MANTID_API DLLExport 
#else
#define EXPORT_OPT_MANTID_API DLLImport
#endif /* IN_MANTID_API */

namespace Mantid
{
namespace API
{

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Algorithm;

/** @class AlgorithmFactory AlgorithmFactory.h Kernel/AlgorithmFactory.h

    The AlgorithmFactory class is in charge of the creation of concrete
    instances of Algorithms. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.
    
    @author Russell Taylor, Tessella Support Services plc
    @date 21/09/2007
    
    Copyright &copy; 2007 ???RAL???

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
class EXPORT_OPT_MANTID_API AlgorithmFactoryImpl : public Kernel::DynamicFactory<Algorithm>
  {
  public:
 
  protected:    
    /// Protected Constructor for singleton class
    AlgorithmFactoryImpl();	
  
    /** Protected destructor
     *  Prevents client from calling 'delete' on the pointer handed 
     *  out by Instance
     */
    virtual ~AlgorithmFactoryImpl();

  private:
	friend struct Mantid::Kernel::CreateUsingNew<AlgorithmFactoryImpl>;
	  
	/// Private copy constructor - NO COPY ALLOWED
	AlgorithmFactoryImpl(const AlgorithmFactoryImpl&);
	/// Private assignment operator - NO ASSIGNMENT ALLOWED
	AlgorithmFactoryImpl& operator = (const AlgorithmFactoryImpl&);

	///static reference to the logger class
	static Kernel::Logger& g_log;

  };
  
	///Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
	template class EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<AlgorithmFactoryImpl>;
#endif /* _WIN32 */
	typedef EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<AlgorithmFactoryImpl> AlgorithmFactory;
	
} // namespace API
} // namespace Mantid

#endif /*MANTID_KERNEL_ALGORITHMFACTORY_H_*/
