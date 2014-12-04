#ifndef MANTID_DATAOBJECTS_HISTOGRAM1D_H_
#define MANTID_DATAOBJECTS_HISTOGRAM1D_H_

#include "MantidAPI/ISpectrum.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace DataObjects
{
/**
  1D histogram implementation.

  Copyright &copy; 2007-2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  
  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport Histogram1D : public Mantid::API::ISpectrum
{
protected:
  MantidVecPtr refY;   ///< RefCounted Y
  MantidVecPtr refE;   ///< RefCounted Error

public:
  Histogram1D();
  Histogram1D(const Histogram1D&);
  Histogram1D& operator=(const Histogram1D&);
  virtual ~Histogram1D();

  /// Sets the data.
  void setData(const MantidVec& Y) {  refY.access()=Y; };
  /// Sets the data and errors
  void setData(const MantidVec& Y, const MantidVec& E) 
  {  refY.access()=Y; refE.access()=E; }

  /// Sets the data.
  void setData(const MantidVecPtr& Y) { refY=Y; }
  /// Sets the data and errors
  void setData(const MantidVecPtr& Y, const MantidVecPtr& E) { refY=Y; refE=E;}
  
  /// Sets the data.
  void setData(const MantidVecPtr::ptr_type& Y) { refY=Y; }
  /// Sets the data and errors
  void setData(const MantidVecPtr::ptr_type& Y, const MantidVecPtr::ptr_type& E) { refY=Y; refE=E;}

  /// Zero the data (Y&E) in this spectrum
  void clearData();

  // Get the array data
  /// Returns the y data const
  virtual const MantidVec& dataY() const { return *refY; }
  /// Returns the error data const
  virtual const MantidVec& dataE() const { return *refE; }

  ///Returns the y data
  virtual MantidVec& dataY() { return refY.access(); }
  ///Returns the error data
  virtual MantidVec& dataE() { return refE.access(); }

  virtual std::size_t size() const { return refY->size(); }          ///< get pseudo size

  /// Checks for errors
  bool isError() const { return refE->empty(); }

  /// Gets the memory size of the histogram
  size_t getMemorySize() const 
  { return ((refX->size()+refY->size()+refE->size())*sizeof(double)); }
};

} // namespace DataObjects
}  //Namespace Mantid
#endif /*MANTID_DATAOBJECTS_HISTOGRAM1D_H_*/
