// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidNexus/NexusClasses_fwd.h"

#include <utility>

namespace Mantid {
namespace DataHandling {

/** LoadILLPolarizedDiffraction : Loads ILL polarized diffraction nexus files
  from instrument D7.

  @date 15/05/20
*/
class MANTID_DATAHANDLING_DLL LoadILLPolarizedDiffraction : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"LoadNexus"}; }
  const std::string category() const override;
  const std::string summary() const override;
  int confidence(Kernel::NexusDescriptor &) const override;
  LoadILLPolarizedDiffraction();

private:
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;

  API::MatrixWorkspace_sptr initStaticWorkspace(const NeXus::NXEntry &);

  void loadData();
  void loadMetaData();
  API::WorkspaceGroup_sptr sortPolarisations();
  std::vector<double> loadTwoThetaDetectors(const API::MatrixWorkspace_sptr &, const NeXus::NXEntry &, const int);
  std::vector<double> loadBankParameters(const API::MatrixWorkspace_sptr &, const int);
  void moveTwoTheta(const NeXus::NXEntry &, const API::MatrixWorkspace_sptr &);
  std::vector<double> prepareAxes(const NeXus::NXEntry &);

  API::MatrixWorkspace_sptr convertSpectrumAxis(API::MatrixWorkspace_sptr);
  API::MatrixWorkspace_sptr transposeMonochromatic(const API::MatrixWorkspace_sptr &);

  size_t m_numberOfChannels; // number of channels data
  size_t m_acquisitionMode;  // acquisition mode of measurement, 0 -
                             // monochromatic, 1 - TOF

  std::string m_instName; ///< instrument name to load the IDF
  std::string m_fileName; ///< file name to load

  double m_wavelength; // wavelength value is read from the YIG IPF

  std::vector<API::MatrixWorkspace_sptr> m_outputWorkspaceGroup; ///< vector with output workspaces
};

} // namespace DataHandling
} // namespace Mantid
