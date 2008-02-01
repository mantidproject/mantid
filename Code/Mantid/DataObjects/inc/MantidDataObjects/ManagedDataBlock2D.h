#ifndef MANTID_DATAOBJECTS_MANAGEDDATABLOCK2D_H_
#define MANTID_DATAOBJECTS_MANAGEDDATABLOCK2D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataObjects/Histogram1D.h"
#include "MantidKernel/Logger.h"
#include <fstream>

namespace Mantid
{
namespace DataObjects
{
/** Stores a block of 2D data.
    The data storage is the same as that of a Workspace2D (i.e. a vector of Histogram1D's),
    but no sample, instrument or history data is held here.
    The class supports the Workspace iterators.

    @author Russell Taylor, Tessella Support Services plc
    @date 18/01/2008
    
    Copyright &copy; 2008 STFC Rutherford Appleton Laboratories

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
class ManagedDataBlock2D
{
  friend std::fstream& operator<<(std::fstream&, ManagedDataBlock2D&);
  friend std::fstream& operator>>(std::fstream&, ManagedDataBlock2D&);
  
public:
	ManagedDataBlock2D(const unsigned int &minIndex, const unsigned int &noVectors, const unsigned int &XLength, const unsigned int &YLength);
	virtual ~ManagedDataBlock2D();
	
	unsigned int minIndex() const;
	bool hasChanges() const;

	// Must be a case for having an interface for these accessor methods, which are the same as Workspace2D
  void setX(const int histnumber, const std::vector<double>&);
  void setX(const int histnumber, const Histogram1D::RCtype&);
  void setX(const int histnumber, const Histogram1D::RCtype::ptr_type&);
  void setData(const int histnumber, const std::vector<double>&);
  void setData(const int histnumber, const std::vector<double>&, const std::vector<double>&);
  void setData(const int histnumber, const std::vector<double>&, const std::vector<double>&, const std::vector<double>&);
  void setData(const int histnumber, const Histogram1D::RCtype&);
  void setData(const int histnumber, const Histogram1D::RCtype&, const Histogram1D::RCtype&);
  void setData(const int histnumber, const Histogram1D::RCtype&, const Histogram1D::RCtype&, const Histogram1D::RCtype&);
  void setData(const int histnumber, const Histogram1D::RCtype::ptr_type&, const Histogram1D::RCtype::ptr_type&);
  void setData(const int histnumber, const Histogram1D::RCtype::ptr_type&, const Histogram1D::RCtype::ptr_type&, const Histogram1D::RCtype::ptr_type&);
	
  std::vector<double>& dataX(const int index);
  std::vector<double>& dataY(const int index);
  std::vector<double>& dataE(const int index);
  std::vector<double>& dataE2(const int index);
  const std::vector<double>& dataX(const int index) const;
  const std::vector<double>& dataY(const int index) const;
  const std::vector<double>& dataE(const int index) const;
  const std::vector<double>& dataE2(const int index) const;
  
private:
  // Make copy constructor and copy assignment operator private (and without definition) unless they're needed
  /// Private copy constructor
  ManagedDataBlock2D(const ManagedDataBlock2D&);
  /// Private copy assignment operator
  ManagedDataBlock2D& operator=(const ManagedDataBlock2D&);
  
  /// The data 'chunk'
  std::vector<Histogram1D> m_data;
  /// The length of the X vector in each Histogram1D. Must all be the same. 
  const unsigned int m_XLength;
  /// The length of the Y & E vectors in each Histogram1D. Must all be the same. 
  const unsigned int m_YLength;
  /// The index of the workspace that this datablock starts from.
  const unsigned int m_minIndex;
  /// A 'dirty' flag. Set if any of the elements of m_data are accessed through non-const accessors.
  bool m_hasChanges;
  
  /// Static reference to the logger class
  static Kernel::Logger &g_log;
};

} // namespace DataObjects
} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_MANAGEDDATABLOCK2D_H_*/
