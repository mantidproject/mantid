// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/cow_ptr.h"

#include <map>

namespace Mantid {
namespace DataHandling {

/** SaveSESANS : Save a workspace in the SESANS file format

  Require properties:
  <UL>
  <LI> InputWorkspace - The name of the workspace to save</LI>
  <LI> Filename - The path to save the file</LI>
  <LI> ThetaZMax - The angular acceptance in the encoding direction</LI>
  <LI> ThetaZMazUnit - Unit for theta_znmax</LI>
  <LI> ThetaYMax - The angular acceptance in the non-encoding direction</LI>
  <LI> ThetaYMazUnit - Unit for theta_ymax</LI>
  <LI> EchoConstant - The spin echo length, in nanometers,
                      probed by a 1A neutron</LI>
  </UL>

  @author Joseph Ramsay, ISIS
  @date 19/07/2017
*/
class MANTID_DATAHANDLING_DLL SaveSESANS final : public API::Algorithm {
public:
  const std::string name() const override;
  const std::string summary() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"LoadSESANS"}; }
  const std::string category() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  // Length of the longest attribute name in headers (+4 for readability in the
  // file)
  const int MAX_HDR_LENGTH = 23;
  // Tolerance to use when comparing two doubles for equality
  const double TOLERANCE = 1e-09;
  const std::vector<std::string> fileExtensions{".ses", ".SES", ".sesans", ".SESANS"};
  const std::vector<std::string> mandatoryDoubleProperties{"ThetaZMax", "ThetaYMax", "EchoConstant"};
  double m_sampleThickness = EMPTY_DBL();

  void init() override;
  void exec() override;

  void writeHeaders(std::ofstream &outfile, API::MatrixWorkspace_const_sptr &ws);
  void writeHeader(std::ofstream &outfile, const std::string &name, const std::string &value);

  std::vector<double> calculateSpinEchoLength(const HistogramData::Points &wavelength) const;
  std::vector<double> calculateDepolarisation(const HistogramData::HistogramY &yValues,
                                              const HistogramData::Points &wavelength) const;
  Mantid::MantidVec calculateError(const HistogramData::HistogramE &eValues, const HistogramData::HistogramY &yValues,
                                   const HistogramData::Points &wavelength) const;
};

} // namespace DataHandling
} // namespace Mantid
