#include <iomanip>
#include <iostream>
#include <vector>

#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Kernel;

namespace Mantid
{
  namespace API
  {
    Kernel::Logger& AlgorithmManager::g_log = Kernel::Logger::get("AlgorithmManager");
    AlgorithmManager* AlgorithmManager::m_instance = 0;

    /// Private Constructor for singleton class
    AlgorithmManager::AlgorithmManager() : DynamicFactory<IAlgorithm>(),
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
    IAlgorithm* AlgorithmManager::createUnmanaged(const std::string& algName) const
    {
      return DynamicFactory<IAlgorithm>::create(algName);                // Throws on fail:
    }

    /** Creates an instance of an algorithm
    *
    *  @param  algName The name of the algorithm required
    *  @return A pointer to the created algorithm
    *  @throw  NotFoundError Thrown if algorithm requested is not registered
    *  @throw  std::runtime_error Thrown if properties string is ill-formed
    */
    IAlgorithm* AlgorithmManager::create(const std::string& algName)
    {
      try
      {
        regAlg.push_back(DynamicFactory<IAlgorithm>::create(algName));      // Throws on fail:	   
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

    /// Finalizes and deletes all registered algorithms
    void AlgorithmManager::clear()
    {
      int errOut(0);
      std::vector<IAlgorithm*>::iterator vc;
      try
      {
        for(vc=regAlg.begin();vc!=regAlg.end();vc++)
        {
          // no test for zero since impossible 
          (*vc)->finalize();
          delete (*vc);
        }
        regAlg.clear();
        no_of_alg=0;
      }
      catch(std::runtime_error& ex)
      {
        g_log.error()<<"AlgorithmManager:: Unable to finalise all algorithms"<<ex.what();
        throw std::runtime_error("AlgorithmManager:: Unable to finalise all algorithms "); 
      }
      return;
    }

  } // namespace API
} // namespace Mantid
