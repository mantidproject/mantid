// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REBIN2D_H_
#define MANTID_ALGORITHMS_REBIN2D_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/**
Rebins both axes of a two-dimensional workspace to the given parameters.

@author Martyn Gigg, Tessella plc
*/
class DLLExport Rebin2D : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "Rebin2D"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Rebins both axes of a 2D workspace using the given parameters";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"Rebin", "SofQW"};
  }
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
  API::MatrixWorkspace_sptr
  createOutputWorkspace(const API::MatrixWorkspace_const_sptr &parent,
                        HistogramData::BinEdges &newXBins,
                        HistogramData::BinEdges &newYBins,
                        const bool useFractionalArea) const;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REBIN2D_H_ */
