/*
  Copyright 2007
  
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

#ifndef MANTID_DATAOBJECTS_WORKSPACE2D_H_
#define MANTID_DATAOBJECTS_WORKSPACE2D_H_

#include "MantidAPI/Workspace.h"
#include "MantidKernel/Logger.h"
#include "MantidDataObjects/Histogram1D.h"

/** @class Workspace2D
    
    Concrete workspace implementation. Data is a vector of Histogram1D.
    Since Histogram1D have share ownership of X, Y or E arrays, 
    duplication is avoided for workspaces for example with identical time bins.     	
    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007
    
*/ 	
namespace Mantid
{
namespace DataObjects
{

class DLLExport Workspace2D : public API::Workspace
{
 private:

  std::vector<Histogram1D> data;

public:

  virtual const std::string id() const {return "Workspace2D";}

  Workspace2D();
  Workspace2D(const Workspace2D&);
  Workspace2D& operator=(const Workspace2D&);
  virtual ~Workspace2D();

  void setHistogramNumber(const int);

  void setX(const int, const std::vector<double>&);
  void setData(const int, const std::vector<double>&);
  void setData(const int, const std::vector<double>&, const std::vector<double>&);
  void setX(const int, const Histogram1D::RCtype&);
  void setX(const int,const Histogram1D::RCtype::ptr_type&);
  void setData(const int, const Histogram1D::RCtype&);
  void setData(const int, const Histogram1D::RCtype&, const Histogram1D::RCtype&);
  const int getHistogramNumber() const;

  //Get methods return the histogram number 
  const std::vector<double>& getX(const int) const;
  const std::vector<double>& getY(const int) const;
  const std::vector<double>& getE(const int) const;

  long int getMemorySize() const;

};

} // namespace DataObjects
} // Namespace Mantid 
#endif /*MANTID_DATAOBJECTS_WORKSPACE2D_H_*/
