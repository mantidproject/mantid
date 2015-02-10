#ifndef MANTID_MDALGORITHMS_CONVERTSPICEDATATOREALSPACE_H_
#define MANTID_MDALGORITHMS_CONVERTSPICEDATATOREALSPACE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/FileDescriptor.h"
#include "MantidAPI/IMDEventWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

/** LoadHFIRPDD : TODO: DESCRIPTION

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
  ConvertSpiceDataToRealSpace();
  virtual ~ConvertSpiceDataToRealSpace();

  /// Algorithm's name
  virtual const std::string name() const { return "ConvertSpiceDataToRealSpace"; }

  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Load a HFIR powder diffractometer SPICE file.";
  }

  /// Algorithm's version
  virtual int version() const { return (1); }

  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Diffraction;DataHandling\\Text";
  }

  /// Returns a confidence value that this algorithm can load a file
  // virtual int confidence(Kernel::FileDescriptor &descriptor) const;

private:
  /// Initialisation code
  void init();

  /// Execution code
  void exec();

  /// Load data by call
  DataObjects::TableWorkspace_sptr
  loadSpiceData(const std::string &spicefilename);

  /// Convert to MD workspaces
  API::IMDEventWorkspace_sptr
  convertToMDEventWS(const std::vector<API::MatrixWorkspace_sptr> &vec_ws2d);

  /// Parse data table workspace to a vector of matrix workspaces
  std::vector<API::MatrixWorkspace_sptr>
  convertToWorkspaces(DataObjects::TableWorkspace_sptr tablews,
                      API::MatrixWorkspace_const_sptr parentws,
                      Kernel::DateAndTime runstart,
                      std::map<std::string, std::vector<double> > &logvecmap,
                      std::vector<Kernel::DateAndTime> &vectimes);

  /// Read parameter information from table workspace
  void readTableInfo(DataObjects::TableWorkspace_const_sptr tablews,
                     size_t &ipt, size_t &irotangle, size_t &itime,
                     std::vector<std::pair<size_t, size_t> > &anodelist,
                     std::map<std::string, size_t> &samplenameindexmap);

  /// Return sample logs
  void parseSampleLogs(DataObjects::TableWorkspace_sptr tablews,
                       const std::map<std::string, size_t> &indexlist,
                       std::map<std::string, std::vector<double> > &logvecmap);

  /// Load one run (one pt.) to a matrix workspace
  API::MatrixWorkspace_sptr
  loadRunToMatrixWS(DataObjects::TableWorkspace_sptr tablews, size_t irow,
                    API::MatrixWorkspace_const_sptr parentws,
                    Kernel::DateAndTime runstart, size_t ipt, size_t irotangle,
                    size_t itime,
                    const std::vector<std::pair<size_t, size_t> > anodelist,
                    double &duration);

  /// Append Experiment Info
  void
  addExperimentInfos(API::IMDEventWorkspace_sptr mdws,
                     const std::vector<API::MatrixWorkspace_sptr> vec_ws2d);

  /// Append sample logs to MD workspace
  void
  appendSampleLogs(API::IMDEventWorkspace_sptr mdws,
                   const std::map<std::string, std::vector<double> > &logvecmap,
                   const std::vector<Kernel::DateAndTime> &vectimes);

  /// Create an MDWorkspace for monitor counts
  API::IMDEventWorkspace_sptr createMonitorMDWorkspace(
      const std::vector<API::MatrixWorkspace_sptr> vec_ws2d,
      const std::vector<double> &vecmonitor);

  /// Name of instrument
  std::string m_instrumentName;

  /// Number of detectors
  size_t m_numSpec;
};

} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_MDALGORITHMS_CONVERTSPICEDATATOREALSPACE_H_ */
