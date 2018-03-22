#ifndef MANTID_MDALGORITHMS_CONVERTSPICEDATATOREALSPACE_H_
#define MANTID_MDALGORITHMS_CONVERTSPICEDATATOREALSPACE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/FileDescriptor.h"

#include <deque>

namespace Mantid {
namespace MDAlgorithms {

/** ConvertSpiceDataToRealSpace : Convert data from SPICE file to singals
  in real space contained in MDEventWrokspaces

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
class DLLExport ConvertSpiceDataToRealSpace : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override {
    return "ConvertSpiceDataToRealSpace";
  }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load a HFIR powder diffractometer SPICE file.";
  }

  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override {
    return "Diffraction\\ConstantWavelength;DataHandling\\Text";
  }

  /// Returns a confidence value that this algorithm can load a file
  // virtual int confidence(Kernel::FileDescriptor &descriptor) const;

private:
  /// Typdef for the white-space separated file data type.
  using DataCollectionType = std::deque<std::string>;

  /// Initialisation code
  void init() override;

  /// Execution code
  void exec() override;

  /// Load data by call
  DataObjects::TableWorkspace_sptr
  loadSpiceData(const std::string &spicefilename);

  /// Parse data table workspace to a vector of matrix workspaces
  std::vector<API::MatrixWorkspace_sptr> convertToMatrixWorkspace(
      DataObjects::TableWorkspace_sptr tablews,
      API::MatrixWorkspace_const_sptr parentws,
      Types::Core::DateAndTime runstart,
      std::map<std::string, std::vector<double>> &logvecmap,
      std::vector<Types::Core::DateAndTime> &vectimes);

  /// Create an MDEventWorspace by converting vector of matrix workspace data
  API::IMDEventWorkspace_sptr
  createDataMDWorkspace(const std::vector<API::MatrixWorkspace_sptr> &vec_ws2d);

  /// Create an MDWorkspace for monitor counts
  API::IMDEventWorkspace_sptr createMonitorMDWorkspace(
      const std::vector<API::MatrixWorkspace_sptr> vec_ws2d,
      const std::vector<double> &vecmonitor);

  /// Read parameter information from table workspace
  void readTableInfo(DataObjects::TableWorkspace_const_sptr tablews,
                     size_t &ipt, size_t &irotangle, size_t &itime,
                     std::vector<std::pair<size_t, size_t>> &anodelist,
                     std::map<std::string, size_t> &samplenameindexmap);

  /// Return sample logs
  void parseSampleLogs(DataObjects::TableWorkspace_sptr tablews,
                       const std::map<std::string, size_t> &indexlist,
                       std::map<std::string, std::vector<double>> &logvecmap);

  /// Load one run (one pt.) to a matrix workspace
  API::MatrixWorkspace_sptr
  loadRunToMatrixWS(DataObjects::TableWorkspace_sptr tablews, size_t irow,
                    API::MatrixWorkspace_const_sptr parentws,
                    Types::Core::DateAndTime runstart, size_t ipt,
                    size_t irotangle, size_t itime,
                    const std::vector<std::pair<size_t, size_t>> anodelist,
                    double &duration);

  /// Append Experiment Info
  void
  addExperimentInfos(API::IMDEventWorkspace_sptr mdws,
                     const std::vector<API::MatrixWorkspace_sptr> vec_ws2d);

  /// Append sample logs to MD workspace
  void
  appendSampleLogs(API::IMDEventWorkspace_sptr mdws,
                   const std::map<std::string, std::vector<double>> &logvecmap,
                   const std::vector<Types::Core::DateAndTime> &vectimes);

  /// Parse detector efficiency table workspace to map
  std::map<detid_t, double>
  parseDetectorEfficiencyTable(DataObjects::TableWorkspace_sptr detefftablews);

  /// Apply the detector's efficiency correction to
  void
  correctByDetectorEfficiency(std::vector<API::MatrixWorkspace_sptr> vec_ws2d,
                              const std::map<detid_t, double> &detEffMap);

  /// Name of instrument
  std::string m_instrumentName;

  /// Number of detectors
  size_t m_numSpec = 0;

  /// x-y-z-value minimum
  std::vector<double> m_extentMins;
  /// x-y-z value maximum
  std::vector<double> m_extentMaxs;
  /// Number of bins
  std::vector<size_t> m_numBins;
  /// Dimension of the output MDEventWorkspace
  size_t m_nDimensions = 3;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CONVERTSPICEDATATOREALSPACE_H_ */
