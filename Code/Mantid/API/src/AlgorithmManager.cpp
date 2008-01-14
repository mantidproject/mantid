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
    Kernel::Logger& AlgorithmManager::g_log = Kernel::Logger::get("AlgorithmManager");
    AlgorithmManager* AlgorithmManager::m_instance = 0;

    /// Private Constructor for singleton class
    AlgorithmManager::AlgorithmManager() : AlgorithmFactory(),
      no_of_alg(0)
    {
    }

    /** Private destructor
    *  Prevents client from calling 'delete' on the pointer handed 
    *  out by Instance
    */
    AlgorithmManager::~AlgorithmManager()
    {
      clear();
    }

    /** Creates an instance of an algorithm, but does not own that instance
    * 
    *  @param  algName The name of the algorithm required
    *  @return A pointer to the created algorithm
    *  @throw  NotFoundError Thrown if algorithm requested is not registered
    */
    Algorithm_sptr AlgorithmManager::createUnmanaged(const std::string& algName) const
    {
      return AlgorithmFactory::create(algName);                // Throws on fail:
    }

    /** Creates an instance of an algorithm
    *
    *  @param  algName The name of the algorithm required
    *  @return A pointer to the created algorithm
    *  @throw  NotFoundError Thrown if algorithm requested is not registered
    *  @throw  std::runtime_error Thrown if properties string is ill-formed
    */
    Algorithm_sptr AlgorithmManager::create(const std::string& algName)
    {
      try
      {
        regAlg.push_back(AlgorithmFactory::create(algName));      // Throws on fail:	   
        regAlg.back()->initialize();
      }
      catch(std::runtime_error& ex)
      {
        g_log.error()<<"AlgorithmManager:: Unable to create algorithm "<< algName<<ex.what();  
        throw std::runtime_error("AlgorithmManager:: Unable to create algorithm " + algName); 
      }
      no_of_alg++;		
      return regAlg.back();
    }

    /** A static method which retrieves the single instance of the Algorithm Manager
    * 
    *  @returns A pointer to the Algorithm Manager instance
    */
    AlgorithmManager* AlgorithmManager::Instance()
    {
      if (!m_instance) m_instance = new AlgorithmManager;	 		
      return m_instance;
    }

    /// deletes all registered algorithms
    void AlgorithmManager::clear()
    {
      std::vector<Algorithm_sptr>::iterator vc;
      regAlg.clear();
      no_of_alg=0;
      return;
    }

  } // namespace API
} // namespace Mantid
