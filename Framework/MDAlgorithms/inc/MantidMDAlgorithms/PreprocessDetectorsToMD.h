// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {
/**
This is helper algorithm used to preprocess detector's positions namely
to perform generic part of the transformation from a matrix workspace of a real
instrument to
physical MD workspace of an experimental results (e.g Q-space).
*/
class MANTID_MDALGORITHMS_DLL PreprocessDetectorsToMD : public API::Algorithm {
public:
  PreprocessDetectorsToMD();
  virtual ~PreprocessDetectorsToMD() = default;

  /// Algorithm's name for identification
  const std::string name() const override { return "PreprocessDetectorsToMD"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Its a helper algorithm, used to make common part of transformation "
           "from real to reciprocal space. "
           "It is used by :ref:`algm-ConvertToMD` and "
           ":ref:`algm-ConvertToMDMinMaxLocal` algorithms to save time spent "
           "on this transformation "
           "when the algorithm used multiple times for multiple measurements "
           "on the same instrument.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override { return "MDAlgorithms\\Utility"; }

private:
  void init() override;
  void exec() override;
  /// the variable specifies if one needs to calculate efixed for detectors
  /// (make sense for indirect instruments)
  bool m_getEFixed;
  /// the variable specifies if one needs to return the state of detector mask
  /// e.g if the detector is masked
  bool m_getIsMasked;

protected: // for testing
  void processDetectorsPositions(const API::MatrixWorkspace_const_sptr &inputWS,
                                 DataObjects::TableWorkspace_sptr &targWS);
  void buildFakeDetectorsPositions(const API::MatrixWorkspace_const_sptr &inputWS,
                                   DataObjects::TableWorkspace_sptr &targWS);
  void updateMasksState(const API::MatrixWorkspace_const_sptr &inputWS, DataObjects::TableWorkspace_sptr &targWS);
  // build a table workspace corresponding to the input matrix workspace
  std::shared_ptr<DataObjects::TableWorkspace> createTableWorkspace(const API::MatrixWorkspace_const_sptr &inputWS);
  bool isDetInfoLost(const Mantid::API::MatrixWorkspace_const_sptr &inWS2D) const;
  // helper function to get efixed if it is there or not;
  double getEi(const API::MatrixWorkspace_const_sptr &inputWS) const;
};

} // namespace MDAlgorithms
} // namespace Mantid
