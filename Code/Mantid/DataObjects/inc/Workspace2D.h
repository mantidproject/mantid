#ifndef MANTID_DATAOBJECTS_WORKSPACE2D_H_
#define MANTID_DATAOBJECTS_WORKSPACE2D_H_

#include "Workspace.h"
#include "MantidKernel/Logger.h"
#include "Histogram1D.h"

/** @class Workspace2D Workspace2D.h
 	
 			Workspace2D
 			Concrete workspace implementation. Data is a vector of Histogram1D.
 			Since Histogram1D have share ownership of X, Y or E arrays, 
 			duplication is avoided for workspaces for example with identical time bins.     	
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

class DLLExport Workspace2D: public Kernel::Workspace
{
public:
	const std::string id() const {return "Workspace2D";}
  Workspace2D();
//	static Workspace* create()
//	{
//		return new Workspace2D;
//	}
	// Set the number of Histogram1D
	// This will resize the number of Histogram1D
	void setHistogramNumber(int);
	//Set X array with data vector
	void setX(int, const std::vector<double>&);
	//Set Y array with data vector, no associated error array
	void setData(int, const std::vector<double>&);
	//Set Y,E arrays with data vectors
	void setData(int, const std::vector<double>&, const std::vector<double>&);
	//Set X array with shared_ptr<vector>
	void setX(int, const Histogram1D::parray&);
	//Set Y array with shared_ptr<vector<double> >, no associated errors
	void setData(int, const Histogram1D::parray&);
	//Set (Y,E) arrays with shared_ptr<vector<double> >
	void setData(int, const Histogram1D::parray&, const Histogram1D::parray&);
	/// Retrieve the number of Histogram1D's in the workspace
	const int getHistogramNumber() const;
	//Get methods return the histogram number 
	const std::vector<double>& getX(int) const;
	const std::vector<double>& getY(int) const;
	const std::vector<double>& getE(int) const;
	// Get the Memory footprint of the Workspace. If the data in the Histograms are shared, should
	// return the fractional memory=real memory divides by number of use_count of shared_ptr. At present 
	// just return the real memory without taking into account shared ownership. Needs modifying.
  long int getMemorySize() const;
protected:
	Workspace2D(const Workspace2D&);
	Workspace2D& operator=(const Workspace2D&);
	virtual ~Workspace2D();
private:
	std::vector<Histogram1D> _data;
	int _nhistogram;
};

} // namespace DataObjects
} // Namespace Mantid 
#endif /*MANTID_DATAOBJECTS_WORKSPACE2D_H_*/
