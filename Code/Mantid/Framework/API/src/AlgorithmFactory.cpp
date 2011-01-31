//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <iostream>
#include <sstream>
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidAPI/CloneableAlgorithm.h"

namespace Mantid
{
  namespace API
  {

    AlgorithmFactoryImpl::AlgorithmFactoryImpl() : 
      Kernel::DynamicFactory<Algorithm>(), g_log(Kernel::Logger::get("AlgorithmFactory")),
      m_vmap(), m_cloneable_algs()
    {
      // we need to make sure the library manager has been loaded before we 
      // are constructed so that it is destroyed after us and thus does
      // not close any loaded DLLs with loaded algorithms in them
      Mantid::Kernel::LibraryManager::Instance();
      g_log.debug() << "Algorithm Factory created." << std::endl;
    }

    AlgorithmFactoryImpl::~AlgorithmFactoryImpl()
    {
    }

    /** Creates a mangled name for interal storage
     * @param name :: the name of the Algrorithm 
     * @param version :: the version of the algroithm 
     * @returns a mangled name string
     */
    std::string AlgorithmFactoryImpl::createName(const std::string& name, const int& version)const
    {
      std::ostringstream oss;
      oss << name << "|" << version;
      return(oss.str());
    }

    /** Creates an instance of an algorithm
     * @param name :: the name of the Algrorithm to create
     * @param version :: the version of the algroithm to create
     * @returns a shared pointer to the created algorithm
     */
    boost::shared_ptr<Algorithm> AlgorithmFactoryImpl::create(const std::string& name,const int& version) const
    {   
      int local_version=version;
      if( version < 0)
      {
        if(version == -1)//get latest version since not supplied
        {
          versionMap::const_iterator it = m_vmap.find(name);
          if (!name.empty())
          {
            if(it == m_vmap.end() )
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
        return this->createAlgorithm(name, local_version);
      }
      catch(Kernel::Exception::NotFoundError&)
      {
        versionMap::const_iterator it = m_vmap.find(name);
        if(it == m_vmap.end() )
          throw std::runtime_error("algorithm not registered "+ name );
        else
        {
          g_log.error()<< "algorithm "<< name<< " version " << version << " is not registered "<<std::endl;
          g_log.error()<< "the latest registered version is " << it->second<<std::endl;
          throw std::runtime_error("algorithm not registered "+ createName(name,local_version) );
        }
      }
    }

    /**
     * Return the keys used for identifying algorithms. This includes those within the Factory itself and 
     * any cleanly constructed algorithms stored here.
     * @returns The strings used to identify individual algorithms
     */
    const std::vector<std::string> AlgorithmFactoryImpl::getKeys() const
    {
      //Start with those subscribed with the factory and add the cleanly constructed algorithm keys
      std::vector<std::string> names = Kernel::DynamicFactory<Algorithm>::getKeys();
      names.reserve(names.size() + m_cloneable_algs.size());

      std::map<std::string, API::CloneableAlgorithm*>::const_iterator itr_end = m_cloneable_algs.end();
      for(std::map<std::string, API::CloneableAlgorithm*>::const_iterator itr = m_cloneable_algs.begin(); 
	  itr!= itr_end; ++itr )
      {
	names.push_back(itr->first);
      }
      return names;
    }

    /**
     * Get a list of descriptor objects used to order the algorithms in the stored map 
     * @returns A vector of descriptor objects
     */
    std::vector<Algorithm_descriptor> AlgorithmFactoryImpl::getDescriptors() const
    {
      std::vector<std::string> sv;
      sv = getKeys();

      std::vector<Algorithm_descriptor> res;

      for(std::vector<std::string>::const_iterator s=sv.begin();s!=sv.end();s++)
      {
	if (s->empty()) continue;
	Algorithm_descriptor desc;
	size_t i = s->find('|');
	if (i == std::string::npos) 
	{
	  desc.name = *s;
	  desc.version = 1;
	}
	else if (i > 0) 
	{
	  desc.name = s->substr(0,i);
	  std::string vers = s->substr(i+1);
	  desc.version = vers.empty()? 1 : atoi(vers.c_str());
	}
	else
	  continue;
	boost::shared_ptr<IAlgorithm> alg = create(desc.name,desc.version);
	desc.category = alg->category();
	res.push_back(desc);
      }
      return res;
    }

    /**
     * Store a pointer to an Algorithm object that is cloneable, e.g. a Python algorithm
     * @param algorithm :: A pointer to a clonable algorithm object
     */
    bool AlgorithmFactoryImpl::storeCloneableAlgorithm(CloneableAlgorithm* algorithm)
    {
      const std::string alg_name = algorithm->name();
      if( alg_name.empty() )
      {
	throw std::runtime_error("Cannot register algorithm with empty name.");
      }
      const int alg_version(algorithm->version());
      //Check if we already have an algorithm of this name and version within the factory
      std::map<std::string, int>::const_iterator itr = m_vmap.find(alg_name);
      if( itr != m_vmap.end() )
      {
	int registered_version = itr->second;
	if( alg_version > registered_version )
	{
	  m_vmap[alg_name] = alg_version;
	}
      }
      else
      {
	m_vmap[alg_name] = alg_version;
      }

      // Finally check that the algorithm can be initialized without throwing
      try
      {
	algorithm->clone()->initialize();
      }
      catch( std::runtime_error & e )
      {
	g_log.error(e.what());
	return false;
      }
      catch( ... )
      {
	return false;
      }
      
      //Insert into map, overwriting if necessary
      m_cloneable_algs[createName(alg_name, alg_version)] = algorithm;
      
      //Notify whomever is interested that the factory has been updated
      notificationCenter.postNotification(new AlgorithmFactoryUpdateNotification);
      
      return true;
    }

    /** Extract the name of an algorithm
     * @param alg :: the Algrorithm to use
     * @returns the name of the algroithm
     */
    const std::string AlgorithmFactoryImpl::extractAlgName(const boost::shared_ptr<IAlgorithm> alg) const
    {
      return alg->name();
    }
    
    /** Extract the version of an algorithm
     * @param alg :: the Algrorithm to use
     * @returns the version of the algroithm
     */
    int AlgorithmFactoryImpl::extractAlgVersion(const boost::shared_ptr<IAlgorithm> alg) const
    {
      return alg->version();
    }

    /**
     * Create a shared pointer to an algorithm object with the given name and version. If the algorithm is one registered with a clean pointer rather than
     * an instantiator then a clone is returned.
     * @param name :: Algorithm name
     * @param version :: Algorithm version
     * @returns A shared pointer to the algorithm object
     */
    boost::shared_ptr<Algorithm> AlgorithmFactoryImpl::createAlgorithm(const std::string & name, const int version) const
    {
      try
      {
        return Kernel::DynamicFactory<Algorithm>::create(createName(name,version));
      }
      catch(Mantid::Kernel::Exception::NotFoundError &)
      {
      }
      //See if we can make one from the clean cache
      const std::string fqlname(createName(name, version));
      std::map<std::string, CloneableAlgorithm*>::const_iterator itr = m_cloneable_algs.find(fqlname);
      if( itr != m_cloneable_algs.end() )
      {
        API::CloneableAlgorithm *cloned = itr->second->clone();
        if( !cloned )
        {
          throw std::runtime_error("Cloning algorithm failed, cannot create algorithm \"" + name + "\"");
        }
        return boost::shared_ptr<API::CloneableAlgorithm>(cloned, std::mem_fun(&CloneableAlgorithm::kill));
      }
      else
      {
        throw Mantid::Kernel::Exception::NotFoundError("Unknown algorithm requested.", name);
      }
    }

  } // namespace API
} // namespace Mantid
