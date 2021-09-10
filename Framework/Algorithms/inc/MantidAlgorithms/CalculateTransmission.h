// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ParallelAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {
/** Calculates the transmission correction, as a function of wavelength, for a
   SANS
    instrument. Currently makes the assumption that the incident beam monitor's
    UDET is 2, while that of the transmission monitor is 3 (as for LOQ).

    Required Properties:
    <UL>
    <LI> SampleRunWorkspace  - The workspace containing the sample transmission
   run. </LI>
    <LI> DirectRunWorkspace  - The workspace containing the direct beam
   transmission run. </LI>
    <LI> OutputWorkspace     - The fitted transmission correction. </LI>
    </UL>

    Optional Properties:
    <UL>
    <LI> IncidentBeamMonitor - The UDET of the incident beam monitor (Default:
   2, as for LOQ). </LI>
    <LI> TransmissionMonitor - The UDET of the transmission monitor (Default: 3,
   as for LOQ). </LI>
    <LI> MinWavelength       - The minimum wavelength for the fit (Default: 2.2
   Angstroms). </LI>
    <LI> MaxWavelength       - The maximum wavelength for the fit (Default: 10
   Angstroms). </LI>
    <LI> FitMethod           - Whether to fit to the log of the transmission
   curve (the default) or directly (i.e. linearly). </LI>
    <LI> OutputUnfittedData  - If true (false is the default), will output an
   additional workspace
                               called [OutputWorkspace]_unfitted containing the
   unfitted transmission
                               correction. </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 22/01/2009
*/
class MANTID_ALGORITHMS_DLL CalculateTransmission : public API::ParallelAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "CalculateTransmission"; }
  /// Summary of algorithms purpose
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates the transmission correction, as a function of "
           "wavelength, for a SANS instrument.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"CalculateTransmissionBeamSpreader", "ApplyTransmissionCorrection"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "SANS;CorrectionFunctions\\TransmissionCorrections"; }

private:
  /// stores an estimate of the progress so far as a proportion (starts at zero
  /// goes to 1.0)
  mutable double m_done{0.0};

  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Pull out a single spectrum from a 2D workspace
  API::MatrixWorkspace_sptr extractSpectra(const API::MatrixWorkspace_sptr &ws, const std::vector<size_t> &indices);
  /// Returns a workspace with the evaulation of the fit to the calculated
  /// transmission fraction
  API::MatrixWorkspace_sptr fit(const API::MatrixWorkspace_sptr &raw, const std::vector<double> &rebinParams,
                                const std::string &fitMethod);
  /// Call the Linear fitting algorithm as a child algorithm
  API::MatrixWorkspace_sptr fitData(const API::MatrixWorkspace_sptr &WS, double &grad, double &offset);
  /// Call the Polynomial fitting algorithm as a child algorithm
  API::MatrixWorkspace_sptr fitPolynomial(const API::MatrixWorkspace_sptr &WS, int order,
                                          std::vector<double> &coeficients);
  /// Calls the rebin algorithm
  API::MatrixWorkspace_sptr rebin(const std::vector<double> &binParams, const API::MatrixWorkspace_sptr &ws);
  /// Outpus message to log if the detector at the given index is not a monitor
  /// in both input workspaces.
  void logIfNotMonitor(const API::MatrixWorkspace_sptr &sampleWS, const API::MatrixWorkspace_sptr &directWS,
                       size_t index);
};

} // namespace Algorithms
} // namespace Mantid
