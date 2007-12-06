#ifndef MANTID_DATAOBJECTS_WORKSPACE1D_H_
#define MANTID_DATAOBJECTS_WORKSPACE1D_H_

#include "MantidAPI/Workspace.h"
#include "MantidKernel/Logger.h"
#include "MantidDataObjects/Histogram1D.h"

/** @class Workspace1D
    \brief Concrete workspace implementation. Data is a Histogram1D      	
    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007 	
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
namespace Mantid
{
namespace DataObjects
{

class DLLExport Workspace1D : public API::Workspace, 
    public Histogram1D
{

public:

/**
	Gets the name of the workspace type
	\return Standard string name
*/
  virtual const std::string id() const {return "Workspace1D";}

  Workspace1D();
  virtual ~Workspace1D();

protected:

  Workspace1D(const Workspace1D&);
  Workspace1D& operator=(const Workspace1D&);

};

} // namespace DataObjects

} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_WORKSPACE1D_H_*/
