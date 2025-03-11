// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/NexusDescriptor.h"

#include <H5Cpp.h>

namespace Mantid {
namespace DataHandling {

/** LoadILLLagrange : Loads nexus files from ILL instrument LAGRANGE.
 */
class MANTID_DATAHANDLING_DLL LoadILLLagrange : public API::IFileLoader<Kernel::NexusDescriptor> {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"LagrangeILLReduction"}; }
  const std::string category() const override;
  const std::string summary() const override;
  int confidence(Kernel::NexusDescriptor &) const override;
  LoadILLLagrange();

private:
  void init() override;
  void exec() override;

  void initWorkspace(const H5::DataSet &);

  void loadData();
  void loadMetaData();

  API::MatrixWorkspace_sptr m_outputWorkspace; ///< output workspace

  size_t m_nScans; ///< number of scans in the file
};

} // namespace DataHandling
} // namespace Mantid
