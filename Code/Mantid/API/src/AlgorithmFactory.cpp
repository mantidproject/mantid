#include "MantidAPI/AlgorithmFactory.h"
#include <iostream>

namespace Mantid
{
namespace API
{
	Kernel::Logger& AlgorithmFactoryImpl::g_log = Kernel::Logger::get("AlgorithmFactory");

	AlgorithmFactoryImpl::AlgorithmFactoryImpl() : Kernel::DynamicFactory<Algorithm>()
	{
		g_log.debug() << "Algorithm Factory created." << std::endl;
	}

	AlgorithmFactoryImpl::~AlgorithmFactoryImpl()
	{
		g_log.debug() << "Algorithm Factory destroyed." << std::endl;
	}

} // namespace API
} // namespace Mantid
