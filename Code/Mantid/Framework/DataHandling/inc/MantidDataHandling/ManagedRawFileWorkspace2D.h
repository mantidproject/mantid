#ifndef MANTID_DATAOBJECTS_MANAGEDRAWFILEWORKSPACE2D_H_
#define MANTID_DATAOBJECTS_MANAGEDRAWFILEWORKSPACE2D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataObjects/ManagedWorkspace2D.h"
#include "MantidKernel/System.h"
#include <Poco/Mutex.h>

class ISISRAW2;

namespace Mantid
{

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}

namespace DataHandling
{
/** \class ManagedRawFileWorkspace2D

    Concrete workspace implementation. Data is a vector of Histogram1D.
    Since Histogram1D have share ownership of X, Y or E arrays,
    duplication is avoided for workspaces for example with identical time bins.

    \author 
    \date 

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport ManagedRawFileWorkspace2D : public DataObjects::ManagedWorkspace2D
{
public:
  /**
  Gets the name of the workspace type
  @return Standard string name
   */
  virtual const std::string id() const {return "ManagedRawFileWorkspace2D";}

  explicit ManagedRawFileWorkspace2D(const std::string& fileName, int opt=0);
  virtual ~ManagedRawFileWorkspace2D();
 
protected:
  /// Reads in a data block.
  virtual void readDataBlock(DataObjects::ManagedDataBlock2D *newBlock,int startIndex)const;
  /// Saves the dropped data block to disk.
  virtual void writeDataBlock(DataObjects::ManagedDataBlock2D *toWrite) const;

private:
  /// Private copy constructor. NO COPY ALLOWED
  ManagedRawFileWorkspace2D(const ManagedRawFileWorkspace2D&);
  /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
  ManagedRawFileWorkspace2D& operator=(const ManagedRawFileWorkspace2D&);

  /// Sets the RAW file for this workspace. Called by the constructor.
  void setRawFile(const int opt);
  void getTimeChannels();
  bool needCache(const int opt);
  void openTempFile();
  void removeTempFile()const;
  /// returns true if the given spectrum index is a monitor
  bool isMonitor(const int64_t readIndex) const;

  boost::shared_ptr<ISISRAW2> isisRaw;        ///< Pointer to an ISISRAW2 object
  const std::string m_filenameRaw;///< RAW file name.
  FILE* m_fileRaw;          ///< RAW file pointer.
  fpos_t m_data_pos;        ///< Position in the file where the data start.
  mutable int m_readIndex;  ///< Index of the spectrum which starts at current position in the file (== index_of_last_read + 1)
  std::vector<boost::shared_ptr<MantidVec> > m_timeChannels; ///< Time bins
  std::map<int64_t,int64_t> m_specTimeRegimes;   ///< Stores the time regime for each spectrum

  // For each data block holds true if it has been modified and must read from ManagedWorkspace2D flat file
  // of false if it must be read from the RAW file.
  // The block's index = startIndex / m_vectorsPerBlock.
  mutable std::vector<bool> m_changedBlock;  ///< Flags for modified blocks. Modified blocks are accessed through ManagedWorkspace2D interface
  static int64_t g_uniqueID;             ///< Counter used to create unique file names
  std::string m_tempfile;            ///< The temporary file name

  int64_t m_numberOfTimeChannels;  ///< The number of time channels (i.e. bins) from the RAW file
  int64_t m_numberOfBinBoundaries; ///< The number of time bin boundaries == m_numberOfTimeChannels + 1
  int64_t m_numberOfSpectra;       ///< The number of spectra in the raw file
  int64_t m_numberOfPeriods;       ///< The number of periods in the raw file

  mutable Poco::FastMutex m_mutex;  ///< The mutex

  /// Static reference to the logger class
  static Kernel::Logger &g_log;

  /// a counter used for skipping the raw file if it's a monitor
  mutable int64_t m_nmonitorSkipCounter;
 
};

} // namespace DataHandling
} // Namespace Mantid
#endif /*MANTID_DATAOBJECTS_MANAGEDRAWFILEWORKSPACE2D_H_*/
