#ifndef MANTID_KERNEL_ICATALOGINFO_H_
#define MANTID_KERNEL_ICATALOGINFO_H_

#include "MantidKernel/DllConfig.h"
#include <string>

namespace Mantid {
namespace Kernel {

/** ICatalogInfo : An abstract class that holds information about catalogs.

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_KERNEL_DLL ICatalogInfo {
public:
  /// Obtain catalog name from the facility file.
  virtual const std::string catalogName() const = 0;
  /// Obtain soap end point from the facility file.
  virtual const std::string soapEndPoint() const = 0;
  /// Obtain the external download URL.
  virtual const std::string externalDownloadURL() const = 0;
  /// Obtain the regex prefix from the  facility file.
  virtual const std::string catalogPrefix() const = 0;
  /// Obtain Windows prefix from the facility file.
  virtual const std::string windowsPrefix() const = 0;
  /// Obtain Macintosh prefix from facility file.
  virtual const std::string macPrefix() const = 0;
  /// Obtain Linux prefix from facility file.
  virtual const std::string linuxPrefix() const = 0;
  /// Clone
  virtual ICatalogInfo *clone() const = 0;
  /// Transform's the archive path based on operating system used.
  virtual std::string transformArchivePath(const std::string &path) const;

  /// virtual destructor
  virtual ~ICatalogInfo() = default;

private:
  /// Replace the content of a string using regex.
  std::string replacePrefix(const std::string &path, const std::string &regex,
                            const std::string &prefix) const;
  /// Replace all occurrences of the search string in the input with the format
  /// string.
  std::string replaceAllOccurences(const std::string &path,
                                   const std::string &search,
                                   const std::string &format) const;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_ICATALOGINFO_H_ */
