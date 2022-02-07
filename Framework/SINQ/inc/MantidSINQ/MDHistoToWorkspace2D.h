// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/**
 * This algorithm flattens a MDHistoWorkspace to a Workspace2D. Mantid has far
 more tools
 * to deal with W2D then for MD ones.
 *
 * Original contributor: Mark Koennecke: mark.koennecke@psi.ch
 *
 *
 */
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidSINQ/DllConfig.h"

class MANTID_SINQ_DLL MDHistoToWorkspace2D : public Mantid::API::Algorithm {
public:
  /// Default constructor
  MDHistoToWorkspace2D();
  /// Algorithm's name
  const std::string name() const override { return "MDHistoToWorkspace2D"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Flattens a n dimensional MDHistoWorkspace into a Workspace2D with "
           "many spectra";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"ConvertMDHistoToMatrixWorkspace"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "MDAlgorithms\\Transforms"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  size_t m_rank;
  size_t m_currentSpectra;
  size_t calculateNSpectra(const Mantid::API::IMDHistoWorkspace_sptr &inws);
  void recurseData(const Mantid::API::IMDHistoWorkspace_sptr &inWS, const Mantid::DataObjects::Workspace2D_sptr &outWS,
                   size_t currentDim, Mantid::coord_t *pos);

  void checkW2D(const Mantid::DataObjects::Workspace2D_sptr &outWS);

  void copyMetaData(const Mantid::API::IMDHistoWorkspace_sptr &inWS,
                    const Mantid::DataObjects::Workspace2D_sptr &outWS);
};
