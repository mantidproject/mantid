#ifndef MANTID_DATAHANDLING_SAVENXCANSAS_H_
#define MANTID_DATAHANDLING_SAVENXCANSAS_H_

#include "MantidDataHandling/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include <H5Cpp.h>

namespace Mantid {
namespace DataHandling {

/** SaveNXcanSAS : Saves a reduced workspace in the NXcanSAS format. Currently
 * only MatrixWorkspaces resulting from 1D and 2D reductions are supported.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_DATAHANDLING_DLL SaveNXcanSAS : public API::Algorithm {
public:
  /// Constructor
  SaveNXcanSAS();
  /// Virtual dtor
  ~SaveNXcanSAS() override {}
  const std::string name() const override { return "SaveNXcanSAS"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Writes a MatrixWorkspace to a file in the NXcanSAS format.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\Nexus";
  }

  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};


namespace H5Util {
  /// Create a 1D data-space to hold data of length.
  MANTID_DATAHANDLING_DLL H5::DataSpace getDataSpace(const size_t length);

  /// Create a 1D data-space that will hold the supplied vector.
  template <typename NumT>
  H5::DataSpace getDataSpace(const std::vector<NumT> &data);

  /// Convert a primitive type to the appropriate H5::DataType.
  template <typename NumT> H5::DataType getType();

  MANTID_DATAHANDLING_DLL H5::Group createGroupNXS(H5::H5File &file,
                                                   const std::string &name,
                                                   const std::string &nxtype);

  MANTID_DATAHANDLING_DLL H5::Group createGroupNXS(H5::Group &group,
                                                   const std::string &name,
                                                   const std::string &nxtype);
  /**
   * Sets up the chunking and compression rate.
   * @param length
   * @param deflateLevel
   * @return The configured property list
   */
  MANTID_DATAHANDLING_DLL H5::DSetCreatPropList
  setCompressionAttributes(const std::size_t length, const int deflateLevel = 6);

  MANTID_DATAHANDLING_DLL void writeStrAttribute(H5::Group &location,
                                                 const std::string &name,
                                                 const std::string &value);

  MANTID_DATAHANDLING_DLL void write(H5::Group &group, const std::string &name,
                                     const std::string &value);

  template <typename NumT>
  void writeArray1D(H5::Group &group, const std::string &name,
                    const std::vector<NumT> &values);

  MANTID_DATAHANDLING_DLL std::string readString(H5::H5File &file,
                                                 const std::string &path);

  MANTID_DATAHANDLING_DLL std::string readString(H5::Group &group,
                                                 const std::string &name);

  MANTID_DATAHANDLING_DLL std::string readString(H5::DataSet &dataset);

  template <typename NumT>
  std::vector<NumT> readArray1DCoerce(H5::Group &group, const std::string &name);

  template <typename NumT>
  std::vector<NumT> readArray1DCoerce(H5::DataSet &dataset);
}

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SAVENXCANSAS_H_ */
