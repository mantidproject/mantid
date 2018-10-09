// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
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
#ifndef INVERTMDDIM_H_
#define INVERTMDDIM_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidSINQ/DllConfig.h"

class MANTID_SINQ_DLL InvertMDDim : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "InvertMDDim"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Inverts dimensions of a MDHistoWorkspace";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"TransformMD"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "MDAlgorithms\\Transforms";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  void copyMetaData(Mantid::API::IMDHistoWorkspace_sptr inws,
                    Mantid::API::IMDHistoWorkspace_sptr outws);
  void recurseDim(Mantid::API::IMDHistoWorkspace_sptr inWS,
                  Mantid::API::IMDHistoWorkspace_sptr outWS, int currentDim,
                  int *idx, int rank);

  unsigned int calcIndex(Mantid::API::IMDHistoWorkspace_sptr ws, int *dim);
  unsigned int calcInvertedIndex(Mantid::API::IMDHistoWorkspace_sptr ws,
                                 int *dim);
};

#endif /*INVERTMDDIM_H_*/
