#ifndef MANTID_DATAOBJECTS_MANAGEDDATABLOCK2D_H_
#define MANTID_DATAOBJECTS_MANAGEDDATABLOCK2D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataObjects/Histogram1D.h"
#include "MantidKernel/DllExport.h"
#include "MantidKernel/cow_ptr.h"
#include <fstream>
#include <vector>

namespace Mantid
{

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}
	
namespace DataObjects
{
/** Stores a block of 2D data.
    The data storage is the same as that of a Workspace2D (i.e. a vector of Histogram1D's),
    but no sample, instrument or history data is held here.
    The class supports the Workspace iterators.

    @author Russell Taylor, Tessella Support Services plc
    @date 18/01/2008
    
    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport ManagedDataBlock2D
{
  /// Output a string representation to a stream
  friend DLLExport std::fstream& operator<<(std::fstream&, ManagedDataBlock2D&);
  /// Input a string representation to a stream
  friend DLLExport std::fstream& operator>>(std::fstream&, ManagedDataBlock2D&);
  
public:
  ManagedDataBlock2D(const std::size_t &minIndex, const std::size_t &noVectors, const std::size_t &XLength, const std::size_t &YLength);
  virtual ~ManagedDataBlock2D();

  int minIndex() const;
  int hashIndexFunction() const;
  bool hasChanges() const;
  void hasChanges(bool has);

  // Must be a case for having an interface for these accessor methods, which are the same as Workspace2D
  void setX(const std::size_t histnumber, const MantidVecPtr&);
  void setX(const std::size_t histnumber, const MantidVecPtr::ptr_type&);
  void setData(const std::size_t histnumber, const MantidVecPtr&);
  void setData(const std::size_t histnumber, const MantidVecPtr&, const MantidVecPtr&);
  void setData(const std::size_t histnumber, const MantidVecPtr::ptr_type&, const MantidVecPtr::ptr_type&);
	
  MantidVec& dataX(const std::size_t index);
  MantidVec& dataY(const std::size_t index);
  MantidVec& dataE(const std::size_t index);
  const MantidVec& dataX(const std::size_t index) const;
  const MantidVec& dataY(const std::size_t index) const;
  const MantidVec& dataE(const std::size_t index) const;
  MantidVecPtr refX(const std::size_t index) const;
  
private:
  // Make copy constructor and copy assignment operator private (and without definition) unless they're needed
  /// Private copy constructor
  ManagedDataBlock2D(const ManagedDataBlock2D&);
  /// Private copy assignment operator
  ManagedDataBlock2D& operator=(const ManagedDataBlock2D&);
  
  /// The data 'chunk'
  std::vector<Histogram1D> m_data;
  /// The length of the X vector in each Histogram1D. Must all be the same. 
  const int m_XLength;
  /// The length of the Y & E vectors in each Histogram1D. Must all be the same. 
  const int m_YLength;
  /// The index of the workspace that this datablock starts from.
  const int m_minIndex;
  /// A 'dirty' flag. Set if any of the elements of m_data are accessed through non-const accessors.
  bool m_hasChanges;
  
  /// Static reference to the logger class
  static Kernel::Logger &g_log;
};

} // namespace DataObjects
} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_MANAGEDDATABLOCK2D_H_*/
