//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMDAlgorithms/ConvertMDHistoToMatrixWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/NullCoordTransform.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/NumericAxis.h"
#include <boost/mpl/if.hpp>
#include <boost/type_traits.hpp>
#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {
/**
 * A shared pointer deleter that doesn't delete.
 */
struct NullDeleter {
  void operator()(void const *) const { // Do nothing
  }
};

/**
Find the dimension to use as the plot axis.

@param start : start point in final frame
@param end : end point in final frame
@param transform : transform to original frame
@param inputWorkspace : inputWorkspace
@param logger : log object
@param id : id, or current index for the dimension to use as the x-plot
dimension
@param xAxisLabel : in/out reference for text to use as the x-axis label.

@return id/index of the dimension with the longest span in the original
coordinate system.
*/
size_t findXAxis(const VMD &start, const VMD &end,
                 CoordTransform const *const transform,
                 IMDHistoWorkspace const *const inputWorkspace, Logger &logger,
                 const size_t id, std::string& xAxisLabel) {

  // Find the start and end points in the original workspace
  VMD originalStart = transform->applyVMD(start);
  VMD originalEnd = transform->applyVMD(end);
  VMD diff = originalEnd - originalStart;

  // Now we find the dimension with the biggest change
  double largest = -1e30;

  size_t dimIndex = id;

  const size_t nOriginalWorkspaces = inputWorkspace->numOriginalWorkspaces();
  if (nOriginalWorkspaces < 1) {
    logger.information("No original workspaces. Assume X-axis is Dim0.");
    return dimIndex;
  }

  auto originalWS = boost::dynamic_pointer_cast<IMDWorkspace>(
      inputWorkspace->getOriginalWorkspace(nOriginalWorkspaces - 1));

  for (size_t d = 0; d < diff.getNumDims(); d++) {
    if (fabs(diff[d]) > largest ||
        (originalWS->getDimension(dimIndex)->getIsIntegrated())) {
      // Skip over any integrated dimensions
      if (originalWS && !originalWS->getDimension(d)->getIsIntegrated()) {
        largest = fabs(diff[d]);
        dimIndex = d;
      }
    }
  }
  // Use the x-axis label from the original workspace.
  xAxisLabel = originalWS->getDimension(dimIndex)->getName();
  return dimIndex;
}
}

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertMDHistoToMatrixWorkspace)

