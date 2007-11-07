#include "../inc/Workspace.h"

namespace Mantid
{
namespace Kernel
{

Logger& Workspace::g_log = Logger::get("Workspace");

Workspace::Workspace()
{}

Workspace::Workspace(const Workspace& A)  :
     _title(A._title),_comment(A._comment)
{}
	
Workspace& 
Workspace::operator=(const Workspace& A)
{
     if (this!=&A)
       {
           _comment=A._comment;
	    _title=A._title;
       }	       
  return *this;
}
Workspace::~Workspace()
{}
	
void Workspace::setTitle(const std::string& t)
{
     _title=t;
}void Workspace::setComment(const std::string& c)
{
	_comment=c;
}
const std::string& 
Workspace::getTitle() const
{
	return _title;
}

const std::string& 
Workspace::getComment() const
{
    return _comment;
}

} // namespace Kernel
} // Namespace Mantid
