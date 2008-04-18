#ifndef MANTID_DATAOBJECTS_HISTOGRAM1D_H_
#define MANTID_DATAOBJECTS_HISTOGRAM1D_H_

#include <boost/shared_ptr.hpp>
#include <vector>
#include "MantidKernel/System.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidAPI/IErrorHelper.h"

namespace Mantid
{
namespace DataObjects
{
/**
	1D histogram implementation.

    \class Histogram1D Histogram1D.h
    \author Laurent C Chapon, ISIS, RAL
    \date 26/09/2007  
    
  Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories

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
  typedef std::vector<double> StorageType;
  /// Data Store: NOTE:: CHANGED TO BREAK THE WRONG USEAGE OF SHARED_PTR 
  typedef Kernel::cow_ptr<StorageType > RCtype;    
  
private:
  RCtype refX;   ///< RefCounted X
  RCtype refY;   ///< RefCounted Y
  RCtype refE;   ///< RefCounted Error
  RCtype refE2;  ///< Second error value for when Poisson errors used
  const API::IErrorHelper* _errorHelper; ///<pointer to the error helper class for this spectra
  int _spectraNo; ///< The spectra no for this histogram 0 = not set

public:
  Histogram1D();
  Histogram1D(const Histogram1D&);
  Histogram1D& operator=(const Histogram1D&);
  virtual ~Histogram1D();

  /// Sets the x data.
  void setX(const StorageType& X) {  refX.access()=X; }
  /// Sets the data.
  void setData(const StorageType& Y) {  refY.access()=Y; };
  /// Sets the data and single-value errors
  void setData(const StorageType& Y, const StorageType& E) 
    {  refY.access()=Y; refE.access()=E; }
  /// Sets the data and errors
  void setData(const StorageType& Y, const StorageType& E, const StorageType& E2) 
    {  refY.access()=Y; refE.access()=E; refE2.access(); }

  /// Sets the x data.
  void setX(const RCtype& X) { refX=X; }
  /// Sets the data.
  void setData(const RCtype& Y) { refY=Y; }
  /// Sets the data and single-value errors
  void setData(const RCtype& Y, const RCtype& E) { refY=Y; refE=E;}
  /// Sets the data and errors
  void setData(const RCtype& Y, const RCtype& E, const RCtype& E2) { refY=Y; refE=E; refE2=E2; }  
  
  /// Sets the x data
  void setX(const RCtype::ptr_type& X) { refX=X; }
  /// Sets the data.
  void setData(const RCtype::ptr_type& Y) { refY=Y; }
  /// Sets the data and single-value errors
  void setData(const RCtype::ptr_type& Y, const RCtype::ptr_type& E) { refY=Y; refE=E;}
  /// Sets the data and errors
  void setData(const RCtype::ptr_type& Y, const RCtype::ptr_type& E, const RCtype::ptr_type& E2) 
    { refY=Y; refE=E; refE2=E2; }

  // Get the array data
  /// Returns the x data const
  virtual const StorageType& dataX() const { return *refX; }  
  /// Returns the y data const
  virtual const StorageType& dataY() const { return *refY; }
  /// Returns the error data const
  virtual const StorageType& dataE() const { return *refE; }
  /// Returns the second error value data const
  virtual const StorageType& dataE2() const { return *refE2; }

  ///Returns the x data
  virtual StorageType& dataX() { return refX.access(); }
  ///Returns the y data
  virtual StorageType& dataY() { return refY.access(); }
  ///Returns the error data
  virtual StorageType& dataE() { return refE.access(); }
  ///Returns the second error value data
  virtual StorageType& dataE2() { return refE2.access(); }

  ///Clear the x data
  StorageType& emptyX() { refX.access().clear(); return refX.access(); }
  ///Clear the y data
  StorageType& emptyY() { refY.access().clear(); return refY.access(); }
  ///Clear the error data
  StorageType& emptyE() { refE.access().clear(); return refE.access(); }
  ///Clear the second error value data
  StorageType& emptyE2() { refE2.access().clear(); return refE2.access(); }

  int nxbin() const { return refX->size(); }         ///< Return the number of X bins
  int nybin() const { return refY->size(); }         ///< Return the number of data bin (Y or YE)
  virtual int size() const { return refY->size(); }          ///< get pseudo size

  /// Checks for errors
  bool isError() const { return refE->empty(); }
  /// Gets the memory size of the histogram
  long int getMemorySize() const 
    { return (refX->size()+refY->size()+refE->size()+refE2->size())*sizeof(double); }

  ///Returns the ErrorHelper applicable for this detector
  void setErrorHelper(const API::IErrorHelper* errorHelper) { _errorHelper = errorHelper; }
  void setErrorHelper(API::IErrorHelper* errorHelper) { _errorHelper = errorHelper; }
  const API::IErrorHelper* errorHelper() const { return _errorHelper; }
  /// Returns the spectrum number to which this histogram refers
  const int spectraNo() const { return _spectraNo; }
  /// The spectrum number to which this histogram refers
  int& spectraNo() { return _spectraNo; }

};

} // namespace DataObjects
}  //Namespace Mantid
#endif /*MANTID_DATAOBJECTS_HISTOGRAM1D_H_*/
