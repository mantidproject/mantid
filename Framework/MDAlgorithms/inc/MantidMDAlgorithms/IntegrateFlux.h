// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {

namespace API {
class MatrixWorkspace;
}

namespace DataObjects {
class EventWorkspace;
}

namespace MDAlgorithms {

/** Algorithm IntegrateFlux.

  Calculates indefinite integral of the spectra in the input workspace sampled
  at a regular grid.
*/
class DLLExport IntegrateFlux : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"Integration"}; }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  std::shared_ptr<API::MatrixWorkspace> createOutputWorkspace(const API::MatrixWorkspace &inputWS, size_t nX) const;
  void integrateSpectra(const API::MatrixWorkspace &inputWS, API::MatrixWorkspace &integrWS) const;
  template <class EventType>
  void integrateSpectraEvents(const DataObjects::EventWorkspace &inputWS, API::MatrixWorkspace &integrWS) const;
  void integrateSpectraMatrix(const API::MatrixWorkspace &inputWS, API::MatrixWorkspace &integrWS) const;
  void integrateSpectraHistograms(const API::MatrixWorkspace &inputWS, API::MatrixWorkspace &integrWS) const;
  void integrateSpectraPointData(const API::MatrixWorkspace &inputWS, API::MatrixWorkspace &integrWS) const;

  size_t getMaxNumberOfPoints(const API::MatrixWorkspace &inputWS) const;
};

} // namespace MDAlgorithms
} // namespace Mantid
