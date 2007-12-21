#ifndef MANTID_DATAOBJECTS_WORKSPACE2D_H_
#define MANTID_DATAOBJECTS_WORKSPACE2D_H_

#include "MantidAPI/Workspace.h"
#include "MantidKernel/Logger.h"
#include "MantidDataObjects/Histogram1D.h"

namespace Mantid
{
namespace DataObjects
{
/** \class Workspace2D
    
    Concrete workspace implementation. Data is a vector of Histogram1D.
    Since Histogram1D have share ownership of X, Y or E arrays, 
    duplication is avoided for workspaces for example with identical time bins.     	
    
    \author Laurent C Chapon, ISIS, RAL
    \date 26/09/2007

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
class DLLExport Workspace2D : public API::Workspace
{
 private:
  /// A vector that holds the 1D shistograms 
  std::vector<Histogram1D> data;

public:

  /**
	Gets the name of the workspace type
	\return Standard string name
*/
  virtual const std::string id() const {return "Workspace2D";}

  Workspace2D();
  Workspace2D(const Workspace2D&);
  Workspace2D& operator=(const Workspace2D&);
  virtual ~Workspace2D();

  void setHistogramNumber(int const);

  void setX(int const, const std::vector<double>&);
  void setData(int const, const std::vector<double>&);
  void setData(int const, const std::vector<double>&, const std::vector<double>&);
  void setX(int const, const Histogram1D::RCtype&);
  void setX(int const, const Histogram1D::RCtype::ptr_type&);
  void setData(int const, const Histogram1D::RCtype&);
  void setData(int const, const Histogram1D::RCtype&, const Histogram1D::RCtype&);
  void setData(int const, const Histogram1D::RCtype::ptr_type&, const Histogram1D::RCtype::ptr_type&);
  
  /// Returns the histogram number
  const int getHistogramNumber() const;

  //section required for iteration
  virtual int size() const;       
  virtual int blocksize() const;
  virtual std::vector<double>& dataX(int const index);
  ///Returns the y data
  virtual std::vector<double>& dataY(int const index);
  ///Returns the error data
  virtual std::vector<double>& dataE(int const index);


  //Get methods return the histogram number 
  virtual const std::vector<double>& dataX(int const index) const;
  virtual const std::vector<double>& dataY(int const index) const;
  virtual const std::vector<double>& dataE(int const index) const;

  long int getMemorySize() const;

};

} // namespace DataObjects
} // Namespace Mantid 
#endif /*MANTID_DATAOBJECTS_WORKSPACE2D_H_*/
