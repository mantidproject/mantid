#ifndef MANTID_KERNEL_FILEDESCRIPTOR_H_
#define MANTID_KERNEL_FILEDESCRIPTOR_H_

#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/DllConfig.h"

#include <fstream>
#include <string>

namespace Mantid {
namespace Kernel {

/**
Defines a wrapper around an open file. Details of the file such as the filename
& extension can be queried.
The file is closed when the object is destroyed.

The object stores an opened fstream object that can be accessed using the data()
method.

Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class MANTID_KERNEL_DLL FileDescriptor {
public:
  /// Returns true if the file is considered ascii
  static bool isAscii(const std::string &filename, const size_t nbytes = 256);
  /// Returns true if the stream is considered ascii
  static bool isAscii(std::istream &data, const size_t nbytes = 256);
  /// Returns true if the file is considered ascii
  static bool isAscii(FILE *file, const size_t nbytes = 256);

public:
  /// Constructor accepting a filename
  FileDescriptor(const std::string &filename);
  /// Destructor
  ~FileDescriptor();

  /**
   * Access the filename
   * @returns A reference to a const string containing the filename
   */
  inline const std::string &filename() const { return m_filename; }
  /**
   * Access the file extension. Defined as the string after and including the
   * last period character
   * @returns A reference to a const string containing the file extension
   */
  inline const std::string &extension() const { return m_extension; }
  /**
   * Returns true if the descriptor is looking at an ascii file
   */
  inline bool isAscii() const { return m_ascii; }
  /**
   * Access the open file stream. DO NOT CLOSE IT
   * @returns The current stream
   */
  inline std::istream &data() { return m_file; }
  /// Reset the file stream to the start of the file
  void resetStreamToStart();

private:
  DISABLE_DEFAULT_CONSTRUCT(FileDescriptor)
  DISABLE_COPY_AND_ASSIGN(FileDescriptor)

  /// Open the file and cache description elements
  void initialize(const std::string &filename);

  /// Full filename
  std::string m_filename;
  /// Extension
  std::string m_extension;
  /// Open file stream
  std::ifstream m_file;
  /// Flag indicating the file is pure ascii
  bool m_ascii;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_FILEDESCRIPTOR_H_ */
