#ifndef MANTID_DATAOBJECTS_MANAGEDWORKSPACE2D_H_
#define MANTID_DATAOBJECTS_MANAGEDWORKSPACE2D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataObjects/ManagedDataBlock2D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/AbsManagedWorkspace2D.h"
#include <fstream>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace Mantid
{
namespace DataObjects
{
/** The ManagedWorkspace2D allows the framework to handle 2D datasets that are too
    large to fit in the available system memory by making use of a temporary file.
    It is a specialisation of Workspace2D.

    The optional configuration property ManagedWorkspace.DataBlockSize sets the size
    (in bytes) of the blocks used to internally buffer data. The default is 1MB.

    @author Russell Taylor, Tessella Support Services plc
    @date 22/01/2008

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
	class DLLExport ManagedWorkspace2D : public AbsManagedWorkspace2D
{

public:
  ManagedWorkspace2D();
  virtual ~ManagedWorkspace2D();

  virtual const std::string id() const {return "ManagedWorkspace2D";}

  virtual long int getMemorySize() const;
	virtual bool threadSafe() const { return false; }

protected:

  /// Reads in a data block.
  virtual void readDataBlock(ManagedDataBlock2D *newBlock,int startIndex)const;
  /// Saves the dropped data block to disk.
  virtual void writeDataBlock(ManagedDataBlock2D *toWrite) const;

private:
  // Make copy constructor and copy assignment operator private (and without definition) unless they're needed
  /// Private copy constructor
  ManagedWorkspace2D(const ManagedWorkspace2D&);
  /// Private copy assignment operator
  ManagedWorkspace2D& operator=(const ManagedWorkspace2D&);

  virtual void init(const int &NVectors, const int &XLength, const int &YLength);

  virtual int getHistogramNumberHelper() const;

  ManagedDataBlock2D* getDataBlock(const int index) const;

  /// The number of blocks per temporary file
  int m_blocksPerFile;

  /// The name of the temporary file
  std::string m_filename;
  /// The stream handle to the temporary file used to store the data
  mutable std::vector<std::fstream*> m_datafile;
  /// Index written up to in temporary file
  mutable int m_indexWrittenTo;

  /// Static instance count. Used to ensure temporary filenames are distinct.
  static int g_uniqueID;
};

} // namespace DataObjects
} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_MANAGEDWORKSPACE2D_H_*/
