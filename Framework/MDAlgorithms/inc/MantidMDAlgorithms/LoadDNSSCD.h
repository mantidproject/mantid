// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_LOADDNSSCD_H_
#define MANTID_MDALGORITHMS_LOADDNSSCD_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include <vector>

namespace Mantid {
namespace MDAlgorithms {

/** LoadDNSSCD : Load a list of DNS .d_dat files into a MDEventWorkspace

  @author Marina Ganeva
  @date 2018-02-15
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

  const std::vector<std::string> seeAlso() const override {
    return {"LoadDNSLegacy", "LoadWANDSCD", "ConvertWANDSCDtoQ"};
  }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  /// The column separator
  std::string m_columnSep;

  /// number of workspace dimensions
  size_t m_nDims;

  /// maximal TOF (for extends)
  double m_tof_max;

  /// type of normalization;
  std::string m_normtype;
  /// factor to multiply the error^2 for normalization
  double m_normfactor;

  /// structure for experimental data
  struct ExpData {
    double deterota; // detector rotation angle
    double huber;    // sample rotation angle
    double wavelength;
    double norm;      // normalizarion
    size_t nchannels; // TOF channels number
    double chwidth;   // channel width, microseconds
    std::vector<std::vector<double>> signal;
    std::vector<int> detID;
  };

  std::vector<ExpData> m_data;

  /// Output IMDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr m_OutWS;

  int splitIntoColumns(std::list<std::string> &columns, std::string &str);
  void read_data(const std::string fname,
                 std::map<std::string, std::string> &str_metadata,
                 std::map<std::string, double> &num_metadata);
  void fillOutputWorkspace(double wavelength);
  void fillOutputWorkspaceRaw(double wavelength);
  API::ITableWorkspace_sptr saveHuber();
  void loadHuber(API::ITableWorkspace_sptr tws);
  template <class T>
  void updateProperties(API::Run &run, std::map<std::string, T> &metadata,
                        std::string time);
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_LOADDNSSCD_H_ */
