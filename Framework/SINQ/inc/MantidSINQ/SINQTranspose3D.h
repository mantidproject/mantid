// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/**
 * This algorithm takes a 3D MD workspace and performs certain axis transposings
 on it.
 * Essentially this fixes some mess which developed at SINQ when being to hasty
 taking the
 * EMBL detectors into operation.
 *
 * I am afraid that this code has grown to do something else: I suspect that
 Mantids MDHistoWorkspace
 * is acting in F77 storage order. This, then, is also fixed here.
 *
 * Original contributor: Mark Koennecke: mark.koennecke@psi.ch
 *
 *
 */
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/IMDHistoWorkspace_fwd.h"
#include "MantidSINQ/DllConfig.h"

class MANTID_SINQ_DLL SINQTranspose3D : public Mantid::API::Algorithm, public Mantid::API::DeprecatedAlgorithm {
public:
  /// Constructor
  SINQTranspose3D() { this->useAlgorithm("TransposeMD", 1); }
  /// Algorithm's name
  const std::string name() const override { return "Transpose3D"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "SINQ specific MD data reordering"; }
  const std::vector<std::string> seeAlso() const override { return {"TransposeMD", "Transpose"}; }
  /// Algorithm's version
  int version() const override { return (1); }
  /// Algorithm's category for identification
  const std::string category() const override { return "MDAlgorithms\\Transforms"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  void doYXZ(const Mantid::API::IMDHistoWorkspace_sptr &inws);
  void doXZY(const Mantid::API::IMDHistoWorkspace_sptr &inws);
  void doTRICS(const Mantid::API::IMDHistoWorkspace_sptr &inws);
  void doAMOR(const Mantid::API::IMDHistoWorkspace_sptr &inws);

  void copyMetaData(const Mantid::API::IMDHistoWorkspace_sptr &inws, const Mantid::API::IMDHistoWorkspace_sptr &outws);
};
