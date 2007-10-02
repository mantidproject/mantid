#ifndef HISTOGRAM1D_H_
#define HISTOGRAM1D_H_
#include <boost/shared_ptr.hpp>
#include <vector>
namespace Mantid
{
/** @class Histogram1D Histogram1D.h
7 	
8 	     	
10 	    @author Laurent C Chapon, ISIS, RAL
12 	    @date 26/09/2007
13 	    
14 	    Copyright © 2007 ???RAL???
15 	
16 	    This file is part of Mantid.
17 	
18 	    Mantid is free software; you can redistribute it and/or modify
19 	    it under the terms of the GNU General Public License as published by
20 	    the Free Software Foundation; either version 3 of the License, or
21 	    (at your option) any later version.
22 	
23 	    Mantid is distributed in the hope that it will be useful,
24 	    but WITHOUT ANY WARRANTY; without even the implied warranty of
25 	    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
26 	    GNU General Public License for more details.
27 	
28 	    You should have received a copy of the GNU General Public License
29 	    along with this program.  If not, see <http://www.gnu.org/licenses/>.
30 	
31 	    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
32 	*/
class Histogram1D
{
public:
	//
	typedef boost::shared_ptr<std::vector<double> > parray;
	// Standard constructors/destructor
	Histogram1D();
	Histogram1D(const Histogram1D&);
	Histogram1D& operator=(const Histogram1D&);
  virtual ~Histogram1D();
  // Set X array with a vector. Shared ptr will be reseted
	void setX(const std::vector<double>&);
  // Set X array with a shared_ptr<vector>. 
  void setX(const parray&);
  // Copy the Xarray from another Histogram1D
  void copyX(const Histogram1D&);
  // Set data  (Y) with a vector
  void setData(const std::vector<double>&);
  // Set data (Y,E) with two vectors
	void setData(const std::vector<double>&, const std::vector<double>&);
  // Set data (Y) with a shared_ptr<vector>
  void setData(const parray&);
  // Set data (Y,E) with two shared_ptr<vector> 
  void setData(const parray&, const parray&); 
	// Get the X array 
  const std::vector<double>& getX() const; 
	// Get the Y array
	const std::vector<double>& getY() const;
	// Get the E array	
  const std::vector<double>& getE() const;
	// Get the ith element of the X array
  double getX(size_t) const;
  // Get the ith element of the Y array
	double getY(size_t) const;
  // Get the ith element of the E array
	double getE(size_t) const;
  // Get a pointer to the ith element triplet (X,Y,E)
	double* operator[](size_t) const;
  // Return the number of X bin
	int nxbin() const;
  // Return the number of data bin (Y or YE)
	int nybin() const;
  // Return flag if data has associated errors
	bool isError() const;
private:
	int _nxbin; // Number of xbins=number of ybins+1	
	int _nybin; // Number of data bins.
	parray _X;
	parray _Y;
	parray _E;
};

}  //Namespace Mantid
#endif /*Histogram1D_H_*/
