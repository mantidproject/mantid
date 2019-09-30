// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <stdexcept>

#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/SpectraAxisValidator.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/SofQW.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/RebinParamsValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SofQW)

using namespace API;
using namespace DataObjects;
using namespace Kernel;

/**
 * @return A summary of the algorithm
 */
const std::string SofQW::summary() const {
  return "Computes S(Q,w) using a either centre point or parallel-piped "
         "rebinning.\n"
         "The output from each method is:\n"
         "CentrePoint - centre-point rebin that takes no account of pixel "
         "curvature or area overlap\n\n"
         "Polygon - parallel-piped rebin, outputting a weighted-sum of "
         "overlapping polygons\n\n"
         "NormalisedPolygon - parallel-piped rebin, outputting a weighted-sum "
         "of "
         "overlapping polygons normalised by the fractional area of each "
         "overlap";
}

/**
 * Create the input properties
 */
void SofQW::init() {
  createCommonInputProperties(*this);

  // Add the Method property to control which algorithm is called
  const char *methodOptions[] = {"Centre", "Polygon", "NormalisedPolygon"};
  this->declareProperty(
      "Method", "Centre",
      boost::make_shared<StringListValidator>(
          std::vector<std::string>(methodOptions, methodOptions + 3)),
      "Defines the method used to compute the output.");
}

/**
 * Create the common set of input properties for the given algorithm
 * @param alg An algorithm object
 */
void SofQW::createCommonInputProperties(API::Algorithm &alg) {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("DeltaE");
  wsValidator->add<SpectraAxisValidator>();
  wsValidator->add<CommonBinsValidator>();
  wsValidator->add<HistogramValidator>();
  wsValidator->add<InstrumentValidator>();
  alg.declareProperty(std::make_unique<WorkspaceProperty<>>(
                          "InputWorkspace", "", Direction::Input, wsValidator),
                      "Reduced data in units of energy transfer DeltaE.\nThe "
                      "workspace must contain histogram data and have common "
                      "bins across all spectra.");
  alg.declareProperty(std::make_unique<WorkspaceProperty<>>(
                          "OutputWorkspace", "", Direction::Output),
                      "The name to use for the q-omega workspace.");
  alg.declareProperty(
      std::make_unique<ArrayProperty<double>>(
          "QAxisBinning", boost::make_shared<RebinParamsValidator>()),
      "The bin parameters to use for the q axis (in the format used by the "
      ":ref:`algm-Rebin` algorithm).");

  const std::vector<std::string> propOptions{"Direct", "Indirect"};
  alg.declareProperty("EMode", "",
                      boost::make_shared<StringListValidator>(propOptions),
                      "The energy transfer analysis mode (Direct/Indirect)");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  alg.declareProperty("EFixed", 0.0, mustBePositive,
                      "The value of fixed energy: :math:`E_i` (EMode=Direct) "
                      "or :math:`E_f` (EMode=Indirect) (meV).\nMust be set "
                      "here if not available in the instrument definition.");
  alg.declareProperty("ReplaceNaNs", false,
                      "If true, all NaN values in the output workspace are "
                      "replaced using the ReplaceSpecialValues algorithm.",
                      Direction::Input);
  alg.declareProperty(
      std::make_unique<ArrayProperty<double>>(
          "EAxisBinning", boost::make_shared<RebinParamsValidator>(true)),
      "The bin parameters to use for the E axis (optional, in the format "
      "used by the :ref:`algm-Rebin` algorithm).");
  alg.declareProperty(
      std::make_unique<WorkspaceProperty<TableWorkspace>>(
          "DetectorTwoThetaRanges", "", Direction::Input,
          PropertyMode::Optional),
      "A table workspace use by SofQWNormalisedPolygon containing a 'Detector "
      "ID' column as well as 'Min two theta' and 'Max two theta' columns "
      "listing the detector's min and max scattering angles in radians.");
}

void SofQW::exec() {
  // Find the approopriate algorithm
  std::string method = this->getProperty("Method");
  std::string child = "SofQW" + method;

  // Setup and run
  Algorithm_sptr childAlg = boost::dynamic_pointer_cast<Algorithm>(
      createChildAlgorithm(child, 0.0, 1.0));
  // This will add the Method property to the child algorithm but it will be
  // ignored anyway...
  childAlg->copyPropertiesFrom(*this);
  childAlg->execute();

  MatrixWorkspace_sptr outputWS = childAlg->getProperty("OutputWorkspace");

  this->setProperty("OutputWorkspace", outputWS);

  // Progress reports & cancellation
  MatrixWorkspace_const_sptr inputWorkspace = getProperty("InputWorkspace");
  const size_t nHistos = inputWorkspace->getNumberHistograms();
  auto m_progress = std::make_unique<Progress>(this, 0.0, 1.0, nHistos);
  m_progress->report("Creating output workspace");
}

} // namespace Algorithms
} // namespace Mantid
