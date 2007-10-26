#ifndef MANTID_ALGORITHMFACTORY_H_
#define MANTID_ALGORITHMFACTORY_H_

/* Used to register classes into the factory. creates a global object in an 
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's 
 * subscribe method.
 */
#define DECLARE_ALGORITHM(classname) \
  namespace { \
    Mantid::RegistrationHelper register_alg_##classname( \
       ((Mantid::AlgorithmFactory::Instance()->subscribe<Mantid::classname>(#classname)) \
       , 0)); \
  }

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "DynamicFactory.h"
#include "Logger.h"

namespace Mantid
{

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class IAlgorithm;

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
  class DLLExport AlgorithmFactory : public DynamicFactory<IAlgorithm>
  {
  public:
    
    /** A static method which retrieves the single instance of the Algorithm Factory
     * 
     *  @returns A pointer to the factory instance
     */
    static AlgorithmFactory* Instance();
    
  private:
    
    /// Private Constructor for singleton class
    AlgorithmFactory();
	
	///static reference to the logger class
	static Logger& g_log;
    
    /** Private destructor
     *  Prevents client from calling 'delete' on the pointer handed 
     *  out by Instance
     */
    virtual ~AlgorithmFactory();

    /// Pointer to the factory instance
    static AlgorithmFactory* m_instance;
  };

} // namespace Mantid

#endif /*MANTID_ALGORITHMFACTORY_H_*/
