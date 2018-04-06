#ifndef MANTID_CRYSTAL_FILTERPEAKS_H_
#define MANTID_CRYSTAL_FILTERPEAKS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/** FilterPeaks : Filter a peaks workspace based on a set number of queries to
  provide a new, filtered peaks workspace.

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport FilterPeaks : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Filters the peaks in a peaks workspace based upon the valur of a "
           "chosen variable.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CreatePeaksWorkspace"};
  }
  const std::string category() const override;

private:
  /// Typedef for the function to get the variable we're filtering against
  using FilterFunction = std::function<double(const Geometry::IPeak &)>;

  /// Override for algorithm init
  void init() override;
  /// Override for algorithm exec
  void exec() override;

  /// Get a function to filter peaks by
  FilterFunction
  getFilterVariableFunction(const std::string &filterVariable) const;

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
  void filterPeaks(const Mantid::API::IPeaksWorkspace &inputWS,
                   Mantid::API::IPeaksWorkspace &filteredWS,
                   const FilterFunction &filterFunction,
                   const double filterValue) {
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

#endif /* MANTID_CRYSTAL_FILTERPEAKS_H_ */
