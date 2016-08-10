//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/Rebin2D.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/FractionalRebinning.h"
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/PolygonIntersection.h"
#include "MantidGeometry/Math/Quadrilateral.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/VectorHelper.h"

#include <boost/math/special_functions/fpclassify.hpp>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Rebin2D)

using namespace API;
using namespace DataObjects;
using namespace Geometry;
using Kernel::V2D;

//--------------------------------------------------------------------------
// Private methods
//--------------------------------------------------------------------------
/**
 * Initialize the algorithm's properties.
 */
void Rebin2D::init() {
  using Kernel::ArrayProperty;
  using Kernel::Direction;
  using Kernel::PropertyWithValue;
  using Kernel::RebinParamsValidator;
  using API::WorkspaceProperty;
  declareProperty(Kernel::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                           Direction::Input),
                  "An input workspace.");
  declareProperty(Kernel::make_unique<WorkspaceProperty<>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
  const std::string docString =
      "A comma separated list of first bin boundary, width, last bin boundary. "
      "Optionally "
      "this can be followed by a comma and more widths and last boundary "
      "pairs. "
      "Negative width values indicate logarithmic binning.";
  auto rebinValidator = boost::make_shared<RebinParamsValidator>();
  declareProperty(Kernel::make_unique<ArrayProperty<double>>("Axis1Binning",
                                                             rebinValidator),
                  docString);
  declareProperty(Kernel::make_unique<ArrayProperty<double>>("Axis2Binning",
                                                             rebinValidator),
                  docString);
  declareProperty(
      Kernel::make_unique<PropertyWithValue<bool>>("UseFractionalArea", false,
                                                   Direction::Input),
      "Flag to turn on the using the fractional area tracking RebinnedOutput "
      "workspace\n."
      "Default is false.");
  declareProperty(
      Kernel::make_unique<PropertyWithValue<bool>>("Transpose", false),
      "Run the Transpose algorithm on the resulting matrix.");
}

/**
 * Execute the algorithm.
 */
