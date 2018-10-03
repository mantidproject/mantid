#include "MantidAlgorithms/MostLikelyMean.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/PropertyWithValue.h"

#include "boost/multi_array.hpp"

namespace Mantid {
namespace Algorithms {

using Mantid::Kernel::ArrayProperty;
using Mantid::Kernel::Direction;
using Mantid::Kernel::NullValidator;
using Mantid::Kernel::PropertyWithValue;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MostLikelyMean)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string MostLikelyMean::name() const { return "MostLikelyMean"; }

/// Algorithm's version for identification. @see Algorithm::version
int MostLikelyMean::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string MostLikelyMean::category() const { return "Arithmetic"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MostLikelyMean::summary() const {
  return "Computes the most likely mean of the array by minimizing the taxicab "
         "distance of the elements from the rest.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void MostLikelyMean::init() {
  auto nullValidator = boost::make_shared<NullValidator>();
  declareProperty(Kernel::make_unique<ArrayProperty<double>>(
                      "InputArray", nullValidator, Direction::Input),
                  "An input array.");
  declareProperty(Kernel::make_unique<PropertyWithValue<double>>(
                      "Output", 0., Direction::Output),
                  "The output (mean).");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void MostLikelyMean::exec() {
  const std::vector<double> input = getProperty("InputArray");
  const int size = static_cast<int>(input.size());
  boost::multi_array<double, 2> cov(boost::extents[size][size]);
  PARALLEL_FOR_IF(true)
  for (int i = 0; i < size; ++i) {
    for (int j = 0; j <= i; ++j) {
      double diff = sqrt(fabs(input[i] - input[j]));
      cov[i][j] = diff;
      cov[j][i] = diff;
    }
  }
  std::vector<double> sums(size);
  for (int i = 0; i < size; ++i) {
    sums[i] = std::accumulate(cov[i].begin(), cov[i].end(), 0.);
  }
  const auto minIndex = std::min_element(sums.cbegin(), sums.cend());
  setProperty("Output", input[std::distance(sums.cbegin(), minIndex)]);
}

} // namespace Algorithms
} // namespace Mantid
