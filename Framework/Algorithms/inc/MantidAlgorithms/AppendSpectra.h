// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_APPENDSPECTRA_H_
#define MANTID_ALGORITHMS_APPENDSPECTRA_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/WorkspaceJoiners.h"

namespace Mantid {
namespace Algorithms {

/**
  Joins two partial, non-overlapping workspaces into one. Used in the situation
  where you
  want to load a raw file in two halves, process the data and then join them
  back into
  a single dataset.
  The input workspaces must come from the same instrument, have common units and
  bins
  and no detectors that contribute to spectra should overlap.

  @date 2012-02-20
*/
class DLLExport AppendSpectra : public WorkspaceJoiners {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ConjoinSpectra"};
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  void fixSpectrumNumbers(const API::MatrixWorkspace &ws1,
                          const API::MatrixWorkspace &ws2,
                          API::MatrixWorkspace &output) override;
  void combineLogs(const API::Run &lhs, const API::Run &rhs, API::Run &ans);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_APPENDSPECTRA_H_ */
