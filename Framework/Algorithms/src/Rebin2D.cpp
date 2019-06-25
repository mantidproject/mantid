// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/Rebin2D.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/FractionalRebinning.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/PolygonIntersection.h"
#include "MantidGeometry/Math/Quadrilateral.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Rebin2D)

using namespace API;
using namespace DataObjects;
using namespace Geometry;
using namespace Mantid::HistogramData;

//--------------------------------------------------------------------------
// Private methods
//--------------------------------------------------------------------------
/**
 * Initialize the algorithm's properties.
 */
void Rebin2D::init() {
  using API::WorkspaceProperty;
  using Kernel::ArrayProperty;
  using Kernel::Direction;
  using Kernel::PropertyWithValue;
  using Kernel::RebinParamsValidator;
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                        Direction::Input),
                  "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "An output workspace.");
  const std::string docString =
      "A comma separated list of first bin boundary, width, last bin boundary. "
      "Optionally "
      "this can be followed by a comma and more widths and last boundary "
      "pairs. "
      "Negative width values indicate logarithmic binning.";
  auto rebinValidator = boost::make_shared<RebinParamsValidator>();
  declareProperty(
      std::make_unique<ArrayProperty<double>>("Axis1Binning", rebinValidator),
      docString);
  declareProperty(
      std::make_unique<ArrayProperty<double>>("Axis2Binning", rebinValidator),
      docString);
  declareProperty(
      std::make_unique<PropertyWithValue<bool>>("UseFractionalArea", false,
                                                Direction::Input),
      "Flag to turn on the using the fractional area tracking RebinnedOutput "
      "workspace\n."
      "Default is false.");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("Transpose", false),
                  "Run the Transpose algorithm on the resulting matrix.");
}

/**
 * Execute the algorithm.
 */
void Rebin2D::exec() {
  // Information to form input grid
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const NumericAxis *oldAxis2 =
      dynamic_cast<API::NumericAxis *>(inputWS->getAxis(1));
  if (!oldAxis2) {
    throw std::invalid_argument(
        "Vertical axis is not a numeric axis, cannot rebin. "
        "If it is a spectra axis try running ConvertSpectrumAxis first.");
  }

  const auto &oldXEdges = inputWS->x(0);
  const size_t numXBins = inputWS->blocksize();
  const size_t numYBins = inputWS->getNumberHistograms();
  // This will convert plain NumericAxis to bin edges while
  // BinEdgeAxis will just return its edges.
  const std::vector<double> oldYEdges = oldAxis2->createBinBoundaries();

  // Output grid and workspace. Fills in the new X and Y bin vectors
  // MantidVecPtr newXBins;
  BinEdges newXBins(oldXEdges.size());
  BinEdges newYBins(oldXEdges.size());

  // Flag for using a RebinnedOutput workspace
  // NB. This is now redundant because if the input is a MatrixWorkspace,
  // useFractionArea=false is forced since there is no fractional area info.
  // But if the input is RebinnedOutput, useFractionalArea=true is forced to
  // give correct signal/errors. It is kept for compatibility with old scripts.
  bool useFractionalArea = getProperty("UseFractionalArea");
  auto inputHasFA = boost::dynamic_pointer_cast<const RebinnedOutput>(inputWS);
  // For MatrixWorkspace, only UseFractionalArea=False makes sense.
  if (useFractionalArea && !inputHasFA) {
    g_log.warning(
        "Fractional area tracking was requested but input workspace does "
        "not have calculated bin fractions. Assuming bins are exact "
        "(fractions are unity). The results may not be accurate if this "
        "workspace was previously rebinned.");
  }
  // For RebinnedOutput, should always use useFractionalArea to get the
  // correct signal and errors (so that weights of input ws is accounted for).
  if (inputHasFA && !useFractionalArea) {
    g_log.warning("Input workspace has bin fractions (e.g. from a "
                  "parallelpiped rebin like SofQW3). To give accurate results, "
                  "fractional area tracking has been turn on.");
    useFractionalArea = true;
  }

  auto outputWS =
      createOutputWorkspace(inputWS, newXBins, newYBins, useFractionalArea);
  auto outputRB = boost::dynamic_pointer_cast<RebinnedOutput>(outputWS);

  // Progress reports & cancellation
  const size_t nreports(static_cast<size_t>(numYBins));
  m_progress = std::make_unique<API::Progress>(this, 0.0, 1.0, nreports);

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS, *outputWS))
  for (int64_t i = 0; i < static_cast<int64_t>(numYBins); ++i) {
    PARALLEL_START_INTERUPT_REGION

    m_progress->report("Computing polygon intersections");
    const double vlo = oldYEdges[i];
    const double vhi = oldYEdges[i + 1];
    for (size_t j = 0; j < numXBins; ++j) {
      // For each input polygon test where it intersects with
      // the output grid and assign the appropriate weights of Y/E
      const double x_j = oldXEdges[j];
      const double x_jp1 = oldXEdges[j + 1];
      Quadrilateral inputQ(x_j, x_jp1, vlo, vhi);
      if (!useFractionalArea) {
        FractionalRebinning::rebinToOutput(std::move(inputQ), inputWS, i, j,
                                           *outputWS, newYBins.rawData());
      } else {
        FractionalRebinning::rebinToFractionalOutput(
            std::move(inputQ), inputWS, i, j, *outputRB, newYBins.rawData(),
            inputHasFA);
      }
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  if (useFractionalArea) {
    outputRB->finalize(true, true);
  }

  FractionalRebinning::normaliseOutput(outputWS, inputWS, m_progress.get());

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
Rebin2D::createOutputWorkspace(const MatrixWorkspace_const_sptr &parent,
                               BinEdges &newXBins, BinEdges &newYBins,
                               const bool useFractionalArea) const {
  using Kernel::VectorHelper::createAxisFromRebinParams;

  auto &newY = newYBins.mutableRawData();
  // First create the two sets of bin boundaries
  static_cast<void>(createAxisFromRebinParams(getProperty("Axis1Binning"),
                                              newXBins.mutableRawData()));
  const int newYSize =
      createAxisFromRebinParams(getProperty("Axis2Binning"), newY);
  // and now the workspace
  HistogramData::BinEdges binEdges(newXBins);
  MatrixWorkspace_sptr outputWS;
  if (!useFractionalArea) {
    outputWS = create<MatrixWorkspace>(*parent, newYSize - 1, binEdges);
  } else {
    outputWS = create<RebinnedOutput>(*parent, newYSize - 1, binEdges);
  }
  auto verticalAxis = std::make_unique<BinEdgeAxis>(newY);
  // Meta data
  verticalAxis->unit() = parent->getAxis(1)->unit();
  verticalAxis->title() = parent->getAxis(1)->title();
  outputWS->replaceAxis(1, std::move(verticalAxis));

  return outputWS;
}

} // namespace Algorithms
} // namespace Mantid
