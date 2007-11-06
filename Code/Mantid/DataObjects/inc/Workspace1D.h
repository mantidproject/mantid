#ifndef MANTID_DATAOBJECTS_WORKSPACE1D_H_
#define MANTID_DATAOBJECTS_WORKSPACE1D_H_

#include "../../Kernel/inc/Workspace.h"
#include "../../Kernel/inc/Logger.h"
#include "Histogram1D.h"

/** @class Workspace1D Workspace1D.h
 	
 			Workspace1D
 			Concrete workspace implementation. Data is a Histogram1D      	
 	    @author Laurent C Chapon, ISIS, RAL
 	    @date 26/09/2007
 	    
 	    Copyright © 2007 ???RAL???
 	
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

class DLLExport Workspace1D: public Kernel::Workspace
{
public:
	const std::string id() const {return "Workspace1D";}
  Workspace1D();
//	static Workspace* create()
//	{
//		return new Workspace1D;
//	}
	//Set X array with data vector
	void setX(const std::vector<double>&);
	//Set Y array with data vector, no associated error array
	void setData(const std::vector<double>&);
	//Set Y,E arrays with data vectors
	void setData(const std::vector<double>&, const std::vector<double>&);
	//Set X array with shared_ptr<vector>
	void setX(const Histogram1D::parray&);
	//Set Y array with shared_ptr<vector<double> >, no associated errors
	void setData(const Histogram1D::parray&);
	//Set (Y,E) arrays with shared_ptr<vector<double> >
	void setData(const Histogram1D::parray&, const Histogram1D::parray&);
	//Get methods
	const std::vector<double>& getX() const;
	const std::vector<double>& getY() const;
	const std::vector<double>& getE() const;
	// Get the Memory footprint of the Workspace. If the data in the Histograms are shared, should
	// return the fractional memory=real memory divides by number of use_count of shared_ptr. At present 
	// just return the real memory without taking into account shared ownership. Needs modifying.
  long int getMemorySize() const;
protected:
	Workspace1D(const Workspace1D&);
	Workspace1D& operator=(const Workspace1D&);
	virtual ~Workspace1D();
private:
	Histogram1D _data;
};

} // namespace DataObjects
} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_WORKSPACE1D_H_*/
