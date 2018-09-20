#ifndef MANTID_DATAHANDLING_DOWNLOADFILE_H_
#define MANTID_DATAHANDLING_DOWNLOADFILE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {

namespace Kernel {
// forward Declaration
class InternetHelper;
} // namespace Kernel

namespace DataHandling {

/** DownloadFile : Downloads a file from a url to the file system

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport DownloadFile : public API::Algorithm {
public:
  DownloadFile();
  ~DownloadFile() override;

  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"Load", "CatalogDownloadDataFiles"};
  }
  const std::string category() const override;
  const std::string summary() const override;

protected:
  Kernel::InternetHelper *m_internetHelper;

private:
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_DOWNLOADFILE_H_ */