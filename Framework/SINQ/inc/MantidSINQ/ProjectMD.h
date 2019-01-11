// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/**
 * This is a little algorithm which can sum a MD dataset along one direction,
 * thereby yielding a dataset with one dimension less.
 *
 * Original contributor: Mark Koennecke: mark.koennecke@psi.ch
 *
 *
 */
#ifndef PROJECTMD_H_
#define PROJECTMD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidSINQ/DllConfig.h"

class MANTID_SINQ_DLL ProjectMD : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ProjectMD"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Sum a MDHistoWorkspace along a choosen dimension";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"CutMD", "BinMD"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "MDAlgorithms\\Slicing";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  void copyMetaData(Mantid::API::IMDHistoWorkspace_sptr inws,
                    Mantid::API::IMDHistoWorkspace_sptr outws);
  void sumData(Mantid::API::IMDHistoWorkspace_sptr inws,
               Mantid::API::IMDHistoWorkspace_sptr outws, int *sourceDim,
               int *targetDim, int targetDimCount, int dimNo, int start,
               int end, int currentDim);

  double getValue(Mantid::API::IMDHistoWorkspace_sptr ws, int *dim);
  void putValue(Mantid::API::IMDHistoWorkspace_sptr ws, int *dim, double val);
  unsigned int calcIndex(Mantid::API::IMDHistoWorkspace_sptr ws, int *dim);
};

#endif /*PROJECTMD_H_*/