/// Decalare the properties
void ConvertMDHistoToMatrixWorkspace::init() {
  declareProperty(new WorkspaceProperty<API::IMDHistoWorkspace>(
                      "InputWorkspace", "", Direction::Input),
                  "An input IMDHistoWorkspace.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "An output Workspace2D.");

  std::vector<std::string> normalizations(3);
  normalizations[0] = "NoNormalization";
  normalizations[1] = "VolumeNormalization";
  normalizations[2] = "NumEventsNormalization";

  declareProperty("Normalization", normalizations[0],
                  Kernel::IValidator_sptr(
                      new Kernel::ListValidator<std::string>(normalizations)),
                  "Signal normalization method");

  declareProperty(
      new PropertyWithValue<bool>("FindXAxis", true, Direction::Input),
      "If True, tries to automatically determine the dimension to use as the "
      "output x-axis. Applies to line cut MD workspaces.");
}

/// Execute the algorithm
void ConvertMDHistoToMatrixWorkspace::exec() {

  IMDHistoWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");
  Mantid::Geometry::VecIMDDimension_const_sptr nonIntegDims =
      inputWorkspace->getNonIntegratedDimensions();

  if (nonIntegDims.size() == 1) {
    make1DWorkspace();
  } else if (nonIntegDims.size() == 2) {
    make2DWorkspace();
  } else {
    throw std::invalid_argument(
        "Cannot convert MD workspace with more than 2 dimensions.");
  }
}

/**
  * Make 1D MatrixWorkspace
  */
void ConvertMDHistoToMatrixWorkspace::make1DWorkspace() {
  IMDHistoWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  // This code is copied from MantidQwtIMDWorkspaceData
  Mantid::Geometry::VecIMDDimension_const_sptr nonIntegDims =
      inputWorkspace->getNonIntegratedDimensions();

  std::string alongDim = "";
  if (!nonIntegDims.empty())
    alongDim = nonIntegDims[0]->getDimensionId();
  else
    alongDim = inputWorkspace->getDimension(0)->getDimensionId();

  size_t nd = inputWorkspace->getNumDims();
  Mantid::Kernel::VMD start = VMD(nd);
  Mantid::Kernel::VMD end = VMD(nd);

  size_t id = 0;
  for (size_t d = 0; d < nd; d++) {
    Mantid::Geometry::IMDDimension_const_sptr dim =
        inputWorkspace->getDimension(d);
    if (dim->getDimensionId() == alongDim) {
      // All the way through in the single dimension
      start[d] = dim->getMinimum();
      end[d] = dim->getMaximum();
      id = d; // We take the first non integrated dimension to be the diemnsion
              // of interest.
    } else {
      // Mid point along each dimension
      start[d] = (dim->getMaximum() + dim->getMinimum()) / 2.0f;
      end[d] = start[d];
    }
  }

  // Unit direction of the line
  Mantid::Kernel::VMD dir = end - start;
  dir.normalize();

  std::string normProp = getPropertyValue("Normalization");

  Mantid::API::MDNormalization normalization;
  if (normProp == "NoNormalization") {
    normalization = NoNormalization;
  } else if (normProp == "VolumeNormalization") {
    normalization = VolumeNormalization;
  } else if (normProp == "NumEventsNormalization") {
    normalization = NumEventsNormalization;
  } else {
    normalization = NoNormalization;
  }

  std::vector<Mantid::coord_t> X;
  std::vector<Mantid::signal_t> Y;
  std::vector<Mantid::signal_t> E;

  inputWorkspace->getLinePlot(start, end, normalization, X, Y, E);

  MatrixWorkspace_sptr outputWorkspace =
      WorkspaceFactory::Instance().create("Workspace2D", 1, X.size(), Y.size());
  outputWorkspace->dataY(0).assign(Y.begin(), Y.end());
  outputWorkspace->dataE(0).assign(E.begin(), E.end());

  const size_t numberTransformsToOriginal =
      inputWorkspace->getNumberTransformsToOriginal();

  CoordTransform_const_sptr transform =
      boost::make_shared<NullCoordTransform>(inputWorkspace->getNumDims());
  if (numberTransformsToOriginal > 0) {
    const size_t indexToLastTransform = numberTransformsToOriginal - 1;
    transform = CoordTransform_const_sptr(
        inputWorkspace->getTransformToOriginal(indexToLastTransform),
        NullDeleter());
  }

  assert(X.size() == outputWorkspace->dataX(0).size());

  std::string xAxisLabel = inputWorkspace->getDimension(id)->getName();
  const bool autoFind = this->getProperty("FindXAxis");
  if (autoFind) {
    // We look to the original workspace if possbible to find the dimension of
    // interest to plot against.
    id = findXAxis(start, end, transform.get(), inputWorkspace.get(), g_log, id, xAxisLabel);
  }

  for (size_t i = 0; i < X.size(); ++i) {
    // Coordinates in the workspace being plotted
    VMD wsCoord = start + dir * X[i];

    VMD inTargetCoord = transform->applyVMD(wsCoord);
    outputWorkspace->dataX(0)[i] = inTargetCoord[id];
  }

  boost::shared_ptr<Kernel::Units::Label> labelX =
      boost::dynamic_pointer_cast<Kernel::Units::Label>(
          Kernel::UnitFactory::Instance().create("Label"));
  labelX->setLabel(xAxisLabel);
  outputWorkspace->getAxis(0)->unit() = labelX;

  outputWorkspace->setYUnitLabel("Signal");

  setProperty("OutputWorkspace", outputWorkspace);
}

/**
  * Make 2D MatrixWorkspace
  */
void ConvertMDHistoToMatrixWorkspace::make2DWorkspace() {
  // get the input workspace
  IMDHistoWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  // find the non-integrated dimensions
  Mantid::Geometry::VecIMDDimension_const_sptr nonIntegDims =
      inputWorkspace->getNonIntegratedDimensions();

  auto xDim = nonIntegDims[0];
  auto yDim = nonIntegDims[1];

  size_t nx = xDim->getNBins();
  size_t ny = yDim->getNBins();

  size_t xDimIndex =
      inputWorkspace->getDimensionIndexById(xDim->getDimensionId());
  size_t xStride = calcStride(*inputWorkspace, xDimIndex);

  size_t yDimIndex =
      inputWorkspace->getDimensionIndexById(yDim->getDimensionId());
  size_t yStride = calcStride(*inputWorkspace, yDimIndex);

  // get the normalization of the output
  std::string normProp = getPropertyValue("Normalization");
  Mantid::API::MDNormalization normalization;
  if (normProp == "NoNormalization") {
    normalization = NoNormalization;
  } else if (normProp == "VolumeNormalization") {
    normalization = VolumeNormalization;
  } else if (normProp == "NumEventsNormalization") {
    normalization = NumEventsNormalization;
  } else {
    normalization = NoNormalization;
  }
  signal_t inverseVolume =
      static_cast<signal_t>(inputWorkspace->getInverseVolume());

  // create the output workspace
  MatrixWorkspace_sptr outputWorkspace =
      WorkspaceFactory::Instance().create("Workspace2D", ny, nx + 1, nx);

  // set the x-values
  Mantid::MantidVec &X = outputWorkspace->dataX(0);
  double dx = xDim->getBinWidth();
  double x = xDim->getMinimum();
  for (auto ix = X.begin(); ix != X.end(); ++ix, x += dx) {
    *ix = x;
  }

  // set the y-values and errors
  for (size_t i = 0; i < ny; ++i) {
    if (i > 0)
      outputWorkspace->setX(i, X);
    auto &Y = outputWorkspace->dataY(i);
    auto &E = outputWorkspace->dataE(i);

    size_t yOffset = i * yStride;
    for (size_t j = 0; j < nx; ++j) {
      size_t linearIndex = yOffset + j * xStride;
      signal_t signal = inputWorkspace->getSignalArray()[linearIndex];
      signal_t error = inputWorkspace->getErrorSquaredArray()[linearIndex];
      // apply normalization
      if (normalization != NoNormalization) {
        if (normalization == VolumeNormalization) {
          signal *= inverseVolume;
          error *= inverseVolume;
        } else // normalization == NumEventsNormalization
        {
          signal_t factor = inputWorkspace->getNumEventsArray()[linearIndex];
          factor = factor != 0.0 ? 1.0 / factor : 1.0;
          signal *= factor;
          error *= factor;
        }
      }
      Y[j] = signal;
      E[j] = sqrt(error);
    }
  }

  // set the first axis
  auto labelX = boost::dynamic_pointer_cast<Kernel::Units::Label>(
      Kernel::UnitFactory::Instance().create("Label"));
  labelX->setLabel(xDim->getName());
  outputWorkspace->getAxis(0)->unit() = labelX;

  // set the second axis
  auto yAxis = new NumericAxis(ny);
  for (size_t i = 0; i < ny; ++i) {
    yAxis->setValue(i, yDim->getX(i));
  }
  auto labelY = boost::dynamic_pointer_cast<Kernel::Units::Label>(
      Kernel::UnitFactory::Instance().create("Label"));
  labelY->setLabel(yDim->getName());
  yAxis->unit() = labelY;
  outputWorkspace->replaceAxis(1, yAxis);

  // set the "units" for the y values
  outputWorkspace->setYUnitLabel("Signal");

  // done
  setProperty("OutputWorkspace", outputWorkspace);
}

/**
  * Calculate the stride for a dimension.
  * @param workspace :: An MD workspace.
  * @param dim :: A dimension index to calculate the stride for.
  */
size_t ConvertMDHistoToMatrixWorkspace::calcStride(
    const API::IMDHistoWorkspace &workspace, size_t dim) const {
  size_t stride = 1;
  for (size_t i = 0; i < dim; ++i) {
    auto dimension = workspace.getDimension(i);
    stride *= dimension->getNBins();
  }
  return stride;
}

} // namespace MDAlgorithms
} // namespace Mantid
