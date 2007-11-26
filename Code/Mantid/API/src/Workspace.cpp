#include "MantidAPI/Workspace.h"

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


Instrument& Workspace::getInstrument() 
{
	return _instrument;
}

} // namespace API
} // Namespace Mantid
