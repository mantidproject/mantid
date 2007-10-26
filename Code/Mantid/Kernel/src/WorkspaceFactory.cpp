/*  The WorkspaceFactory class is in charge of the creation of all types
    of workspaces. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.
    
    @author Laurent C Chapon, ISIS, RAL
    @author Russell Taylor, Tessella Support Services plc
    @date 26/09/2007
    
    Copyright &copy; 2007 ???RAL???

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
*/

#include "../inc/WorkspaceFactory.h"

namespace Mantid
{

Logger& WorkspaceFactory::g_log = Logger::get("WorkspaceFactory");

// Initialise the instance pointer to zero
WorkspaceFactory* WorkspaceFactory::m_instance = 0;

WorkspaceFactory::WorkspaceFactory()
{
}

WorkspaceFactory::~WorkspaceFactory()
{
	delete m_instance;
}

WorkspaceFactory* WorkspaceFactory::Instance()
{
	if (!m_instance) m_instance=new WorkspaceFactory;
	return m_instance;
}

} // Namespace Mantid
