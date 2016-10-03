#ifndef MANTID_DATAHANDLING_LOADSAVUTOMOCONFIG_H_
#define MANTID_DATAHANDLING_LOADSAVUTOMOCONFIG_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

namespace NeXus {
class File;
}

namespace Mantid {
namespace DataHandling {

/**
  LoadSavuTomoConfig : Load a tomographic reconstruction parameters
  file (as used in the savu tomography reconstructin pipeline) into a
  TableWorkspace

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
class DLLExport LoadSavuTomoConfig : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadSavuTomoConfig"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load configuration parameters from a tomographic "
           "reconstruction parameter file.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Imaging";
  }

private:
  /// Implement abstract Algorithm methods
  void init() override;
  /// Implement abstract Algorithm methods
  void exec() override;

  // do the real loading
  Mantid::API::ITableWorkspace_sptr loadFile(std::string &fname,
                                             std::string &wsName);

  // open file safely and carefully checking potential issues
  bool checkOpenFile(std::string fname, boost::shared_ptr<NeXus::File> &f);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADSAVUTOMOCONFIG_H_ */
