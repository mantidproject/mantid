// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/**
 * This algorithm takes a MDHistoWorkspace and allows to select a slab out of
 * it which is storeed into the result workspace.
 *
 * Original contributor: Mark Koennecke: mark.koennecke@psi.ch
 *
 *
 */
#ifndef SLICEMDHISTO_H_
#define SLICEMDHISTO_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidSINQ/DllConfig.h"

class MANTID_SINQ_DLL SliceMDHisto : public Mantid::API::Algorithm {
public:
  /// Default constructor
  SliceMDHisto();
  /// Algorithm's name
  const std::string name() const override { return "SliceMDHisto"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Extracts a hyperslab of data from a MDHistoWorkspace";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"SliceMD", "IntegrateMDHistoWorkspace"};
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

  unsigned int m_rank;
  std::vector<int> m_dim;
  void cutData(Mantid::API::IMDHistoWorkspace_sptr inWS,
               Mantid::API::IMDHistoWorkspace_sptr outWS,
               Mantid::coord_t *sourceDim, Mantid::coord_t *targetDim,
               std::vector<int> start, std::vector<int> end, unsigned int dim);

  void copyMetaData(Mantid::API::IMDHistoWorkspace_sptr inws,
                    Mantid::API::IMDHistoWorkspace_sptr outws);
};

#endif /*SLICEMDHISTO_H_*/
