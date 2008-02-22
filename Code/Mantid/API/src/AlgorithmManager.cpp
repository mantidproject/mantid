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
    Kernel::Logger& AlgorithmManagerImpl::g_log = Kernel::Logger::get("AlgorithmManager");

    /// Private Constructor for singleton class
    AlgorithmManagerImpl::AlgorithmManagerImpl(): no_of_alg(0)
    {
		g_log.debug() << "Algorithm Manager created." << std::endl;
    }

    /** Private destructor
    *  Prevents client from calling 'delete' on the pointer handed 
    *  out by Instance
    */
    AlgorithmManagerImpl::~AlgorithmManagerImpl()
    {
		g_log.debug() << "Algorithm Manager destroyed." << std::endl;
    }

    /** Creates an instance of an algorithm, but does not own that instance
    * 
    *  @param  algName The name of the algorithm required
    *  @return A pointer to the created algorithm
    *  @throw  NotFoundError Thrown if algorithm requested is not registered
    */
    Algorithm_sptr AlgorithmManagerImpl::createUnmanaged(const std::string& algName) const
    {
      return AlgorithmFactory::Instance().create(algName);                // Throws on fail:
    }

    /** Creates an instance of an algorithm
    *
    *  @param  algName The name of the algorithm required
    *  @return A pointer to the created algorithm
    *  @throw  NotFoundError Thrown if algorithm requested is not registered
    *  @throw  std::runtime_error Thrown if properties string is ill-formed
    */
    Algorithm_sptr AlgorithmManagerImpl::create(const std::string& algName)
    {
      try
      {
        regAlg.push_back(AlgorithmFactory::Instance().create(algName));      // Throws on fail:	   
        regAlg.back()->initialize();
      }
      catch(std::runtime_error& ex)
      {
        g_log.error()<<"AlgorithmManager:: Unable to create algorithm "<< algName <<ex.what();  
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
