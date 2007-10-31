#include "../inc/AlgorithmManager.h"
#include "../inc/IAlgorithm.h"
#include "../inc/StatusCode.h"

using namespace std;

namespace Mantid
{
	Logger& AlgorithmManager::g_log = Logger::get("AlgorithmManager");
	// Initialise the instance pointer to zero
	AlgorithmManager* AlgorithmManager::m_instance = 0;
	int AlgorithmManager::m_no_of_alg=0;
	
	AlgorithmManager::AlgorithmManager()
	{
	}

	AlgorithmManager::~AlgorithmManager()
	{
		delete m_instance;
	}

	IAlgorithm* AlgorithmManager::createAlgorithm(const std::string& algName)
	{
		list.push_back(this->create(algName));
		StatusCode status = list[m_no_of_alg]->initialize();
		if (status.isFailure())
		{
			throw runtime_error("Unable to initialise algorithm " + algName); 
		}
		m_no_of_alg+=1;		
		return list[m_no_of_alg-1];
	}

	AlgorithmManager* AlgorithmManager::Instance()
	{
	  if (!m_instance) 
	  {
		  m_instance=new AlgorithmManager;	 
			  }
	  return m_instance;
	}

	void AlgorithmManager::clear()
	{
		int st_size=list.size();
		for (int loop=st_size-1;loop > -1;loop--)
		{
			if(list[loop] != NULL)
				{
					StatusCode status = list[loop]->finalize();
					if (status.isFailure())
						{
							throw runtime_error("Unable to finalise algorithm " ); 
						}
					delete list[loop];
					m_no_of_alg -= 1;
				}
		}
		list.clear();
	}
} // namespace Mantid