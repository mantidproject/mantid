#include <iostream>
#include <iomanip>
#include <map>
#include <stdexcept>

#include "System.h"
#include "Instantiator.h"
#include "Logger.h"
#include "DynamicFactory.h"
#include "Workspace.h"
#include "IAlgorithm.h"


namespace Mantid 
{
namespace Kernel
{

template<typename Base> 
DynamicFactory<Base>::~DynamicFactory()
{
 std::cout<<"Calling DFactory "<<
     std::setbase(16)<<reinterpret_cast<long>(this)<<std::endl;
 for (typename FactoryMap::iterator it=_map.begin();
           it!=_map.end(); it++)
    {
      delete it->second;
    }
}

template class DynamicFactory<Mantid::Kernel::IAlgorithm>;
template class DynamicFactory<Mantid::Kernel::Workspace>;
template class DynamicFactory<int>;

} // NAMESPACE Kernel
} // NAMESPACE Mantid  
