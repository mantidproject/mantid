// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <utility>

#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/NexusDescriptor.h"
#include "MantidNexus/NexusClasses.h"

namespace Mantid {
namespace DataHandling {

/** LoadILLPolarizedDiffraction : Loads ILL polarized diffraction nexus files.

  @date 15/05/20
*/
class MANTID_DATAHANDLING_DLL LoadILLPolarizedDiffraction
    : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"LoadNexus"};
  }
  const std::string category() const override;
  const std::string summary() const override;
  int confidence(Kernel::NexusDescriptor &descriptor) const override;
  LoadILLPolarizedDiffraction();

private:
  void init() override;
  void exec() override;

  std::string getInstrumentFilePath(const std::string &) const;
  bool calibrationFileExists(const std::string &filename) const;

  API::MatrixWorkspace_sptr initStaticWorkspace();

  void loadData();
  void loadMetaData(API::MatrixWorkspace_sptr &);
  void loadInstrument(API::MatrixWorkspace_sptr &);
  std::vector<double> loadTwoThetaDetectors(const NeXus::NXEntry &, int);
  void moveTwoTheta(const NeXus::NXEntry &, API::MatrixWorkspace_sptr &);

  size_t m_numberOfChannels; // number of channels data

  std::string m_instName;            ///< instrument name to load the IDF
  std::set<std::string> m_instNames; ///< supported instruments
  std::string m_filename;            ///< file name to load

  LoadHelper m_loadHelper;                    ///< a helper for metadata
  API::WorkspaceGroup_sptr m_outputWorkspace; ///< output workspace
};

} // namespace DataHandling
} // namespace Mantid
