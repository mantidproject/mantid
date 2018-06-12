#ifndef MANTID_MDALGORITHMS_LOADDNSSCD_H_
#define MANTID_MDALGORITHMS_LOADDNSSCD_H_

#include <vector>
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace MDAlgorithms {

/** LoadDNSSCD : Load a list of DNS .d_dat files into a MDEventWorkspace

  @author Marina Ganeva
  @date 2018-02-15

  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport LoadDNSSCD : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  LoadDNSSCD();

  /// Algorithm's name for identification
  const std::string name() const override { return "LoadDNSSCD"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load a list of DNS .d_dat files into a MDEventWorkspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }

  /// Algorithm's category for identification
  const std::string category() const override {
    return "MDAlgorithms\\DataHandling";
  }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  /// number of workspace dimensions
  size_t m_nDims;

  /// type of normalization;
  std::string m_normtype;
  /// factor to multiply the error^2 for normalization
  double m_normfactor;

  /// structure for experimental data
  struct ExpData {
    double deterota;
    double huber;
    double wavelength;
    double norm;
    std::vector<double> signal;
    std::vector<int> detID;
  };

  std::vector<ExpData> m_data;

  /// Output IMDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr m_OutWS;

  void read_data(const std::string fname,
                 std::map<std::string, std::string> &str_metadata,
                 std::map<std::string, double> &num_metadata);
  void fillOutputWorkspace(double wavelength);
  API::ITableWorkspace_sptr saveHuber();
  void loadHuber(API::ITableWorkspace_sptr tws);
  template <class T>
  void updateProperties(API::Run &run, std::map<std::string, T> &metadata,
                        std::string time);
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_LOADDNSSCD_H_ */
