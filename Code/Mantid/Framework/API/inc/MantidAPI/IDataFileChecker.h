#ifndef MANTID_API_IDATAFILECHECKER_H
#define MANTID_API_IDATAFILECHECKER_H

#include "MantidAPI/Algorithm.h"

#ifdef WIN32
static const unsigned char g_hdf5_signature[] = { '\211', 'H', 'D', 'F', '\r', '\n', '\032', '\n' };
/// Magic HDF5 cookie that is stored in the first 4 bytes of the file.
static const uint32_t g_hdf5_cookie = 0x0e031301;
#endif

namespace Mantid
{
  namespace API
  {

    /** 
    Base class for data file loading algorithms.This class provides interface for 
    data file loading algorithms to quickly check the file type by opening it and 
    reading the first 100 bytes /extension of the file. It also provides an interface to 
    check how much it can load the file by reading the first few lines of the file.
    
    @author Sofia Antony, ISIS Rutherford Appleton Laboratory
    @date 17/11/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
    */
    class DLLExport IDataFileChecker : public Algorithm
    {
    public:
      ///constructor
      IDataFileChecker();
      /// virtual destructor
      virtual ~IDataFileChecker();

#ifndef WIN32
      /// Magic signature identifying a HDF5 file.
      static unsigned char const g_hdf5_signature[8];
      /// Magic HDF5 cookie that is stored in the first 4 bytes of the file.
      static uint32_t const g_hdf5_cookie;
#endif
      /// The default number of bytes of the header to check
      enum { g_hdr_bytes = 100 };

      /// A union representing the first g_hdr_bytes of a file
      union file_header
      {
        /// The first four-bytes of the header
        uint32_t four_bytes;
        /// The full header buffer, including a null terminator
        unsigned char full_hdr[g_hdr_bytes+1];
      };

      /// quick file check by reading first g_bufferSize bytes of the file or by checking the extension
      virtual bool quickFileCheck(const std::string& filePath,size_t nread,const file_header & header)=0;
      /// file check by looking at the structure of the data file
      virtual int fileCheck(const std::string& filePath)=0;
      /// returns the extension of the file from filename
      std::string extension(const std::string& filePath);
    };

    /// Typedef for a shared pointer
    typedef boost::shared_ptr<IDataFileChecker> IDataFileChecker_sptr;
    
  }
}
#endif
