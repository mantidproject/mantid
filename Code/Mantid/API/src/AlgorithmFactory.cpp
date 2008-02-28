#include <iostream>
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidKernel/LibraryManager.h"

namespace Mantid
{
namespace API
{

	AlgorithmFactoryImpl::AlgorithmFactoryImpl() : Kernel::DynamicFactory<Algorithm>(), g_log(Kernel::Logger::get("AlgorithmFactory"))
	{
	// we need to make sure the library manager has been loaded before we 
	// are constructed so that it is destroyed after us and thus does
	// not close any loaded DLLs with loaded algorithms in them
		Mantid::Kernel::LibraryManager::Instance();
		std::cerr << "Algorithm Factory created." << std::endl;
		g_log.debug() << "Algorithm Factory created." << std::endl;
	}

	AlgorithmFactoryImpl::~AlgorithmFactoryImpl()
	{
		std::cerr << "Algorithm Factory destroyed." << std::endl;
//		g_log.debug() << "Algorithm Factory destroyed." << std::endl;
	}

} // namespace API
} // namespace Mantid
