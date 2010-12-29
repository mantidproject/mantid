#include "MantidAPI/Workspace.h"
#include "MantidKernel/PropertyManager.h"

namespace Mantid
{
namespace API
{

/// Default constructor
  Workspace::Workspace() : m_title(), m_comment(), m_name(), m_history()
{}

/** Set the title of the workspace
 *
 *  @param t The title
 */
void Workspace::setTitle(const std::string& t)
{
  m_title=t;
}

/** Set the comment field of the workspace
 *
 *  @param c The comment
 */
void Workspace::setComment(const std::string& c)
{
  m_comment=c;
}

/** Set the name field of the workspace
 *
 *  @param name The name
 */
void Workspace::setName(const std::string& name)
{
  m_name = name;
}

/** Get the workspace title
 *
 *  @return The title
 */
const std::string& Workspace::getTitle() const
{
  return m_title;
}

/** Get the workspace comment
 *
 *  @return The comment
 */
const std::string& Workspace::getComment() const
{
  return m_comment;
}

/** Get the workspace name
 *
 *  @return The name
 */
const std::string& Workspace::getName() const
{
  return m_name;
}


} // namespace API
} // Namespace Mantid

///\cond TEMPLATE
namespace Mantid
{
namespace Kernel
{

template<> DLLExport
Mantid::API::Workspace_sptr IPropertyManager::getValue<Mantid::API::Workspace_sptr>(const std::string &name) const
{
  PropertyWithValue<Mantid::API::Workspace_sptr>* prop =
                    dynamic_cast<PropertyWithValue<Mantid::API::Workspace_sptr>*>(getPointerToProperty(name));
  if (prop)
  {
    return *prop;
  }
  else
  {
    std::string message = "Attempt to assign property "+ name +" to incorrect type";
    throw std::runtime_error(message);
  }
}

template<> DLLExport
Mantid::API::Workspace_const_sptr IPropertyManager::getValue<Mantid::API::Workspace_const_sptr>(const std::string &name) const
{
  PropertyWithValue<Mantid::API::Workspace_sptr>* prop =
                    dynamic_cast<PropertyWithValue<Mantid::API::Workspace_sptr>*>(getPointerToProperty(name));
  if (prop)
  {
    return prop->operator()();
  }
  else
  {
    std::string message = "Attempt to assign property "+ name +" to incorrect type";
    throw std::runtime_error(message);
  }
}

} // namespace Kernel
} // namespace Mantid
///\endcond TEMPLATE

