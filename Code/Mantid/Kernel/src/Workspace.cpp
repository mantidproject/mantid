#include "../inc/Workspace.h"
namespace Mantid
{

Workspace::Workspace()
{
}

Workspace::Workspace(const Workspace& w)
{
	_comment=w._comment;
}
Workspace& Workspace::operator=(const Workspace& w)
{
  _comment=w._comment;
  return *this;
}Workspace::~Workspace()
{
}
void Workspace::setTitle(const std::string& t)
{
	_title=t;
}void Workspace::setComment(const std::string& c)
{
	_comment=c;
}
std::string Workspace::getTitle() const
{
	return _title;
}
std::string Workspace::getComment() const
{
	return _comment;
}

} // Namespace Mantid
