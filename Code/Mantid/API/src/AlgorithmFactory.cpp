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
		}

    /** Creates a mangled name for interal storage
    * @param name the name of the Algrorithm 
    * @param version the version of the algroithm 
    * @returns a mangled name string
    */
		std::string AlgorithmFactoryImpl::createName(const std::string& name, const int& version)const
		{
			std::ostringstream oss;
			oss << name << "¦" << version;
			return(oss.str());
		}

    /** Creates an instance of an algorithm
    * @param name the name of the Algrorithm to create
    * @param version the version of the algroithm to create
    * @returns a shared pointer to the created algorithm
    */
		boost::shared_ptr<Algorithm> AlgorithmFactoryImpl::create(const std::string& name,const int& version) const
		{   
			int local_version=version;
			if( version < 0)
			{
				if(version == -1)//get latest version since not supplied
				{
					versionMap::const_iterator it = _vmap.find(name);
					if (!name.empty())
					{
						if(it == _vmap.end() )			  
							throw std::runtime_error("algorithm not registered "+ name );
						else
							local_version = it->second;						  
					}
					else			  
						throw;					
				}		
			}
			try
			{
        return(Kernel::DynamicFactory<Algorithm>::create(createName(name,local_version)));
			}
			catch(Kernel::Exception::NotFoundError& ex)
			{
				versionMap::const_iterator it = _vmap.find(name);
				if(it == _vmap.end() )			  
					throw std::runtime_error("algorithm not registered "+ name );
				else
				{
					g_log.error()<< "algorithm "<< name<< " version " << version << " is not registered "<<std::endl;
					g_log.error()<< "the latest registered version is " << it->second<<std::endl;
					throw std::runtime_error("algorithm not registered "+ createName(name,local_version) );			
				}
			}
		}

    /** Extract the name of an algorithm
    * @param alg the Algrorithm to use
    * @returns the name of the algroithm
    */
    const std::string AlgorithmFactoryImpl::extractAlgName(const boost::shared_ptr<Algorithm> alg) const
    {
      return alg->name();
    }

    /** Extract the version of an algorithm
    * @param alg the Algrorithm to use
    * @returns the version of the algroithm
    */
    const int AlgorithmFactoryImpl::extractAlgVersion(const boost::shared_ptr<Algorithm> alg) const
    {
      return alg->version();
    }


	} // namespace API
} // namespace Mantid
