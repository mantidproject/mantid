#ifndef MANTID_KERNEL_ALGORITHMMANAGER_H_
#define MANTID_KERNEL_ALGORITHMMANAGER_H_

/* Used to register classes into the factory. creates a global object in an 
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's 
 * subscribe method.
 */
#define DECLARE_NAMESPACED_ALGORITHM(ns, classname) \
  namespace { \
    Mantid::Kernel::RegistrationHelper register_alg_##classname( \
       ((Mantid::Kernel::AlgorithmManager::Instance()->subscribe<ns::classname>(#classname)) \
       , 0)); \
  }

#define DECLARE_ALGORITHM(classname) \
  namespace { \
    Mantid::Kernel::RegistrationHelper register_alg_##classname( \
       ((Mantid::Kernel::AlgorithmManager::Instance()->subscribe<classname>(#classname)) \
       , 0)); \
  }



//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "AlgorithmFactory.h"
#include <vector>

namespace Mantid
{
namespace Kernel
{
/** @class AlgorithmManager AlgorithmManager.h Kernel/AlgorithmManager.h

	The Algorithm Manager class is responsible for controlling algorithm 
	instances. It provides a facade for the factory, it initializes and
	finalizes algorithms.


	@author Dickon Champion, ISIS, RAL
	@date 30/10/2007
	
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

class DLLExport AlgorithmManager  : public AlgorithmFactory
{
public:

   /** A static method which retrieves the single instance of the Algorithm Manager
	 * 
	 *  @returns A pointer to the Algorithm Manager instance
	 */
	static AlgorithmManager* Instance();


	/** Creates an instance of an algorithm
	 * 
	 *  @param algName The name of the algorithm required
	 *  @return A pointer to the created algorithm
	 * 
	 *  @throw runtime_error Thrown if algorithm requested is not registered
	 */
	IAlgorithm* createAlgorithm(const std::string& algName);

	/// finalizes and deletes all registered algorithms
	void clear();

private:

	/// Private Constructor for singleton class
	AlgorithmManager();

    /** Private destructor
     *  Prevents client from calling 'delete' on the pointer handed 
     *  out by Instance
     */
    virtual ~AlgorithmManager();

 ///static reference to the logger class
	static Logger& g_log;
  /// internal counter of registered algorithms
	static int m_no_of_alg;
  /// vector containing pointers to registered algorithms
	std::vector<IAlgorithm*> list;
	/// Pointer to the Algorithm Manager instance
	static AlgorithmManager* m_instance;
};

} // namespace Kernel
}  //Namespace Mantid

#endif /* MANTID_KERNEL_ALGORITHMMANAGER_H_ */
