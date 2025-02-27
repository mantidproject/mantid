// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SANSCollimationLengthEstimator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/V3D.h"
#include "boost/lexical_cast.hpp"

namespace {
Mantid::Kernel::Logger g_log("SANSCollimationLengthEstimator");

/**
 * Provide an string and check if it can be converted to a double
 * @param val: a value as a string
 * @returns true if it is convertible else false
 */
bool checkForDouble(const std::string &val) {
  auto isDouble = false;
  try {
    boost::lexical_cast<double>(val);
    isDouble = true;
  } catch (boost::bad_lexical_cast const &) {
  }
  return isDouble;
}
} // namespace

namespace Mantid::Algorithms {

using namespace Kernel;
using namespace API;

/**
 * Provide the collimation length which is associated with the instrument
 * @param workspace: the input workspace
 * @returns the collimation length
 */
double SANSCollimationLengthEstimator::provideCollimationLength(const Mantid::API::MatrixWorkspace_sptr &workspace) {
  // If the instrument does not have a correction specified then set the length
  // to 4
  const double defaultLColim = 4.0;
  auto collimationLengthID = "collimation-length-correction";

  if (!workspace->getInstrument()->hasParameter(collimationLengthID)) {
    g_log.error("Error in SANSCollimtionLengthEstimator: The instrument "
                "parameter file does not contain a collimation length "
                "correction,"
                "a default of 4 is provided. Please update the instrument "
                "parameter file.");
    return defaultLColim;
  }

  // Get the L1 length
  const V3D samplePos = workspace->getInstrument()->getSample()->getPos();
  const V3D sourcePos = workspace->getInstrument()->getSource()->getPos();
  const V3D SSD = samplePos - sourcePos;
  const double L1 = SSD.norm();

  auto collimationLengthCorrection = workspace->getInstrument()->getNumberParameter(collimationLengthID);

  if (workspace->getInstrument()->hasParameter("special-default-collimation-length-method")) {
    auto specialCollimationMethod =
        workspace->getInstrument()->getStringParameter("special-default-collimation-length-method");
    if (specialCollimationMethod[0] == "guide") {
      try {
        return getCollimationLengthWithGuides(workspace, L1, collimationLengthCorrection[0]);
      } catch (std::invalid_argument &ex) {
        g_log.notice() << ex.what();
        g_log.notice() << "SANSCollimationLengthEstimator: Not using any guides";
        return L1 - collimationLengthCorrection[0];
      }
    } else {
      throw std::invalid_argument("Error in SANSCollimationLengthEstimator: "
                                  "Unknown special collimation method.");
    }
  }
  return L1 - collimationLengthCorrection[0];
}

/**
 * This extraction strategy gets applied when guides are used to calculate the
 * collimation length. The instrument
 * parameter file contains the information about the number of guides to use.
 * The guide data itself is fetched
 * from the sample logs.
 * @param inOutWS: a matrix workspace
 * @param L1: the distance between sample and source
 * @param collimationLengthCorrection: The correction to get the collimation
 * length
 */
double SANSCollimationLengthEstimator::getCollimationLengthWithGuides(const MatrixWorkspace_sptr &inOutWS,
                                                                      const double L1,
                                                                      const double collimationLengthCorrection) const {
  auto lCollim = L1 - collimationLengthCorrection;

  // Make sure we have guide cutoffs
  if (!inOutWS->getInstrument()->hasParameter("guide-cutoff")) {
    throw std::invalid_argument("TOFSANSResolutionByPixel: Could not get a "
                                "GuideCutoff from the instrument");
  }

  // Make sure we have a defined number of guidess
  if (!inOutWS->getInstrument()->hasParameter("number-of-guides")) {
    throw std::invalid_argument("TOFSANSResolutionByPixel: Could not get the number of guides.");
  }

  // Make sure we have a guide increment specified
  if (!inOutWS->getInstrument()->hasParameter("guide-collimation-length-increment")) {
    throw std::invalid_argument("TOFSANSResolutionByPixel: Could not find a guide increment.");
  }

  auto numberOfGuides = static_cast<unsigned int>(inOutWS->getInstrument()->getNumberParameter("number-of-guides")[0]);
  auto guideIncrement = inOutWS->getInstrument()->getNumberParameter("guide-collimation-length-increment")[0];

  // Make sure that all guides are there. They are labelled as Guide1, Guide2,
  // Guide3, ...
  // The entry is a numeric TimeSeriesProperty or a numeric entry, if something
  // else then default
  std::vector<double> guideValues;
  for (unsigned int i = 1; i <= numberOfGuides; i++) {
    auto guideName = "Guide" + std::to_string(i);
    if (inOutWS->run().hasProperty(guideName)) {
      auto guideValue = getGuideValue(inOutWS->run().getProperty(guideName));
      guideValues.emplace_back(guideValue);
    } else {
      throw std::invalid_argument("TOFSANSResolutionByPixel: Mismatch between "
                                  "specified number of Guides and actual "
                                  "Guides.");
    }
  }

  auto guideCutoff = inOutWS->getInstrument()->getNumberParameter("guide-cutoff")[0];
  // Go through the guides and check in an alternate manner if the guide is
  // smaller
  // or larger than the cut off value. We start at the last guide and check that
  // it is
  // larger than the cutoff, the next one has to be smaller and so on. For
  // example in pseudocode
  // If Guide5 > 130: LCollim+=2.0 else break;
  // If Guide4 < 130: LCollim+=2.0 else break;
  // If Guide3 > 130: LCollim+=2.0 else break;
  // ...
  unsigned int largerSmallerCounter = 0;
  for (auto it = guideValues.rbegin(); it != guideValues.rend(); ++it) {
    bool guideIsLarger = largerSmallerCounter % 2 == 0;
    if (guideIsLarger && (*it > guideCutoff)) {
      lCollim += guideIncrement;
    } else if (!guideIsLarger && (*it < guideCutoff)) {
      lCollim += guideIncrement;
    } else {
      break;
    }
    largerSmallerCounter++;
  }
  return lCollim;
}

/**
 * Extracts the value of the guide
 * @param prop: a property
 * @returns the guide value
 */
double SANSCollimationLengthEstimator::getGuideValue(const Mantid::Kernel::Property *prop) const {
  if (auto timeSeriesProperty = dynamic_cast<const TimeSeriesProperty<double> *>(prop)) {
    return timeSeriesProperty->firstValue();
  } else if (auto doubleProperty = dynamic_cast<const PropertyWithValue<double> *>(prop)) {
    auto val = doubleProperty->value();
    if (checkForDouble(val)) {
      g_log.warning("SANSCollimationLengthEstimator: The Guide was not "
                    "recoginized as a TimeSeriesProperty, but rather as a "
                    "Numeric.");
      return boost::lexical_cast<double, std::string>(val);
    }
  }
  throw std::invalid_argument("TOFSANSResolutionByPixel: Unknown type for "
                              "Guides. Currently only Numeric and TimeSeries "
                              "are supported.");
}
} // namespace Mantid::Algorithms
