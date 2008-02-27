#include <iostream>
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidKernel/LibraryManager.h"

namespace Mantid
{
namespace API
{
	Kernel::Logger& AlgorithmFactoryImpl::g_log = Kernel::Logger::get("AlgorithmFactory");

	AlgorithmFactoryImpl::AlgorithmFactoryImpl() : Kernel::DynamicFactory<Algorithm>()
	{
	// we need to make sure the library manager has been loaded before we 
	// are constructed so that it is destroyed after us and thus does
	// not close any loaded DLLs with loaded algorithms in them
		Mantid::Kernel::LibraryManager::Instance();
		g_log.debug() << "Algorithm Factory created." << std::endl;
	}

	AlgorithmFactoryImpl::~AlgorithmFactoryImpl()
	{
//		g_log.debug() << "Algorithm Factory destroyed." << std::endl;
	}

} // namespace API
} // namespace Mantid
