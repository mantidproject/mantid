#ifndef MANTID_API_FILEBACKEDEXPERIMENTINFO_H_
#define MANTID_API_FILEBACKEDEXPERIMENTINFO_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"

namespace Mantid {
namespace API {

/**
    Implements a lazy-loading mechanism for the experimental information
    stored in a NeXus file.

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
class MANTID_API_DLL FileBackedExperimentInfo : public ExperimentInfo {
public:
  FileBackedExperimentInfo(const std::string &filename,
                           const std::string &nxpath);
  ExperimentInfo *cloneExperimentInfo() const override;

private:
  void populateIfNotLoaded() const override;
  void populateFromFile() const;

  mutable bool m_loaded;
  std::string m_filename;
  std::string m_nxpath;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_FILEBACKEDEXPERIMENTINFO_H_ */
