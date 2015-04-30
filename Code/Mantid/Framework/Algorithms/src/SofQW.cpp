//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <stdexcept>

#include "MantidAlgorithms/SofQW.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SofQW)

/// Energy to K constant
double SofQW::energyToK() {
  static const double energyToK = 8.0 * M_PI * M_PI *
                                  PhysicalConstants::NeutronMass *
                                  PhysicalConstants::meV * 1e-20 /
                                  (PhysicalConstants::h * PhysicalConstants::h);
  return energyToK;
}

using namespace Kernel;
using namespace API;

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
  alg.declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
                                              Direction::Input, wsValidator),
                      "Reduced data in units of energy transfer DeltaE.\nThe "
                      "workspace must contain histogram data and have common "
                      "bins across all spectra.");
  alg.declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name to use for the q-omega workspace.");
  alg.declareProperty(
      new ArrayProperty<double>("QAxisBinning",
                                boost::make_shared<RebinParamsValidator>()),
      "The bin parameters to use for the q axis (in the format used by the "
      ":ref:`algm-Rebin` algorithm).");

  std::vector<std::string> propOptions;
  propOptions.push_back("Direct");
  propOptions.push_back("Indirect");
  alg.declareProperty("EMode", "",
                      boost::make_shared<StringListValidator>(propOptions),
                      "The energy transfer analysis mode (Direct/Indirect)");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  alg.declareProperty("EFixed", 0.0, mustBePositive,
                      "The value of fixed energy: :math:`E_i` (EMode=Direct) "
                      "or :math:`E_f` (EMode=Indirect) (meV).\nMust be set "
                      "here if not available in the instrument definition.");
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
}

/** Creates the output workspace, setting the axes according to the input
 * binning parameters
 *  @param[in]  inputWorkspace The input workspace
 *  @param[in]  binParams The bin parameters from the user
 *  @param[out] newAxis        The 'vertical' axis defined by the given
 * parameters
 *  @return A pointer to the newly-created workspace
 */
API::MatrixWorkspace_sptr
SofQW::setUpOutputWorkspace(API::MatrixWorkspace_const_sptr inputWorkspace,
                            const std::vector<double> &binParams,
                            std::vector<double> &newAxis) {
  // Create vector to hold the new X axis values
  MantidVecPtr xAxis;
  xAxis.access() = inputWorkspace->readX(0);
  const int xLength = static_cast<int>(xAxis->size());
  // Create a vector to temporarily hold the vertical ('y') axis and populate
  // that
  const int yLength = static_cast<int>(
      VectorHelper::createAxisFromRebinParams(binParams, newAxis));

  // Create the output workspace
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(
      inputWorkspace, yLength - 1, xLength, xLength - 1);
  // Create a numeric axis to replace the default vertical one
  Axis *const verticalAxis = new BinEdgeAxis(newAxis);
  outputWorkspace->replaceAxis(1, verticalAxis);

  // Now set the axis values
  for (int i = 0; i < yLength - 1; ++i) {
    outputWorkspace->setX(i, xAxis);
  }

  // Set the axis units
  verticalAxis->unit() = UnitFactory::Instance().create("MomentumTransfer");
  verticalAxis->title() = "|Q|";

  // Set the X axis title (for conversion to MD)
  outputWorkspace->getAxis(0)->title() = "Energy transfer";

  return outputWorkspace;
}

} // namespace Algorithms
} // namespace Mantid
