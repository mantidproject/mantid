// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace WorkflowAlgorithms {

/** LoadEventAndCompress : TODO: DESCRIPTION
 */
class DLLExport LoadEventAndCompress : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"LoadEventNexus", "CompressEvents"}; }
  const std::string category() const override;
  const std::string summary() const override;

protected:
  API::ITableWorkspace_sptr determineChunk(const std::string &filename) override;
  API::MatrixWorkspace_sptr loadChunk(const size_t rowIndex) override;

private:
  void init() override;
  void exec() override;

  API::ITableWorkspace_sptr m_chunkingTable;
  double m_filterBadPulses;
};

} // namespace WorkflowAlgorithms
} // namespace Mantid
