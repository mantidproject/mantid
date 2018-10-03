// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_REBINRAGGED_H_
#define MANTID_ALGORITHMS_REBINRAGGED_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidKernel/System.h"
#include "MantidKernel/cow_ptr.h"
#include <map>

namespace Mantid {
namespace Algorithms {

/** ResampleX : TODO: DESCRIPTION
 */
class DLLExport ResampleX : public Algorithms::Rebin {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"RebinToWorkspace", "Rebin2D",           "Rebunch",
            "Regroup",          "RebinByPulseTimes", "RebinByTimeAtSample"};
  }
  const std::string category() const override;
  const std::string alias() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Resample the x-axis of the data with the requested number of "
           "points.";
  }

  /// MADE PUBLIC FOR TESTING ONLY - DO NOT USE
  double determineBinning(MantidVec &xValues, const double xmin,
                          const double xmax);
  /// MADE PUBLIC FOR TESTING ONLY - DO NOT USE
  void setOptions(const int numBins, const bool useLogBins, const bool isDist);

private:
  const std::string workspaceMethodName() const override {
    return "";
  } // Override the one from Rebin to ignore us

  void init() override;
  void exec() override;

  std::map<std::string, std::string> validateInputs() override;
  bool m_useLogBinning = true;
  bool m_preserveEvents = true;
  int m_numBins = 0;
  bool m_isDistribution = false;
  bool m_isHistogram = true;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REBINRAGGED_H_ */
