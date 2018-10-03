// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CONVERTTOHISTOGRAM_H_
#define MANTID_ALGORITHMS_CONVERTTOHISTOGRAM_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/XDataConverter.h"

namespace Mantid {
namespace Algorithms {
/**
  Converts a histogram workspace to point data by simply taking the centre point
  of the bin
  as the new point on the X axis

  @author Martyn Gigg, Tessella plc
  @date 2010-12-21
*/
class DLLExport ConvertToHistogram : public XDataConverter {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "ConvertToHistogram"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts a workspace containing point data into one containing "
           "histograms.";
  }
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertToPointData"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Axes"; }

private:
  /// Returns true if the algorithm needs to be run.
  bool
  isProcessingRequired(const API::MatrixWorkspace_sptr inputWS) const override;
  /// Returns the size of the new X vector
  std::size_t getNewXSize(const std::size_t ySize) const override;
  /// Calculate the X point values. Implement in an inheriting class.
  Kernel::cow_ptr<HistogramData::HistogramX> calculateXPoints(
      Kernel::cow_ptr<HistogramData::HistogramX> inputX) const override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTTOHISTOGRAM_H_*/
