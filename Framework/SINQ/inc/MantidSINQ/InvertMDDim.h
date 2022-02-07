// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/**
 * This Algorithms inverts the dimensions of a MD data set. The
 * application area is when fixing up MD workspaces which had to have the
 * dimensions inverted because they were delivered in C storage order.
 *
 * Original contributor: Mark Koennecke: mark.koennecke@psi.ch
 *
 *
 */
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidSINQ/DllConfig.h"

class MANTID_SINQ_DLL InvertMDDim : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "InvertMDDim"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Inverts dimensions of a MDHistoWorkspace"; }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"TransformMD"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "MDAlgorithms\\Transforms"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  void copyMetaData(const Mantid::API::IMDHistoWorkspace_sptr &inws, const Mantid::API::IMDHistoWorkspace_sptr &outws);
  void recurseDim(const Mantid::API::IMDHistoWorkspace_sptr &inWS, const Mantid::API::IMDHistoWorkspace_sptr &outWS,
                  int currentDim, int *idx, int rank);

  unsigned int calcIndex(const Mantid::API::IMDHistoWorkspace_sptr &ws, int *dim);
  unsigned int calcInvertedIndex(const Mantid::API::IMDHistoWorkspace_sptr &ws, int *dim);
};
