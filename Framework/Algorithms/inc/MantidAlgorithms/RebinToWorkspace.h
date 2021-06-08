// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {

namespace Algorithms {
/**
   Rebins a workspace so that the binning, for all its spectra, match that of
   the first spectrum
   of a second workspace.

   Required properties:
   <UL>
   <LI>WorkspaceToRebin - The workspace to rebin</LI>
   <LI>WorkspaceToMatch - The name of the workspace whose bin parameters are to
   be matched.</LI>
   <LI>OutputWorkspace - The name of the output workspace</LI>
   </UL>
*/

class MANTID_ALGORITHMS_DLL RebinToWorkspace : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "RebinToWorkspace"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Rebin a selected workspace to the same binning as a different "
           "workspace";
  }

  /// Algorithm's version
  int version() const override { return (1); }

  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Rebin"; }
  /// Algorithm's seeAlso
  const std::vector<std::string> seeAlso() const override { return {"Rebin"}; }

protected:
  Parallel::ExecutionMode
  getParallelExecutionMode(const std::map<std::string, Parallel::StorageMode> &storageModes) const override;

private:
  bool m_preserveEvents{true};
  bool m_isEvents{true};

  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  bool needToRebin(const API::MatrixWorkspace_sptr &left, const API::MatrixWorkspace_sptr &rght);
  void rebin(API::MatrixWorkspace_sptr &toRebin, API::MatrixWorkspace_sptr &toMatch);
  void histogram(API::MatrixWorkspace_sptr &toRebin, API::MatrixWorkspace_sptr &toMatch);
};
} // namespace Algorithms
} // namespace Mantid
