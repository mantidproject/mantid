/*
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

#ifndef MANTID_DATAOBJECTS_HISTOGRAM1D_H_
#define MANTID_DATAOBJECTS_HISTOGRAM1D_H_

#include <boost/shared_ptr.hpp>
#include <vector>
#include "MantidKernel/RefControl.h"

namespace Mantid
{
namespace DataObjects
{
/*!
 
    @class Histogram1D Histogram1D.h
    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007
*/

class Histogram1D
{
public:

  /// Data Store: NOTE:: CHANGED TO BREAK THE WRONG USEAGE OF SHARED_PTR 

  typedef RefControl<std::vector<double> > RCtype;    
  
 private:

  RCtype refX;   ///< RefCounted X
  RCtype refY;   ///< RefCounted Y
  RCtype refE;

public:
  /// Data Store: NOTE:: CHANGED TO BREAK THE WRONG USEAGE OF SHARED_PTR 

  Histogram1D();
  Histogram1D(const Histogram1D&);
  Histogram1D& operator=(const Histogram1D&);
  virtual ~Histogram1D();


  void setX(const std::vector<double>& X) {  refX.access()=X; }
  void setData(const std::vector<double>& Y) {  refY.access()=Y; };
  void setData(const std::vector<double>& Y,const std::vector<double> E) 
    {  refY.access()=Y; refE.access()=E; }

  void setX(const RCtype& X) { refX=X; }
  void setData(const RCtype& Y) { refY=Y; }
  void setData(const RCtype& Y, const RCtype& E) { refY=Y; refE=E;}

  void setX(const RCtype::ptr_type& X) { refX=X; }
  void setData(const RCtype::ptr_type& Y) { refY=Y; }
  void setData(const RCtype::ptr_type& Y, const RCtype::ptr_type& E) { refY=Y; refE=E;}

  // Get the array data
  const std::vector<double>& dataX() const { return *refX; }  
  const std::vector<double>& dataY() const { return *refY; }
  const std::vector<double>& dataE() const { return *refE; }

  std::vector<double>& dataX() { return refX.access(); }
  std::vector<double>& dataY() { return refY.access(); }
  std::vector<double>& dataE() { return refE.access(); }

  //  Zero the Array
  std::vector<double>& emptyX() { refX.access().clear(); return refX.access(); }
  std::vector<double>& emptyY() { refY.access().clear(); return refY.access(); }
  std::vector<double>& emptyE() { refE.access().clear(); return refE.access(); }

  int nxbin() const { return refX->size(); }         ///< Return the number of X bins
  int nybin() const { return refY->size(); }   ///< Return the number of data bin (Y or YE)


  // Return flag if data has associated errors
  bool isError() const { return refE->empty(); }
  long int getMemorySize() const { return (refX->size()+refY->size()+refE->size())*sizeof(double); }

};

} // namespace DataObjects

}  //Namespace Mantid
#endif /*MANTID_DATAOBJECTS_HISTOGRAM1D_H_*/
