// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GroupToXResolution.h"

#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/WorkspaceHasDxValidator.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"

#include <boost/math/special_functions/pow.hpp>

namespace {
namespace Prop {
std::string const FRACTION{"FractionOfDx"};
std::string const INPUT_WS{"InputWorkspace"};
std::string const OUTPUT_WS{"OutputWorkspace"};
} // namespace Prop
constexpr double FWHM_GAUSSIAN_EQUIVALENT{0.68};
} // namespace

namespace Mantid {
namespace Algorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GroupToXResolution)

/// Algorithms name for identification. @see Algorithm::name
const std::string GroupToXResolution::name() const {
  return "GroupToXResolution";
}

/// Algorithm's version for identification. @see Algorithm::version
int GroupToXResolution::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string GroupToXResolution::category() const {
  return "Transforms\\Rebin";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string GroupToXResolution::summary() const {
  return "Groups points within intervals given by the Dx into single points";
}

/** Initialize the algorithm's properties.
 */
void GroupToXResolution::init() {
  auto inputValidator = boost::make_shared<Kernel::CompositeValidator>();
  inputValidator->add(boost::make_shared<API::WorkspaceHasDxValidator>());
  constexpr bool acceptHistograms{false};
  inputValidator->add(
      boost::make_shared<API::HistogramValidator>(acceptHistograms));
  declareProperty(
      std::make_unique<API::WorkspaceProperty<>>(
          Prop::INPUT_WS, "", Kernel::Direction::Input, inputValidator),
      "An input workspace with Dx values.");
  declareProperty(std::make_unique<API::WorkspaceProperty<>>(
                      Prop::OUTPUT_WS, "", Kernel::Direction::Output),
                  "The grouped workspace.");
  auto positive = boost::make_shared<Kernel::BoundedValidator<double>>();
  positive->setLower(0.);
  positive->setLowerExclusive(true);
  declareProperty(Prop::FRACTION, 0.2, positive,
                  "A fraction of Dx to group the points to.");
}

std::map<std::string, std::string> GroupToXResolution::validateInputs() {
  std::map<std::string, std::string> issues;
  API::MatrixWorkspace_const_sptr inWS = getProperty(Prop::INPUT_WS);
  if (inWS->getNumberHistograms() != 1) {
    issues[Prop::INPUT_WS] =
        "The workspace should contain only a single histogram.";
  }
  return issues;
}

/** Execute the algorithm.
 */
void GroupToXResolution::exec() {
  using boost::math::pow;
  API::MatrixWorkspace_const_sptr inWS = getProperty(Prop::INPUT_WS);
  double const groupingFraction = getProperty(Prop::FRACTION);
  HistogramData::Histogram h(HistogramData::Histogram::XMode::Points,
                             HistogramData::Histogram::YMode::Counts);
  auto const &inXs = inWS->x(0);
  auto const &inYs = inWS->y(0);
  auto const &inEs = inWS->e(0);
  auto const &inDxs = inWS->dx(0);
  std::vector<double> outXs;
  outXs.reserve(inXs.size());
  std::vector<double> outYs;
  outYs.reserve(inXs.size());
  std::vector<double> outEs;
  outEs.reserve(inXs.size());
  std::vector<double> outDxs;
  outDxs.reserve(inXs.size());
  size_t pointIndex{0};
  double begin = inXs.front();
  while (true) {
    auto const Dx = inDxs[pointIndex];
    if (Dx <= 0.) {
      throw std::out_of_range("Nonpositive DX value in the workspace.");
    }
    auto const width = groupingFraction * Dx;
    auto const end = inXs[pointIndex] + width;
    auto const beginXIterator =
        std::lower_bound(inXs.cbegin(), inXs.cend(), begin);
    auto const endXIterator = std::lower_bound(inXs.cbegin(), inXs.cend(), end);
    auto const pickSize =
        static_cast<size_t>(std::distance(beginXIterator, endXIterator));
    if (pickSize > 0) {
      auto const offset =
          static_cast<size_t>(std::distance(inXs.cbegin(), beginXIterator));
      double xSum{0.};
      double ySum{0.};
      double eSquaredSum{0.};
      for (size_t pickIndex = offset; pickIndex < offset + pickSize;
           ++pickIndex) {
        xSum += inXs[pickIndex];
        ySum += inYs[pickIndex];
        eSquaredSum += pow<2>(inEs[pickIndex]);
      }
      outXs.emplace_back(xSum / static_cast<double>(pickSize));
      outYs.emplace_back(ySum / static_cast<double>(pickSize));
      outEs.emplace_back(std::sqrt(eSquaredSum) /
                         static_cast<double>(pickSize));
      auto const groupedXWidth = *std::prev(endXIterator) - *beginXIterator;
      outDxs.emplace_back(
          std::sqrt(pow<2>(inDxs[pointIndex]) +
                    pow<2>(FWHM_GAUSSIAN_EQUIVALENT * groupedXWidth)));
    } else {
      throw std::out_of_range(
          "Failed to group. Is the X data sorted in ascending order?");
    }
    begin = end;
    if (begin > inXs.back()) {
      break;
    }
    pointIndex += pickSize;
  }
  HistogramData::HistogramBuilder constructionYard;
  constructionYard.setX(std::move(outXs));
  constructionYard.setY(std::move(outYs));
  constructionYard.setE(std::move(outEs));
  constructionYard.setDx(std::move(outDxs));
  constructionYard.setDistribution(false);
  API::MatrixWorkspace_sptr outWS =
      DataObjects::create<DataObjects::Workspace2D>(*inWS,
                                                    constructionYard.build());
  setProperty(Prop::OUTPUT_WS, outWS);
}

} // namespace Algorithms
} // namespace Mantid
