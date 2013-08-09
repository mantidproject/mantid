//------------------------------------------------------
// Includes
//------------------------------------------------------
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/WorkspaceIteratorCode.h"

///\cond TEMPLATE
template MANTID_API_DLL class Mantid::API::workspace_iterator<Mantid::API::LocatedDataRef,Mantid::API::IEventWorkspace>;
template MANTID_API_DLL class Mantid::API::workspace_iterator<const Mantid::API::LocatedDataRef, const Mantid::API::IEventWorkspace>;

namespace Mantid
{

namespace API
{

/**
 */
const std::string IEventWorkspace::toString() const
{
  std::ostringstream os;
  os << MatrixWorkspace::toString() << "\n";

  os << "Events: " + boost::lexical_cast<std::string>(getNumberEvents());
  switch ( getEventType() )
  {
  case WEIGHTED:
    os << " (weighted)\n";
    break;
  case WEIGHTED_NOTIME:
    os << " (weighted, no times)\n";
    break;
  case TOF:
    os << "\n";
    break;
  }
  return os.str();
}

/**
 * @return :: A pointer to the created info node.
 */
API::Workspace::InfoNode *IEventWorkspace::createInfoNode() const
{
    auto node = MatrixWorkspace::createInfoNode();
    std::string extra("");
    switch ( getEventType() )
    {
    case WEIGHTED:
      extra = " (weighted)";
      break;
    case WEIGHTED_NOTIME:
      extra = " (weighted, no times)";
      break;
    case TOF:
      extra = "";
      break;
    }
    node->addLine( "Events: " + boost::lexical_cast<std::string>(getNumberEvents()) + extra );
    return node;
}

}

/*
 * In order to be able to cast PropertyWithValue classes correctly a definition for the PropertyWithValue<IEventWorkspace> is required
 *
 */
namespace Kernel
{

template<> MANTID_API_DLL
Mantid::API::IEventWorkspace_sptr IPropertyManager::getValue<Mantid::API::IEventWorkspace_sptr>(const std::string &name) const
{
  PropertyWithValue<Mantid::API::IEventWorkspace_sptr>* prop =
                    dynamic_cast<PropertyWithValue<Mantid::API::IEventWorkspace_sptr>*>(getPointerToProperty(name));
  if (prop)
  {
    return *prop;
  }
  else
  {
    std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected IEventWorkspace.";
    throw std::runtime_error(message);
  }
}

template<> MANTID_API_DLL
Mantid::API::IEventWorkspace_const_sptr IPropertyManager::getValue<Mantid::API::IEventWorkspace_const_sptr>(const std::string &name) const
{
  PropertyWithValue<Mantid::API::IEventWorkspace_sptr>* prop =
                    dynamic_cast<PropertyWithValue<Mantid::API::IEventWorkspace_sptr>*>(getPointerToProperty(name));
  if (prop)
  {
    return prop->operator()();
  }
  else
  {
    std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const IEventWorkspace.";
    throw std::runtime_error(message);
  }
}


}

// namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
