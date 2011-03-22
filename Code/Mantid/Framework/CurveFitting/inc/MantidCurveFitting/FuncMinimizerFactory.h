#ifndef MANTID_CURVEFITTING_FUNCMINIMIZERFACTORY_H_
#define MANTID_CURVEFITTING_FUNCMINIMIZERFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidAPI/DllExport.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid
{
namespace CurveFitting
{

//----------------------------------------------------------------------
// More forward declarations
//----------------------------------------------------------------------
  class IFuncMinimizer;

/** @class FuncMinimizerFactoryImpl

    The FuncMinimizerFactory class is in charge of the creation of concrete
    instances of minimizers. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.
    
    @author Anders Markvardsen, ISIS, RAL
    @date 20/05/2010
    
    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  class DLLExport FuncMinimizerFactoryImpl : public Kernel::DynamicFactory<IFuncMinimizer>
  {
  public:
    /**Creates an instance of a function
     * @param type :: The function's type
     * @return A pointer to the created function
     */
    IFuncMinimizer* createFunction(const std::string& type) const;


  private:
    friend struct Mantid::Kernel::CreateUsingNew<FuncMinimizerFactoryImpl>;
    /// Private Constructor for singleton class
    FuncMinimizerFactoryImpl();

    ///static reference to the logger class
    Kernel::Logger& g_log;
  };
  
	///Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
	template class Mantid::Kernel::SingletonHolder<FuncMinimizerFactoryImpl>;
#endif /* _WIN32 */
	typedef Mantid::Kernel::SingletonHolder<FuncMinimizerFactoryImpl> FuncMinimizerFactory;
	
} // namespace API
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FUNCMINIMIZERFACTORY_H_*/
