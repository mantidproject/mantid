#ifndef MANTID_DATAOBJECTS_HISTOGRAM1D_H_
#define MANTID_DATAOBJECTS_HISTOGRAM1D_H_

#include <boost/shared_ptr.hpp>
#include <vector>
#include "MantidKernel/System.h"
#include "MantidKernel/cow_ptr.h"

using namespace Mantid;

namespace Mantid
{
namespace DataObjects
{
/**
	1D histogram implementation.

    \class Histogram1D Histogram1D.h
    \author Laurent C Chapon, ISIS, RAL
    \date 26/09/2007  
    
  Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  
  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport Histogram1D
{
public:

  /// The data storage type used internally in a Histogram1D
  // typedef std::vector<double> MantidVec; //Removed redundant typedef
  /// Data Store: NOTE:: CHANGED TO BREAK THE WRONG USEAGE OF SHARED_PTR 
  //typedef Kernel::cow_ptr<MantidVec > MantidVecPtr;
  
private:
  MantidVecPtr refX;   ///< RefCounted X
  MantidVecPtr refY;   ///< RefCounted Y
  MantidVecPtr refE;   ///< RefCounted Error

public:
  Histogram1D();
  Histogram1D(const Histogram1D&);
  Histogram1D& operator=(const Histogram1D&);
  virtual ~Histogram1D();

  /// Sets the x data.
  void setX(const MantidVec& X) {  refX.access()=X; }
  /// Sets the data.
  void setData(const MantidVec& Y) {  refY.access()=Y; };
  /// Sets the data and errors
  void setData(const MantidVec& Y, const MantidVec& E) 
    {  refY.access()=Y; refE.access()=E; }

  /// Sets the x data.
  void setX(const MantidVecPtr& X) { refX=X; }
  /// Sets the data.
  void setData(const MantidVecPtr& Y) { refY=Y; }
  /// Sets the data and errors
  void setData(const MantidVecPtr& Y, const MantidVecPtr& E) { refY=Y; refE=E;}
  
  /// Sets the x data
  void setX(const MantidVecPtr::ptr_type& X) { refX=X; }
  /// Sets the data.
  void setData(const MantidVecPtr::ptr_type& Y) { refY=Y; }
  /// Sets the data and errors
  void setData(const MantidVecPtr::ptr_type& Y, const MantidVecPtr::ptr_type& E) { refY=Y; refE=E;}

  // Get the array data
  /// Returns the x data const
  virtual const MantidVec& dataX() const { return *refX; }  
  /// Returns the y data const
  virtual const MantidVec& dataY() const { return *refY; }
  /// Returns the error data const
  virtual const MantidVec& dataE() const { return *refE; }

  ///Returns the x data
  virtual MantidVec& dataX() { return refX.access(); }
  ///Returns the y data
  virtual MantidVec& dataY() { return refY.access(); }
  ///Returns the error data
  virtual MantidVec& dataE() { return refE.access(); }

  /// Returns a pointer to the x data
  virtual MantidVecPtr ptrX() const { return refX; }  
  
  ///Clear the x data
  MantidVec& emptyX() { refX.access().clear(); return refX.access(); }
  ///Clear the y data
  MantidVec& emptyY() { refY.access().clear(); return refY.access(); }
  ///Clear the error data
  MantidVec& emptyE() { refE.access().clear(); return refE.access(); }

  int nxbin() const { return static_cast<int>(refX->size()); }         ///< Return the number of X bins
  int nybin() const { return static_cast<int>(refY->size()); }         ///< Return the number of data bin (Y or YE)
  virtual int size() const { return static_cast<int>(refY->size()); }          ///< get pseudo size

  /// Checks for errors
  bool isError() const { return refE->empty(); }
  /// Gets the memory size of the histogram
  long int getMemorySize() const 
    { return static_cast<long int>((refX->size()+refY->size()+refE->size())*sizeof(double)); }
};

} // namespace DataObjects
}  //Namespace Mantid
#endif /*MANTID_DATAOBJECTS_HISTOGRAM1D_H_*/
