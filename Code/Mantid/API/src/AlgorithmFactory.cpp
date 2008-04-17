#include <iostream>
#include <sstream>
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidAPI/Algorithm.h"
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

		std::string AlgorithmFactoryImpl::createName(const std::string& className, const int& version)const
		{
			std::ostringstream oss;
			oss << className << "!£$%^&*#~" << version;
			return(oss.str());
		}

		boost::shared_ptr<Algorithm> AlgorithmFactoryImpl::create(const std::string& className,const int& version) const
		{   
			int local_version=version;
			if( version < 0)
			{
				if(version == -1)//get latest version since not supplied
				{
					versionMap::const_iterator it = _vmap.find(className);
					if (!className.empty())
					{
						if(it == _vmap.end() )			  
							throw std::runtime_error("algorithm not registered "+ className );
						else
							local_version = it->second;						  
					}
					else			  
						throw;					
				}		
			}
			try
			{
				return(DynamicFactory<Algorithm>::create(createName(className,local_version)));
			}
			catch(Kernel::Exception::NotFoundError& ex)
			{
				versionMap::const_iterator it = _vmap.find(className);
				if(it == _vmap.end() )			  
					throw std::runtime_error("algorithm not registered "+ className );
				else
				{
					g_log.error()<< "algorithm "<< className<< " version " << version << " is not registered "<<std::endl;
					g_log.error()<< "the latest registered version is " << it->second<<std::endl;
					throw std::runtime_error("algorithm not registered "+ createName(className,local_version) );			
				}
			}
		}




	} // namespace API
} // namespace Mantid
