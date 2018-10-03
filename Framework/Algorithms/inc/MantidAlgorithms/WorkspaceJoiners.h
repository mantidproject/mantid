// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_WORKSPACEJOINERS_H_
#define MANTID_ALGORITHMS_WORKSPACEJOINERS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {
/** A base class to hold code common to two algorithms that bolt two workspaces
    together spectra-wise - ConjoinWorkspaces & AppendSpectra.
    The main difference between these two is that the latter has an
   OutputWorkspace
    property into which the result is stored, whereas the former puts the result
    into a workspace with the same name as the first input workspace.
  */
class DLLExport WorkspaceJoiners : public API::Algorithm {
public:
  WorkspaceJoiners();
  ~WorkspaceJoiners() override;

  const std::string category() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Join two workspaces together by appending their spectra.";
  }

protected:
  API::MatrixWorkspace_sptr execWS2D(const API::MatrixWorkspace &ws1,
                                     const API::MatrixWorkspace &ws2);
  DataObjects::EventWorkspace_sptr
  execEvent(const DataObjects::EventWorkspace &eventWs1,
            const DataObjects::EventWorkspace &eventWs2);
  using Mantid::API::Algorithm::validateInputs;
  void validateInputs(const API::MatrixWorkspace &ws1,
                      const API::MatrixWorkspace &ws2, const bool checkBinning);
  void getMinMax(const API::MatrixWorkspace &ws, specnum_t &min,
                 specnum_t &max);

  /// Abstract method to be implemented in concrete algorithm classes
  virtual void fixSpectrumNumbers(const API::MatrixWorkspace &ws1,
                                  const API::MatrixWorkspace &ws2,
                                  API::MatrixWorkspace &output) = 0;

  API::Progress *m_progress; ///< Progress reporting object
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_WORKSPACEJOINERS_H_ */
