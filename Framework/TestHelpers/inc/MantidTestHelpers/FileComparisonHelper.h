#ifndef FILECOMPARISONHELPER_H_
#define FILECOMPARISONHELPER_H_

#include <iosfwd>
#include <iterator>
#include <string>

namespace FileComparisonHelper {
/**
  FileComparisonHelper provides several helper functions to compare
  files or file-streams within unit tests. It accounts for EOL
  differences between Unix and Windows and will emit an error in
  the Mantid logging system as to how the files differed

  @author David Fairbrother
  @date 2017

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

/// Typedef buffered stream iterator (char) to a shorter name
using streamCharIter = std::istreambuf_iterator<char>;

/// Compares the length and content of std::streams iterators
///	and returns if they are equal
bool areIteratorsEqual(streamCharIter refStream, streamCharIter testStream,
                       streamCharIter refStreamEnd = streamCharIter(),
                       streamCharIter testStreamEnd = streamCharIter());

/// Checks if two files are equal in content and length at the specified path.
/// Accounts for EOL differences if they exist
bool areFilesEqual(const std::string &referenceFileFullPath,
                   const std::string &outFileFullPath);

/// Checks if two file streams are equal in content and length. Accounts for EOL
/// differences.
bool areFileStreamsEqual(std::ifstream &referenceFileStream,
                         std::ifstream &fileToCheck);

/// Attempts to find a reference file with the given name using Mantid
/// then compares content and length of files (ignoring EOL differences).
bool isEqualToReferenceFile(const std::string &referenceFileName,
                            const std::string &outFileFullPath);
} // Namespace FileComparisonHelper

#endif // FILECOMPARISONHELPER_H_
