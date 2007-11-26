#include "MantidAPI/AlgorithmFactory.h"

namespace Mantid
{
namespace API
{
	Kernel::Logger& AlgorithmFactory::g_log = Kernel::Logger::get("AlgorithmFactory");

	// Initialise the instance pointer to zero
	AlgorithmFactory* AlgorithmFactory::m_instance = 0;

	AlgorithmFactory::AlgorithmFactory()
	{
	}

	AlgorithmFactory::~AlgorithmFactory()
	{
	  delete m_instance;
	}

	AlgorithmFactory* AlgorithmFactory::Instance()
	{
	  if (!m_instance) m_instance=new AlgorithmFactory;
	  return m_instance;
	}

} // namespace API
} // namespace Mantid
