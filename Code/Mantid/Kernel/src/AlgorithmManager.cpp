#include <iomanip>
#include <iostream>
#include <vector>

#include "IAlgorithm.h"
#include "StatusCode.h"
#include "AlgorithmManager.h"
#include "Exception.h"

namespace Mantid
{
namespace Kernel
{
	Logger& AlgorithmManager::g_log = Logger::get("AlgorithmManager");
	AlgorithmManager* AlgorithmManager::m_instance = 0;
	
	AlgorithmManager::AlgorithmManager() : DynamicFactory<IAlgorithm>(),
	   no_of_alg(0)
	{
		std::cout<<"AlgorithmManager == "<<std::setbase(16)
			<<reinterpret_cast<long>(this)
		        <<std::endl;	
	}

	AlgorithmManager::~AlgorithmManager()
	{
		std::cout<<"AlgorithmManager Delete == "<<std::setbase(16)
			<<reinterpret_cast<long>(this)
		        <<std::endl;	
		clear();
	}
        
	IAlgorithm* 
	AlgorithmManager::createUnmanaged(const std::string& algName) const
	  /*!
    	   Creates an instance of an algorithm
	  
	   @param algName The name of the algorithm required
	   @return A pointer to the created algorithm
	   @throw NotFoundError Thrown if algorithm requested is not registered
	 */
	{
	    return DynamicFactory<IAlgorithm>::create(algName);                // Throws on fail:
	}

	IAlgorithm* AlgorithmManager::create(const std::string& algName)
	  /*!
		Creates an instance of an algorithm
	  
	   @param algName The name of the algorithm required
	   @return A pointer to the created algorithm
       @throw NotFoundError Thrown if algorithm requested is not registered
	   @throw std::runtime_error Thrown if properties string is ill-formed
	 */
	{
	   regAlg.push_back(DynamicFactory<IAlgorithm>::create(algName));                // Throws on fail:
	   StatusCode status = regAlg.back()->initialize();
	    if (status.isFailure())
		{
		    throw std::runtime_error("AglorithmManager:: Unable to initialise algorithm " + algName); 
		}
	    no_of_alg++;		
	    return regAlg.back();
	}

	AlgorithmManager* AlgorithmManager::Instance()
	{
	     if (!m_instance) 
		m_instance=new AlgorithmManager;	 
		
	  return m_instance;
	}

	void AlgorithmManager::clear()
	    /// finalizes and deletes all registered algorithms
	{
	     int errOut(0);
	     std::vector<IAlgorithm*>::iterator vc;
	     for(vc=regAlg.begin();vc!=regAlg.end();vc++)
	        {
		      // no test for zero since impossible 
		      StatusCode status = (*vc)->finalize();
		       errOut+= status.isFailure();
		       delete (*vc);
		}
	     regAlg.clear();
	     no_of_alg=0;
	     if (errOut)
                throw std::runtime_error("AlgorithmManager:: Unable to finalise algorithm " ); 
	return;
     }
	
	
} // namespace Kernel
} // namespace Mantid
