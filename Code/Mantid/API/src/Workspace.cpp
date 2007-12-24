#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/TripleRef.h"
#include "MantidAPI/TripleIterator.h"
#include "MantidAPI/TripleIteratorCode.h"

namespace Mantid
{
namespace API
{

Kernel::Logger& Workspace::g_log = Kernel::Logger::get("Workspace");

/// Default constructor
Workspace::Workspace()
{}

/// Copy constructor
Workspace::Workspace(const Workspace& A)  :
     _title(A._title),_comment(A._comment)
{}

/// Assignment operator
Workspace& Workspace::operator=(const Workspace& A)
{
  if (this!=&A)
  {
    _comment=A._comment;
    _title=A._title;
  }	       
  return *this;
}

/// Destructor// RJT, 3/10/07: The Analysis Data Service needs to be able to delete workspaces, so I moved this from protected to public.
Workspace::~Workspace()
{}
	
/** Set the title of the workspace
 * 
 *  @param t The title
 */
void Workspace::setTitle(const std::string& t)
{
     _title=t;
}
/** Set the comment field of the workspace
 * 
 *  @param c The comment
 */
void Workspace::setComment(const std::string& c)
{
	_comment=c;
}

/** Get the workspace title
 * 
 *  @return The title
 */
const std::string& Workspace::getTitle() const
{
	return _title;
}

/** Get the workspace comment
 * 
 *  @return The comment
 */
const std::string& Workspace::getComment() const
{
    return _comment;
}

/** Get the Instrument associated with this workspace
 * 
 *  @return The instrument class
 */
Instrument& Workspace::getInstrument() 
{
	return _instrument;
}

/** Get the sample associated with this workspace
 * 
 *  @return The sample class
 */
Sample& Workspace::getSample()
{
  return _sample;
}

} // namespace API
} // Namespace Mantid


///\cond TEMPLATE
template DLLExport class Mantid::API::triple_iterator<Mantid::API::Workspace>;

template DLLExport class Mantid::API::WorkspaceProperty<Mantid::API::Workspace>;

namespace Mantid
{
namespace Kernel
{
template<> DLLExport
Mantid::API::Workspace* PropertyManager::getValue<Mantid::API::Workspace*>(const std::string &name) const
{
  PropertyWithValue<Mantid::API::Workspace*> *prop = 
                    dynamic_cast<PropertyWithValue<Mantid::API::Workspace*>*>(getPointerToProperty(name));
  if (prop)
  {
    return *prop;
  }
  else
  {
    throw std::runtime_error("Attempt to assign property of incorrect type");
  }
  
}
template<> DLLExport
Mantid::API::Workspace_sptr PropertyManager::getValue<Mantid::API::Workspace_sptr>(const std::string &name) const
{
  PropertyWithValue<Mantid::API::Workspace_sptr>* prop = 
                    dynamic_cast<PropertyWithValue<Mantid::API::Workspace_sptr>*>(getPointerToProperty(name));
  if (prop)
  {
    return *prop;
  }
  else
  {
    throw std::runtime_error("Attempt to assign property of incorrect type");
  }
}
} // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
