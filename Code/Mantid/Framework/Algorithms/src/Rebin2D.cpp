//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/Rebin2D.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/Quadrilateral.h"
#include "MantidGeometry/Math/LaszloIntersection.h"

#include <boost/math/special_functions/fpclassify.hpp>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Rebin2D)

using namespace API;
using namespace DataObjects;
using Geometry::ConvexPolygon;
using Geometry::Quadrilateral;
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
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "An input workspace.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "An output workspace.");
  const std::string docString =
      "A comma separated list of first bin boundary, width, last bin boundary. "
      "Optionally "
      "this can be followed by a comma and more widths and last boundary "
      "pairs. "
      "Negative width values indicate logarithmic binning.";
  auto rebinValidator = boost::make_shared<RebinParamsValidator>();
  declareProperty(new ArrayProperty<double>("Axis1Binning", rebinValidator),
                  docString);
  declareProperty(new ArrayProperty<double>("Axis2Binning", rebinValidator),
                  docString);
  declareProperty(
      new PropertyWithValue<bool>("UseFractionalArea", false, Direction::Input),
      "Flag to turn on the using the fractional area tracking RebinnedOutput "
      "workspace\n."
      "Default is false.");
  declareProperty(new PropertyWithValue<bool>("Transpose", false),
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

  this->useFractionalArea = getProperty("UseFractionalArea");
  MatrixWorkspace_sptr outputWS =
      createOutputWorkspace(inputWS, newXBins.access(), newYBins);
  if (this->useFractionalArea &&
      !boost::dynamic_pointer_cast<RebinnedOutput>(outputWS)) {
    g_log.warning("Fractional area tracking requires the input workspace to "
                  "contain calculated bin fractions from a parallelpiped rebin "
                  "like SofQW"
                  "Continuing without fractional area tracking");
    this->useFractionalArea = false;
  }

  // Progress reports & cancellation
  const size_t nreports(static_cast<size_t>(inputWS->getNumberHistograms() *
                                            inputWS->blocksize()));
  m_progress = boost::shared_ptr<API::Progress>(
      new API::Progress(this, 0.0, 1.0, nreports));

  // PARALLEL_FOR2(inputWS, outputWS)
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
      if (!this->useFractionalArea) {
        rebinToOutput(inputQ, inputWS, i, j, outputWS, newYBins);
      } else {
        rebinToFractionalOutput(
            inputQ, inputWS, i, j,
            boost::dynamic_pointer_cast<RebinnedOutput>(outputWS), newYBins);
      }
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  if (this->useFractionalArea) {
    boost::dynamic_pointer_cast<RebinnedOutput>(outputWS)->finalize();
  }
  normaliseOutput(outputWS, inputWS);

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
 * Rebin the input quadrilateral to the output grid
 * @param inputQ The input polygon
 * @param inputWS The input workspace containing the input intensity values
 * @param i The index in the vertical axis direction that inputQ references
 * @param j The index in the horizontal axis direction that inputQ references
 * @param outputWS A pointer to the output workspace that accumulates the data
 * @param verticalAxis A vector containing the output vertical axis bin
 * boundaries
 */
void Rebin2D::rebinToOutput(const Geometry::Quadrilateral &inputQ,
                            MatrixWorkspace_const_sptr inputWS, const size_t i,
                            const size_t j, MatrixWorkspace_sptr outputWS,
                            const std::vector<double> &verticalAxis) {
  const MantidVec &X = outputWS->readX(0);
  size_t qstart(0), qend(verticalAxis.size() - 1), en_start(0),
      en_end(X.size() - 1);
  if (!getIntersectionRegion(outputWS, verticalAxis, inputQ, qstart, qend,
                             en_start, en_end))
    return;

  for (size_t qi = qstart; qi < qend; ++qi) {
    const double vlo = verticalAxis[qi];
    const double vhi = verticalAxis[qi + 1];
    for (size_t ei = en_start; ei < en_end; ++ei) {
      const V2D ll(X[ei], vlo);
      const V2D lr(X[ei + 1], vlo);
      const V2D ur(X[ei + 1], vhi);
      const V2D ul(X[ei], vhi);
      const Quadrilateral outputQ(ll, lr, ur, ul);

      double yValue = inputWS->readY(i)[j];
      if (boost::math::isnan(yValue)) {
        continue;
      }
      try {
        ConvexPolygon overlap = intersectionByLaszlo(outputQ, inputQ);
        const double weight = overlap.area() / inputQ.area();
        yValue *= weight;
        double eValue = inputWS->readE(i)[j] * weight;
        const double overlapWidth = overlap.largestX() - overlap.smallestX();
        if (inputWS->isDistribution()) {
          yValue *= overlapWidth;
          eValue *= overlapWidth;
        }
        eValue = eValue * eValue;
        PARALLEL_CRITICAL(overlap) {
          outputWS->dataY(qi)[ei] += yValue;
          outputWS->dataE(qi)[ei] += eValue;
        }
      } catch (Geometry::NoIntersectionException &) {
      }
    }
  }
}

/**
 * Rebin the input quadrilateral to the output grid
 * @param inputQ The input polygon
 * @param inputWS The input workspace containing the input intensity values
 * @param i The index in the vertical axis direction that inputQ references
 * @param j The index in the horizontal axis direction that inputQ references
 * @param outputWS A pointer to the output workspace that accumulates the data
 * @param verticalAxis A vector containing the output vertical axis bin
 * boundaries
 */
void Rebin2D::rebinToFractionalOutput(const Geometry::Quadrilateral &inputQ,
                                      MatrixWorkspace_const_sptr inputWS,
                                      const size_t i, const size_t j,
                                      RebinnedOutput_sptr outputWS,
                                      const std::vector<double> &verticalAxis) {
  const MantidVec &X = outputWS->readX(0);
  size_t qstart(0), qend(verticalAxis.size() - 1), en_start(0),
      en_end(X.size() - 1);
  if (!getIntersectionRegion(outputWS, verticalAxis, inputQ, qstart, qend,
                             en_start, en_end))
    return;

  for (size_t qi = qstart; qi < qend; ++qi) {
    const double vlo = verticalAxis[qi];
    const double vhi = verticalAxis[qi + 1];
    for (size_t ei = en_start; ei < en_end; ++ei) {
      const V2D ll(X[ei], vlo);
      const V2D lr(X[ei + 1], vlo);
      const V2D ur(X[ei + 1], vhi);
      const V2D ul(X[ei], vhi);
      const Quadrilateral outputQ(ll, lr, ur, ul);

      double yValue = inputWS->readY(i)[j];
      if (boost::math::isnan(yValue)) {
        continue;
      }
      try {
        ConvexPolygon overlap = intersectionByLaszlo(outputQ, inputQ);
        const double weight = overlap.area() / inputQ.area();
        yValue *= weight;
        double eValue = inputWS->readE(i)[j] * weight;
        const double overlapWidth = overlap.largestX() - overlap.smallestX();
        // Don't do the overlap removal if already RebinnedOutput.
        // This wreaks havoc on the data.
        if (inputWS->isDistribution() && inputWS->id() != "RebinnedOutput") {
          yValue *= overlapWidth;
          eValue *= overlapWidth;
        }
        eValue *= eValue;
        PARALLEL_CRITICAL(overlap) {
          outputWS->dataY(qi)[ei] += yValue;
          outputWS->dataE(qi)[ei] += eValue;
          outputWS->dataF(qi)[ei] += weight;
        }
      } catch (Geometry::NoIntersectionException &) {
      }
    }
  }
}

/**
 * Find the possible region of intersection on the output workspace for the
 * given polygon
 * @param outputWS A pointer to the output workspace
 * @param verticalAxis A vector containing the output vertical axis edges
 * @param inputQ The input polygon
 * @param qstart An output giving the starting index in the Q direction
 * @param qend An output giving the end index in the Q direction
 * @param en_start An output giving the start index in the dE direction
 * @param en_end An output giving the end index in the dE direction
 * @return True if an intersecition is possible
 */
bool Rebin2D::getIntersectionRegion(API::MatrixWorkspace_const_sptr outputWS,
                                    const std::vector<double> &verticalAxis,
                                    const Geometry::Quadrilateral &inputQ,
                                    size_t &qstart, size_t &qend,
                                    size_t &en_start, size_t &en_end) const {
  const MantidVec &xAxis = outputWS->readX(0);
  const double xn_lo(inputQ.smallestX()), xn_hi(inputQ.largestX());
  const double yn_lo(inputQ.smallestY()), yn_hi(inputQ.largestY());

  if (xn_hi < xAxis.front() || xn_lo > xAxis.back() ||
      yn_hi < verticalAxis.front() || yn_lo > verticalAxis.back())
    return false;

  MantidVec::const_iterator start_it =
      std::upper_bound(xAxis.begin(), xAxis.end(), xn_lo);
  MantidVec::const_iterator end_it =
      std::upper_bound(xAxis.begin(), xAxis.end(), xn_hi);
  en_start = 0;
  en_end = xAxis.size() - 1;
  if (start_it != xAxis.begin()) {
    en_start = (start_it - xAxis.begin() - 1);
  }
  if (end_it != xAxis.end()) {
    en_end = end_it - xAxis.begin();
  }

  // Q region
  start_it = std::upper_bound(verticalAxis.begin(), verticalAxis.end(), yn_lo);
  end_it = std::upper_bound(verticalAxis.begin(), verticalAxis.end(), yn_hi);
  qstart = 0;
  qend = verticalAxis.size() - 1;
  if (start_it != verticalAxis.begin()) {
    qstart = (start_it - verticalAxis.begin() - 1);
  }
  if (end_it != verticalAxis.end()) {
    qend = end_it - verticalAxis.begin();
  }

  return true;
}

/**
 * Computes the square root of the errors and if the input was a distribution
 * this divides by the new bin-width
 * @param outputWS The workspace containing the output data
 * @param inputWS The input workspace used for testing distribution state
 */
void Rebin2D::normaliseOutput(MatrixWorkspace_sptr outputWS,
                              MatrixWorkspace_const_sptr inputWS) {
  // PARALLEL_FOR1(outputWS)
  for (int64_t i = 0; i < static_cast<int64_t>(outputWS->getNumberHistograms());
       ++i) {
    PARALLEL_START_INTERUPT_REGION

    MantidVec &outputY = outputWS->dataY(i);
    MantidVec &outputE = outputWS->dataE(i);
    for (size_t j = 0; j < outputWS->blocksize(); ++j) {
      m_progress->report("Calculating errors");
      const double binWidth =
          (outputWS->readX(i)[j + 1] - outputWS->readX(i)[j]);
      double eValue = std::sqrt(outputE[j]);
      // Don't do this for a RebinnedOutput workspace. The fractions
      // take care of such things.
      if (inputWS->isDistribution() && inputWS->id() != "RebinnedOutput") {
        outputY[j] /= binWidth;
        eValue /= binWidth;
      }
      outputE[j] = eValue;
    }

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  outputWS->isDistribution(inputWS->isDistribution());
}

/**
 * Setup the output workspace
 * @param parent :: A pointer to the input workspace
 * @param newXBins [out] :: An output vector to be filled with the new X bin
 * boundaries
 * @param newYBins [out] :: An output vector to be filled with the new Y bin
 * boundaries
 * @return A pointer to the output workspace
 */
MatrixWorkspace_sptr
Rebin2D::createOutputWorkspace(MatrixWorkspace_const_sptr parent,
                               MantidVec &newXBins, MantidVec &newYBins) const {
  using Kernel::VectorHelper::createAxisFromRebinParams;
  // First create the two sets of bin boundaries
  const int newXSize =
      createAxisFromRebinParams(getProperty("Axis1Binning"), newXBins);
  const int newYSize =
      createAxisFromRebinParams(getProperty("Axis2Binning"), newYBins);
  // and now the workspace
  MatrixWorkspace_sptr outputWS;
  if (!this->useFractionalArea) {
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

  // Now set the axis values
  for (size_t i = 0; i < static_cast<size_t>(newYSize - 1); ++i) {
    outputWS->setX(i, newXBins);
  }

  return outputWS;
}

} // namespace Algorithms
} // namespace Mantid
