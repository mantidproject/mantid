#include <iomanip>
#include <iostream>
#include <vector>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Kernel;

namespace Mantid
{
  namespace API
  {

    /// Private Constructor for singleton class
    AlgorithmManagerImpl::AlgorithmManagerImpl(): g_log(Kernel::Logger::get("AlgorithmManager")), no_of_alg(0)
    {
		std::cerr << "Algorithm Manager created." << std::endl;
		g_log.debug() << "Algorithm Manager created." << std::endl;
    }

    /** Private destructor
    *  Prevents client from calling 'delete' on the pointer handed 
    *  out by Instance
    */
    AlgorithmManagerImpl::~AlgorithmManagerImpl()
    {
		std::cerr << "Algorithm Manager destroyed." << std::endl;
//		g_log.debug() << "Algorithm Manager destroyed." << std::endl;
    }

    /** Creates an instance of an algorithm, but does not own that instance
    * 
    *  @param  algName The name of the algorithm required
	*  @param  version The version of the algorithm required, if not defined most recent version is used -> version =-1
    *  @return A pointer to the created algorithm
    *  @throw  NotFoundError Thrown if algorithm requested is not registered
    */
    Algorithm_sptr AlgorithmManagerImpl::createUnmanaged(const std::string& algName,const int& version) const
    {
      return AlgorithmFactory::Instance().create(algName,version);                // Throws on fail:
    }
    
    /** Gets the names of all the currently available algorithms
    *
    *  \return A pointer to the created algorithm
    */
    const std::vector<std::string> AlgorithmManagerImpl::getNames() const
    {
	std::vector<std::string> names;
	    
	for (unsigned int i=0; i < regAlg.size(); ++i)
	{
		names.push_back(regAlg[i]->name());
	}
	    
        return names;              
    }

    /** Creates an instance of an algorithm
    *
    *  @param  algName The name of the algorithm required
	*  @param  version The version of the algorithm required, if not defined most recent version is used -> version =-1
    *  @return A pointer to the created algorithm
    *  @throw  NotFoundError Thrown if algorithm requested is not registered
    *  @throw  std::runtime_error Thrown if properties string is ill-formed
    */
    Algorithm_sptr AlgorithmManagerImpl::create(const std::string& algName, const int& version)
    {
      try
      {
        regAlg.push_back(AlgorithmFactory::Instance().create(algName,version));      // Throws on fail:	   
        regAlg.back()->initialize();
      }
      catch(std::runtime_error& ex)
      {
        g_log.error()<<"AlgorithmManager:: Unable to create algorithm "<< algName <<ex.what() << std::endl;  
        throw std::runtime_error("AlgorithmManager:: Unable to create algorithm " + algName); 
      }
      no_of_alg++;		
      return regAlg.back();
    }

    /// deletes all registered algorithms
    void AlgorithmManagerImpl::clear()
    {
      std::vector<Algorithm_sptr>::iterator vc;
      regAlg.clear();
      no_of_alg=0;
      return;
    }

  } // namespace API
} // namespace Mantid
