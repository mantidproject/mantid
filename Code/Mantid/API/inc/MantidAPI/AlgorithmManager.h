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
       ((Mantid::API::AlgorithmManager::Instance().subscribe<ns::classname>(#classname)) \
       , 0)); \
  }

#define DECLARE_ALGORITHM(classname) \
  namespace { \
    Mantid::Kernel::RegistrationHelper register_alg_##classname( \
       ((Mantid::API::AlgorithmManager::Instance().subscribe<classname>(#classname)) \
       , 0)); \
  }



//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmFactory.h"
#include "boost/shared_ptr.hpp"
#include <vector>
#include "MantidKernel/SingletonHolder.h"

namespace Mantid
{
namespace API
{
//Forward declarations
  class Algorithm;

  ///@cond (Don't document here because done elsewhere
  //Typedef for a shared pointer to an Algorithm
  typedef boost::shared_ptr<Algorithm> Algorithm_sptr;
  ///@endcond
/** @class AlgorithmManagerImpl AlgorithmManager.h Kernel/AlgorithmManager.h

	  The AlgorithmManagerImpl class is responsible for controlling algorithm 
	  instances. It incorporates the algorithm factory and initializes algorithms.

	  @author Dickon Champion, ISIS, RAL
	  @date 30/10/2007

	  Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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


class DLLExport AlgorithmManagerImpl  : public AlgorithmFactory
{
public:

	// Methods to create algorithm instances
	Algorithm_sptr create( const std::string& algName );
	Algorithm_sptr createUnmanaged( const std::string& algName ) const;

	/// deletes all registered algorithms
	void clear();
	
	/** Gives the number of managed algorithms
	 *  @return The number of registered algorithms
	 */
  int size() const { return regAlg.size(); }

private:
	friend class Mantid::Kernel::CreateUsingNew<AlgorithmManagerImpl>;

	//Class cannot be instantiated by normal means
	AlgorithmManagerImpl();
	~AlgorithmManagerImpl();
	AlgorithmManagerImpl(const AlgorithmManagerImpl&);
	AlgorithmManagerImpl& operator = (const AlgorithmManagerImpl&);


  /// Static reference to the logger class
	static Kernel::Logger& g_log;
 
	int no_of_alg;                       ///< counter of registered algorithms
	std::vector<Algorithm_sptr> regAlg;     ///<  pointers to registered algorithms [policy???]
};

///The singleton definition of AlgorithmManagerImpl
typedef Mantid::Kernel::SingletonHolder<AlgorithmManagerImpl> AlgorithmManager;

} // namespace API
}  //Namespace Mantid

#endif /* MANTID_KERNEL_ALGORITHMMANAGER_H_ */
