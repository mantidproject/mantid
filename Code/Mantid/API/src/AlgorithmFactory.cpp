//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
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
      g_log.debug() << "Algorithm Factory created." << std::endl;
    }

    AlgorithmFactoryImpl::~AlgorithmFactoryImpl()
    {
      //std::cerr << "Algorithm Factory destroyed." << std::endl;
    }

    /** Creates a mangled name for interal storage
     * @param name the name of the Algrorithm 
     * @param version the version of the algroithm 
     * @returns a mangled name string
     */
    std::string AlgorithmFactoryImpl::createName(const std::string& name, const int& version)const
    {
      std::ostringstream oss;
      oss << name << "|" << version;
      return(oss.str());
    }

    /** Creates an instance of an algorithm
     * @param name the name of the Algrorithm to create
     * @param version the version of the algroithm to create
     * @returns a shared pointer to the created algorithm
     */
    boost::shared_ptr<IAlgorithm> AlgorithmFactoryImpl::create(const std::string& name,const int& version) const
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
      catch(Kernel::Exception::NotFoundError&)
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
    const std::string AlgorithmFactoryImpl::extractAlgName(const boost::shared_ptr<IAlgorithm> alg) const
    {
      return alg->name();
    }

    /** Extract the version of an algorithm
     * @param alg the Algrorithm to use
     * @returns the version of the algroithm
     */
    const int AlgorithmFactoryImpl::extractAlgVersion(const boost::shared_ptr<IAlgorithm> alg) const
    {
      return alg->version();
    }

    /// Returns algorithm descriptors.
    std::vector<Algorithm_descriptor> AlgorithmFactoryImpl::getDescriptors() const
    {
      std::vector<std::string> sv;
      sv = getKeys();

      std::vector<Algorithm_descriptor> res;
      res.reserve(sv.size());

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
        else continue;

        boost::shared_ptr<IAlgorithm> alg = create(desc.name,desc.version);
        desc.category = alg->category();
        res.push_back(desc);
      }

      return res;
    }

    /** Adds a pointer to a Python algorithm to the vector.
     * \param pyAlg :: The Python Algorithm to add
     */
    void AlgorithmFactoryImpl::addPyAlgorithm(Algorithm* pyAlg)
    {
      pythonAlgs.push_back(pyAlg);
    }

    /** Executes the named Python algorithm that Mantid holds a pointer to.
     * \param algName :: The name of the algorithm to execute
     */
    void AlgorithmFactoryImpl::executePythonAlg(std::string algName)
    {
      std::vector<Algorithm*>::iterator iter = pythonAlgs.begin();

      for (; iter != pythonAlgs.end(); ++iter)
      {
        if ((*iter)->name() == algName)
        {
          (*iter)->initialize();
          (*iter)->execute();
          break;
        }
      }
    }

  } // namespace API
} // namespace Mantid
