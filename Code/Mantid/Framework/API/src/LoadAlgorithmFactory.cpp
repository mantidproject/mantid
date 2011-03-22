#include "MantidAPI/LoadAlgorithmFactory.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
  namespace API
  {
    ///Constructor
    LoadAlgorithmFactoryImpl::LoadAlgorithmFactoryImpl():
    Kernel::DynamicFactory<IDataFileChecker>(),
    m_log(Kernel::Logger::get("LoadAlgorithmFactoryImpl"))
    {
    }
    ///Destructor
    LoadAlgorithmFactoryImpl::~LoadAlgorithmFactoryImpl()
    {      
    }
    /** Returns an instance of the class with the given name. Overrides the base class method.
    *  If an instance already exists, a pointer to it is returned, otherwise
    *  a new instance is created by the DynamicFactory::create method.
    *  @param className :: The name of the class to be created
    *  @return A shared pointer to the instance of the requested unit
    */
    boost::shared_ptr<IDataFileChecker> LoadAlgorithmFactoryImpl::create(const std::string& className) const 
    {      
        return  Kernel::DynamicFactory<IDataFileChecker>::create(className);
    }

  }
}
