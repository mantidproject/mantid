#ifndef MANTID_DATAHANDLING_DOWNLOADFILE_H_
#define MANTID_DATAHANDLING_DOWNLOADFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {

namespace Kernel {
// forward Declaration
class InternetHelper;
}

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
  virtual ~DownloadFile();

  virtual const std::string name() const;
  virtual int version() const;
  virtual const std::string category() const;
  virtual const std::string summary() const;

protected:
  Kernel::InternetHelper *m_internetHelper;

private:
  void init();
  void exec();
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_DOWNLOADFILE_H_ */