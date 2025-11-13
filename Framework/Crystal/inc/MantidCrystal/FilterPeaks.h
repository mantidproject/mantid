// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidCrystal/DllConfig.h"

namespace Mantid {
namespace Crystal {

/** FilterPeaks : Filter a peaks workspace based on a set number of queries to
  provide a new, filtered peaks workspace.
*/
class MANTID_CRYSTAL_DLL FilterPeaks final : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Filters the peaks in a peaks workspace based upon the value of a "
           "chosen variable.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"CreatePeaksWorkspace"}; }
  const std::string category() const override;

private:
  /// Typedef for the function to get the variable we're filtering against
  using FilterFunction = std::function<double(const Geometry::IPeak &)>;
  using FilterFunctionStr = std::function<std::string(const Geometry::IPeak &)>;

  /// Override for algorithm init
  void init() override;
  /// Override for algorithm exec
  void exec() override;

  /// Get a function to filter peaks by
  FilterFunction getFilterVariableFunction(const std::string &filterVariable) const;

  /** Filter the input peaks workspace using a comparator and a value selection
   * function
   *
   * The template parameter is a comparator function such as std::less to use to
   * compare the filterValue against the corresponding value of the peak.
   *
   * @param inputWS :: the input peaks workspace containing peaks to be filtered
   * @param filteredWS :: the output peaks workspace which will contain a subset
   * of the inputWS
   * @param filterFunction :: function extracting the value to filter on from
   * the peak object
   * @param filterValue :: the value to compare the filter variable against
   */
  template <typename Comparator>
  void filterPeaks(const Mantid::API::IPeaksWorkspace &inputWS, Mantid::API::IPeaksWorkspace &filteredWS,
                   const FilterFunction &filterFunction, const double filterValue) {
    Comparator operatorFunc;
    for (int i = 0; i < inputWS.getNumberPeaks(); ++i) {
      const Geometry::IPeak &currentPeak = inputWS.getPeak(i);
      const auto currentValue = filterFunction(currentPeak);

      if (operatorFunc(currentValue, filterValue))
        filteredWS.addPeak(currentPeak);
    }
  }

  /**
   * @brief Select peaks from the input peak workspace by checking a string
   *        value
   *
   * @tparam Comparator
   * @param inputWS :: the input peaks workspace containing peaks to be filtered
   * @param filteredWS :: the output peaks workspace which will contain a subset
   * of the inputWS
   * @param filterFunction :: function extracting the value to filter on from
   * the peak object
   * @param filterValue :: the string value to check
   */
  template <typename Comparator>
  void filterPeaksStr(const Mantid::API::IPeaksWorkspace &inputWS, Mantid::API::IPeaksWorkspace &filteredWS,
                      const FilterFunctionStr &filterFunction, const std::string &filterValue) {
    Comparator operatorFunc;
    for (int i = 0; i < inputWS.getNumberPeaks(); ++i) {
      const Geometry::IPeak &currentPeak = inputWS.getPeak(i);
      const auto currentValue = filterFunction(currentPeak);

      if (operatorFunc(currentValue, filterValue))
        filteredWS.addPeak(currentPeak);
    }
  }
};

} // namespace Crystal
} // namespace Mantid
