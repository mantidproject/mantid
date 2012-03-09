#ifndef MANTID_DATAOBJECTS_MANAGEDDATABLOCK2D_H_
#define MANTID_DATAOBJECTS_MANAGEDDATABLOCK2D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ISpectrum.h"
#include "MantidDataObjects/DllConfig.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidDataObjects/ManagedHistogram1D.h"
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

    Copyright &copy; 2008-2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  ManagedDataBlock2D(const std::size_t &minIndex, const std::size_t &noVectors, const std::size_t &XLength, const std::size_t &YLength,
      AbsManagedWorkspace2D * parentWS, MantidVecPtr sharedDx);
  virtual ~ManagedDataBlock2D();

  void initialize();

  int minIndex() const;
  bool hasChanges() const;
  void hasChanges(bool has);

  void releaseData();

  size_t getNumSpectra() const
  { return m_data.size(); }

  /// Return the underlying ISpectrum ptr at the given workspace index.
  virtual Mantid::API::ISpectrum * getSpectrum(const size_t index);

  /// Return the underlying ISpectrum ptr (const version) at the given workspace index.
  virtual const Mantid::API::ISpectrum * getSpectrum(const size_t index) const;

  //------------------------------------------------------------------------
  /// @return true if the data was loaded from disk
  bool isLoaded() const
  { return m_loaded; }

  /** Set the loaded flag
   * @param loaded :: bool flag value */
  void setLoaded(bool loaded)
  { m_loaded = loaded; }

  
private:
  // Make copy constructor and copy assignment operator private (and without definition) unless they're needed
  /// Private copy constructor
  ManagedDataBlock2D(const ManagedDataBlock2D&);
  /// Private copy assignment operator
  ManagedDataBlock2D& operator=(const ManagedDataBlock2D&);
  
  /// The data 'chunk'. NOTE: These pointers are owned by Workspace2D!
  std::vector<ManagedHistogram1D *> m_data;

  /// The length of the X vector in each Histogram1D. Must all be the same. 
  const std::size_t m_XLength;
  /// The length of the Y & E vectors in each Histogram1D. Must all be the same. 
  const std::size_t m_YLength;
  /// The index of the workspace that this datablock starts from.
  const std::size_t m_minIndex;
  
  /// Is the data block initialized or loaded from disk?
  bool m_loaded;

  /// Static reference to the logger class
  static Kernel::Logger &g_log;
};

} // namespace DataObjects
} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_MANAGEDDATABLOCK2D_H_*/
