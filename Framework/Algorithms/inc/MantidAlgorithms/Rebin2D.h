// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/RebinnedOutput.h"

namespace Mantid {
namespace Algorithms {

/**
Rebins both axes of a two-dimensional workspace to the given parameters.

@author Martyn Gigg, Tessella plc
*/
class MANTID_ALGORITHMS_DLL Rebin2D : public API::Algorithm {
public:
  ~Rebin2D() override = default;

  /// Algorithm's name for identification
  const std::string name() const override { return "Rebin2D"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Rebins both axes of a 2D workspace using the given parameters"; }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"Rebin", "SofQW"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Transforms\\Rebin"; }

protected:
  /// Progress reporter
  std::unique_ptr<API::Progress> m_progress;

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Setup the output workspace
  API::MatrixWorkspace_sptr createOutputWorkspace(const API::MatrixWorkspace_const_sptr &parent,
                                                  HistogramData::BinEdges &newXBins, HistogramData::BinEdges &newYBins,
                                                  const bool useFractionalArea) const;
};

} // namespace Algorithms
} // namespace Mantid