void Rebin2D::exec() {
  // Information to form input grid
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  NumericAxis *oldAxis2 = dynamic_cast<API::NumericAxis *>(inputWS->getAxis(1));
  if (!oldAxis2) {
    throw std::invalid_argument(
        "Vertical axis is not a numeric axis, cannot rebin. "
        "If it is a spectra axis try running ConvertSpectrumAxis first.");
  }

  const MantidVec &oldXEdges = inputWS->readX(0);
  const size_t numXBins = inputWS->blocksize();
  const size_t numYBins = inputWS->getNumberHistograms();
  std::vector<double> oldYEdges;
  if (numYBins == oldAxis2->length()) {
    // Pt data on axis 2, create bins
    oldYEdges = oldAxis2->createBinBoundaries();
  } else {
    oldYEdges.resize(oldAxis2->length());
    for (size_t i = 0; i < oldAxis2->length(); ++i) {
      oldYEdges[i] = (*oldAxis2)(i);
    }
  }

  // Output grid and workspace. Fills in the new X and Y bin vectors
  MantidVecPtr newXBins;
  MantidVec newYBins;

  // Flag for using a RebinnedOutput workspace
  bool useFractionalArea = getProperty("UseFractionalArea");
  MatrixWorkspace_sptr outputWS = createOutputWorkspace(
      inputWS, newXBins.access(), newYBins, useFractionalArea);
  if (useFractionalArea &&
      !boost::dynamic_pointer_cast<RebinnedOutput>(outputWS)) {
    g_log.warning("Fractional area tracking requires the input workspace to "
                  "contain calculated bin fractions from a parallelpiped rebin "
                  "like SofQW"
                  "Continuing without fractional area tracking");
    useFractionalArea = false;
  }

  // Progress reports & cancellation
  const size_t nreports(static_cast<size_t>(inputWS->getNumberHistograms() *
                                            inputWS->blocksize()));
  m_progress = boost::shared_ptr<API::Progress>(
      new API::Progress(this, 0.0, 1.0, nreports));

  PARALLEL_FOR2(inputWS, outputWS)
  for (int64_t i = 0; i < static_cast<int64_t>(numYBins);
       ++i) // signed for openmp
  {
    PARALLEL_START_INTERUPT_REGION

    const double vlo = oldYEdges[i];
    const double vhi = oldYEdges[i + 1];
    for (size_t j = 0; j < numXBins; ++j) {
      m_progress->report("Computing polygon intersections");
      // For each input polygon test where it intersects with
      // the output grid and assign the appropriate weights of Y/E
      const double x_j = oldXEdges[j];
      const double x_jp1 = oldXEdges[j + 1];
      Quadrilateral inputQ = Quadrilateral(x_j, x_jp1, vlo, vhi);
      if (!useFractionalArea) {
        FractionalRebinning::rebinToOutput(inputQ, inputWS, i, j, outputWS,
                                           newYBins);
      } else {
        FractionalRebinning::rebinToFractionalOutput(
            inputQ, inputWS, i, j,
            boost::dynamic_pointer_cast<RebinnedOutput>(outputWS), newYBins);
      }
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  if (useFractionalArea) {
    boost::dynamic_pointer_cast<RebinnedOutput>(outputWS)->finalize();
  }

  FractionalRebinning::normaliseOutput(outputWS, inputWS, m_progress);

  bool Transpose = this->getProperty("Transpose");
  if (Transpose) {
    IAlgorithm_sptr alg = this->createChildAlgorithm("Transpose", 0.9, 1.0);
    alg->setProperty("InputWorkspace", outputWS);
    alg->setPropertyValue("OutputWorkspace", "__anonymous");
    alg->execute();
    outputWS = alg->getProperty("OutputWorkspace");
  }

  setProperty("OutputWorkspace", outputWS);
}

/**
 * Setup the output workspace
 * @param parent :: A pointer to the input workspace
 * @param newXBins [out] :: An output vector to be filled with the new X bin
 * boundaries
 * @param newYBins [out] :: An output vector to be filled with the new Y bin
 * boundaries
 * @param useFractionalArea :: use a RebinnedOutput workspace
 * @return A pointer to the output workspace
 */
MatrixWorkspace_sptr
Rebin2D::createOutputWorkspace(MatrixWorkspace_const_sptr parent,
                               MantidVec &newXBins, MantidVec &newYBins,
                               const bool useFractionalArea) const {
  using Kernel::VectorHelper::createAxisFromRebinParams;
  // First create the two sets of bin boundaries
  const int newXSize =
      createAxisFromRebinParams(getProperty("Axis1Binning"), newXBins);
  const int newYSize =
      createAxisFromRebinParams(getProperty("Axis2Binning"), newYBins);
  // and now the workspace
  MatrixWorkspace_sptr outputWS;
  if (!useFractionalArea) {
    outputWS = WorkspaceFactory::Instance().create(parent, newYSize - 1,
                                                   newXSize, newXSize - 1);
  } else {
    outputWS = WorkspaceFactory::Instance().create(
        "RebinnedOutput", newYSize - 1, newXSize, newXSize - 1);
    WorkspaceFactory::Instance().initializeFromParent(parent, outputWS, true);
  }
  Axis *const verticalAxis = new BinEdgeAxis(newYBins);
  // Meta data
  verticalAxis->unit() = parent->getAxis(1)->unit();
  verticalAxis->title() = parent->getAxis(1)->title();
  outputWS->replaceAxis(1, verticalAxis);

  HistogramData::BinEdges binEdges(newXBins);
  // Now set the axis values
  for (size_t i = 0; i < static_cast<size_t>(newYSize - 1); ++i) {
    outputWS->setBinEdges(i, binEdges);
  }

  return outputWS;
}

} // namespace Algorithms
} // namespace Mantid
