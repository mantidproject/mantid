// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/ReflectometryTransform.h"

#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/CalculateReflectometry.h"
#include "MantidDataObjects/FractionalRebinning.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/V2D.h"
#include "MantidKernel/VectorHelper.h"

#include <boost/shared_ptr.hpp>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace {
/**
 *  Writes one row to an existing table
 *  @param vertexes : The table that the rows will be written to
 *  @param vertex : The vertex from which the data is retrieved for writing i.e
 * lower left, lower right etc.
 *  @param nHisto : The number of the histogram
 *  @param nBins : The number of the bin
 *  @param signal : The Y value of the bin
 *  @param error : The E value of the bin
 */
void writeRow(boost::shared_ptr<Mantid::DataObjects::TableWorkspace> &vertexes,
              const V2D &vertex, size_t nHisto, size_t nBins, double signal,
              double error) {
  TableRow row = vertexes->appendRow();
  row << vertex.X() << vertex.Y() << int(nHisto) << int(nBins) << signal
      << error;
}
/**
 *  Adds the column headings to a table
 *  @param vertexes : Table to which the columns are written to.
 */
void addColumnHeadings(Mantid::DataObjects::TableWorkspace &vertexes,
                       const std::string &outputDimensions) {

  if (outputDimensions == "Q (lab frame)") {
    vertexes.addColumn("double", "Qx");
    vertexes.addColumn("double", "Qy");
    vertexes.addColumn("int", "OriginIndex");
    vertexes.addColumn("int", "OriginBin");
    vertexes.addColumn("double", "CellSignal");
    vertexes.addColumn("double", "CellError");
  }
  if (outputDimensions == "P (lab frame)") {
    vertexes.addColumn("double", "Pi+Pf");
    vertexes.addColumn("double", "Pi-Pf");
    vertexes.addColumn("int", "OriginIndex");
    vertexes.addColumn("int", "OriginBin");
    vertexes.addColumn("double", "CellSignal");
    vertexes.addColumn("double", "CellError");
  }
  if (outputDimensions == "K (incident, final)") {
    vertexes.addColumn("double", "Ki");
    vertexes.addColumn("double", "Kf");
    vertexes.addColumn("int", "OriginIndex");
    vertexes.addColumn("int", "OriginBin");
    vertexes.addColumn("double", "CellSignal");
    vertexes.addColumn("double", "CellError");
  }
}
} // namespace
namespace Mantid {
namespace DataObjects {

/**
 * Constructor
 * @param d0Label : label for the first dimension axis
 * @param d0ID : unique identifier for the first dimension
 * @param d0Min : minimum value for the first dimension
 * @param d0Max : maximum value for the first dimension
 * @param d0NumBins : number of bins in first dimension
 * @param d1Label : label for the second dimension axis
 * @param d1ID : unique identifier for the second dimension
 * @param d1Min : minimum value for the second dimension
 * @param d1Max : maximum value for the second dimension
 * @param d1NumBins : number of bins in the second dimension
 * @param calc : Pointer to CalculateReflectometry object.
 */
ReflectometryTransform::ReflectometryTransform(
    const std::string &d0Label, const std::string &d0ID, double d0Min,
    double d0Max, const std::string &d1Label, const std::string &d1ID,
    double d1Min, double d1Max, size_t d0NumBins, size_t d1NumBins,
    CalculateReflectometry *calc)
    : m_d0NumBins(d0NumBins), m_d1NumBins(d1NumBins), m_d0Min(d0Min),
      m_d1Min(d1Min), m_d0Max(d0Max), m_d1Max(d1Max), m_d0Label(d0Label),
      m_d1Label(d1Label), m_d0ID(d0ID), m_d1ID(d1ID), m_calculator(calc) {
  if (d0Min >= d0Max || d1Min >= d1Max)
    throw std::invalid_argument(
        "The supplied minimum values must be less than the maximum values.");
}

/**
 * Creates an MD workspace
 * @param a : pointer to the first dimension of the MDWorkspace
 *@param b : pointer to the second dimension of the MDWorkspace
 * @param boxController : controls how the MDWorkspace will be split
 */
boost::shared_ptr<MDEventWorkspace2Lean>
ReflectometryTransform::createMDWorkspace(
    Mantid::Geometry::IMDDimension_sptr a,
    Mantid::Geometry::IMDDimension_sptr b,
    BoxController_sptr boxController) const {
  auto ws = boost::make_shared<MDEventWorkspace2Lean>();

  ws->addDimension(a);
  ws->addDimension(b);

  BoxController_sptr wsbc = ws->getBoxController(); // Get the box controller
  wsbc->setSplitInto(boxController->getSplitInto(0));
  wsbc->setMaxDepth(boxController->getMaxDepth());
  wsbc->setSplitThreshold(boxController->getSplitThreshold());

  // Initialize the workspace.
  ws->initialize();

  // Start with a MDGridBox.
  ws->splitBox();
  return ws;
}

/**
 * Create a new X-Axis for the output workspace
 * @param ws : Workspace to attach the axis to
 * @param gradX : Gradient used in the linear transform from index to X-scale
 * @param cxToUnit : C-offset used in the linear transform
 * @param nBins : Number of bins along this axis
 * @param caption : Caption for the axis
 * @param units : Units label for the axis
 * @return Vector containing increments along the axis.
 */
MantidVec createXAxis(MatrixWorkspace *const ws, const double gradX,
                      const double cxToUnit, const size_t nBins,
                      const std::string &caption, const std::string &units) {
  // Create an X - Axis.
  auto xAxis = std::make_unique<BinEdgeAxis>(nBins);
  ws->replaceAxis(0, std::move(xAxis));
  auto unitXBasePtr = UnitFactory::Instance().create("Label");
  boost::shared_ptr<Mantid::Kernel::Units::Label> xUnit =
      boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(unitXBasePtr);
  xUnit->setLabel(caption, units);
  xAxis->unit() = xUnit;
  xAxis->title() = caption;
  MantidVec xAxisVec(nBins);
  for (size_t i = 0; i < nBins; ++i) {
    double qxIncrement =
        ((1 / gradX) * (static_cast<double>(i) + 1) + cxToUnit);
    xAxis->setValue(i, qxIncrement);
    xAxisVec[i] = qxIncrement;
  }
  return xAxisVec;
}

/**
 * Create a new Y, or Vertical Axis for the output workspace
 * @param ws : Workspace to attache the vertical axis to
 * @param xAxisVec : Vector of x axis increments
 * @param gradY : Gradient used in linear transform from index to Y-scale
 * @param cyToUnit : C-offset used in the linear transform
 * @param nBins : Number of bins along the axis
 * @param caption : Caption for the axis
 * @param units : Units label for the axis
 */
void createVerticalAxis(MatrixWorkspace *const ws, const MantidVec &xAxisVec,
                        const double gradY, const double cyToUnit,
                        const size_t nBins, const std::string &caption,
                        const std::string &units) {
  // Create a Y (vertical) Axis
  auto verticalAxis = std::make_unique<BinEdgeAxis>(nBins);
  ws->replaceAxis(1, std::move(verticalAxis));
  auto unitZBasePtr = UnitFactory::Instance().create("Label");
  boost::shared_ptr<Mantid::Kernel::Units::Label> verticalUnit =
      boost::dynamic_pointer_cast<Mantid::Kernel::Units::Label>(unitZBasePtr);
  verticalAxis->unit() = verticalUnit;
  verticalUnit->setLabel(caption, units);
  verticalAxis->title() = caption;
  auto xAxis = Kernel::make_cow<HistogramData::HistogramX>(xAxisVec);
  for (size_t i = 0; i < nBins; ++i) {
    ws->setX(i, xAxis);
    double qzIncrement =
        ((1 / gradY) * (static_cast<double>(i) + 1) + cyToUnit);
    verticalAxis->setValue(i, qzIncrement);
  }
}

/**
 * A map detector ID and Q ranges
 * This method looks unnecessary as it could be calculated on the fly but
 * the parallelization means that lazy instantation slows it down due to the
 * necessary CRITICAL sections required to update the cache. The Q range
 * values are required very frequently so the total time is more than
 * offset by this precaching step
 */
DetectorAngularCache initAngularCaches(const MatrixWorkspace *const workspace) {
  const size_t nhist = workspace->getNumberHistograms();
  std::vector<double> thetas(nhist);
  std::vector<double> thetaWidths(nhist);
  std::vector<double> detectorHeights(nhist);

  auto inst = workspace->getInstrument();
  const V3D upDirVec = inst->getReferenceFrame()->vecPointingUp();

  const auto &spectrumInfo = workspace->spectrumInfo();
  const auto &detectorInfo = workspace->detectorInfo();
  for (size_t i = 0; i < nhist; ++i) {
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i)) {
      // If no detector found, skip onto the next spectrum
      thetas[i] = -1.0; // Indicates a detector to skip
      thetaWidths[i] = -1.0;
      continue;
    }

    // We have to convert theta from radians to degrees
    const double theta = spectrumInfo.signedTwoTheta(i) * rad2deg;
    thetas[i] = theta;
    /**
     * Determine width from shape geometry. A group is assumed to contain
     * detectors with the same shape & r, theta value, i.e. a ring mapped-group
     * The shape is retrieved and rotated to match the rotation of the detector.
     * The angular width is computed using the l2 distance from the sample
     */
    // If the spectrum is based on a group of detectors assume they all have
    // same shape and same r,theta
    // DetectorGroup::getID gives ID of first detector.
    size_t detIndex = detectorInfo.indexOf(spectrumInfo.detector(i).getID());
    double l2 = detectorInfo.l2(detIndex);
    // Get the shape
    auto shape =
        detectorInfo.detector(detIndex)
            .shape(); // Defined in its own reference frame with centre at 0,0,0
    BoundingBox bbox = shape->getBoundingBox();
    auto maxPoint(bbox.maxPoint());
    auto minPoint(bbox.minPoint());
    auto span = maxPoint - minPoint;
    detectorHeights[i] = span.scalar_prod(upDirVec);
    thetaWidths[i] = 2.0 * std::fabs(std::atan((detectorHeights[i] / 2) / l2)) *
                     180.0 / M_PI;
  }
  DetectorAngularCache cache;
  cache.thetas = thetas;
  cache.thetaWidths = thetaWidths;
  cache.detectorHeights = detectorHeights;
  return cache;
}

/**
 * Performs centre-point rebinning and produces an MDWorkspace
 * @param inputWs : The workspace you wish to perform centre-point rebinning on.
 * @param boxController : controls how the MDWorkspace will be split
 * @param frame: the md frame for the two MDHistoDimensions
 * @returns An MDWorkspace based on centre-point rebinning of the inputWS
 */
Mantid::API::IMDEventWorkspace_sptr ReflectometryTransform::executeMD(
    Mantid::API::MatrixWorkspace_const_sptr inputWs,
    BoxController_sptr boxController,
    Mantid::Geometry::MDFrame_uptr frame) const {
  auto dim0 = boost::make_shared<MDHistoDimension>(
      m_d0Label, m_d0ID, *frame, static_cast<Mantid::coord_t>(m_d0Min),
      static_cast<Mantid::coord_t>(m_d0Max), m_d0NumBins);
  auto dim1 = boost::make_shared<MDHistoDimension>(
      m_d1Label, m_d1ID, *frame, static_cast<Mantid::coord_t>(m_d1Min),
      static_cast<Mantid::coord_t>(m_d1Max), m_d1NumBins);

  auto ws = createMDWorkspace(dim0, dim1, boxController);

  auto spectraAxis = inputWs->getAxis(1);
  for (size_t index = 0; index < inputWs->getNumberHistograms(); ++index) {
    auto counts = inputWs->readY(index);
    auto wavelengths = inputWs->readX(index);
    auto errors = inputWs->readE(index);
    const size_t nInputBins = wavelengths.size() - 1;
    const double theta_final = spectraAxis->getValue(index);
    m_calculator->setThetaFinal(theta_final);
    // Loop over all bins in spectra
    for (size_t binIndex = 0; binIndex < nInputBins; ++binIndex) {
      const double &wavelength =
          0.5 * (wavelengths[binIndex] + wavelengths[binIndex + 1]);
      double _d0 = m_calculator->calculateDim0(wavelength);
      double _d1 = m_calculator->calculateDim1(wavelength);
      double centers[2] = {_d0, _d1};

      ws->addEvent(MDLeanEvent<2>(float(counts[binIndex]),
                                  float(errors[binIndex] * errors[binIndex]),
                                  centers));
    }
  }
  ws->splitAllIfNeeded(nullptr);
  ws->refreshCache();
  return ws;
}

/**
 * Convert to the output dimensions
 * @param inputWs : Input Matrix workspace
 * @return workspace group containing output matrix workspaces of ki and kf
 */
Mantid::API::MatrixWorkspace_sptr ReflectometryTransform::execute(
    Mantid::API::MatrixWorkspace_const_sptr inputWs) const {
  auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();

  ws->initialize(m_d1NumBins, m_d0NumBins,
                 m_d0NumBins); // Create the output workspace as a distribution

  // Mapping so that d0 and d1 values calculated can be added to the matrix
  // workspace at the correct index.
  const double gradD0 =
      double(m_d0NumBins) / (m_d0Max - m_d0Min); // The x - axis
  const double gradD1 =
      double(m_d1NumBins) / (m_d1Max - m_d1Min); // Actually the y-axis
  const double cxToIndex = -gradD0 * m_d0Min;
  const double cyToIndex = -gradD1 * m_d1Min;
  const double cxToD0 = m_d0Min - (1 / gradD0);
  const double cyToD1 = m_d1Min - (1 / gradD1);

  // Create an X - Axis.
  MantidVec xAxisVec = createXAxis(ws.get(), gradD0, cxToD0, m_d0NumBins,
                                   m_d0Label, "1/Angstroms");
  // Create a Y (vertical) Axis
  createVerticalAxis(ws.get(), xAxisVec, gradD1, cyToD1, m_d1NumBins, m_d1Label,
                     "1/Angstroms");

  // Loop over all entries in the input workspace and calculate d0 and d1
  // for each.
  auto spectraAxis = inputWs->getAxis(1);
  for (size_t index = 0; index < inputWs->getNumberHistograms(); ++index) {
    auto counts = inputWs->readY(index);
    auto wavelengths = inputWs->readX(index);
    auto errors = inputWs->readE(index);
    const size_t nInputBins = wavelengths.size() - 1;
    const double theta_final = spectraAxis->getValue(index);
    m_calculator->setThetaFinal(theta_final);
    // Loop over all bins in spectra
    for (size_t binIndex = 0; binIndex < nInputBins; ++binIndex) {
      const double wavelength =
          0.5 * (wavelengths[binIndex] + wavelengths[binIndex + 1]);
      double _d0 = m_calculator->calculateDim0(wavelength);
      double _d1 = m_calculator->calculateDim1(wavelength);

      if (_d0 >= m_d0Min && _d0 <= m_d0Max && _d1 >= m_d1Min &&
          _d1 <= m_d1Max) // Check that the calculated ki and kf are in range
      {
        const int outIndexX = static_cast<int>((gradD0 * _d0) + cxToIndex);
        const int outIndexZ = static_cast<int>((gradD1 * _d1) + cyToIndex);

        ws->dataY(outIndexZ)[outIndexX] += counts[binIndex];
        ws->dataE(outIndexZ)[outIndexX] += errors[binIndex];
      }
    }
  }
  return ws;
}

IMDHistoWorkspace_sptr ReflectometryTransform::executeMDNormPoly(
    MatrixWorkspace_const_sptr inputWs) const {

  auto input_x_dim = inputWs->getXDimension();

  MDHistoDimension_sptr dim0 = MDHistoDimension_sptr(new MDHistoDimension(
      input_x_dim->getName(), input_x_dim->getDimensionId(),
      input_x_dim->getMDFrame(),
      static_cast<Mantid::coord_t>(input_x_dim->getMinimum()),
      static_cast<Mantid::coord_t>(input_x_dim->getMaximum()),
      input_x_dim->getNBins()));

  auto input_y_dim = inputWs->getYDimension();

  MDHistoDimension_sptr dim1 = MDHistoDimension_sptr(new MDHistoDimension(
      input_y_dim->getName(), input_y_dim->getDimensionId(),
      input_y_dim->getMDFrame(),
      static_cast<Mantid::coord_t>(input_y_dim->getMinimum()),
      static_cast<Mantid::coord_t>(input_y_dim->getMaximum()),
      input_y_dim->getNBins()));

  auto outWs = boost::make_shared<MDHistoWorkspace>(dim0, dim1);

  for (size_t nHistoIndex = 0; nHistoIndex < inputWs->getNumberHistograms();
       ++nHistoIndex) {
    const auto &Y = inputWs->y(nHistoIndex);
    const auto &E = inputWs->e(nHistoIndex);

    const size_t numBins = Y.size();
    for (size_t nBinIndex = 0; nBinIndex < numBins; ++nBinIndex) {
      const auto value_index = outWs->getLinearIndex(nBinIndex, nHistoIndex);
      outWs->setSignalAt(value_index, Y[nBinIndex]);
      outWs->setErrorSquaredAt(value_index, E[nBinIndex] * E[nBinIndex]);
    }
  }
  return outWs;
}

/**
 * Execution path for NormalisedPolygon Rebinning
 * @param inputWS : Workspace to be rebinned
 * @param vertexes : TableWorkspace for debugging purposes
 * @param dumpVertexes : determines whether vertexes will be written to for
 * debugging purposes or not
 * @param outputDimensions : used for the column headings for Dump Vertexes
 */
MatrixWorkspace_sptr ReflectometryTransform::executeNormPoly(
    const MatrixWorkspace_const_sptr &inputWS,
    boost::shared_ptr<Mantid::DataObjects::TableWorkspace> &vertexes,
    bool dumpVertexes, std::string outputDimensions) const {
  MatrixWorkspace_sptr temp = WorkspaceFactory::Instance().create(
      "RebinnedOutput", m_d1NumBins, m_d0NumBins + 1, m_d0NumBins);
  RebinnedOutput_sptr outWS = boost::static_pointer_cast<RebinnedOutput>(temp);

  const double widthD0 = (m_d0Max - m_d0Min) / double(m_d0NumBins);
  const double widthD1 = (m_d1Max - m_d1Min) / double(m_d1NumBins);

  std::vector<double> xBinsVec;
  std::vector<double> zBinsVec;
  VectorHelper::createAxisFromRebinParams({m_d1Min, widthD1, m_d1Max},
                                          zBinsVec);
  VectorHelper::createAxisFromRebinParams({m_d0Min, widthD0, m_d0Max},
                                          xBinsVec);

  // Put the correct bin boundaries into the workspace
  auto verticalAxis = std::make_unique<BinEdgeAxis>(zBinsVec);
  outWS->replaceAxis(1, std::move(verticalAxis));
  HistogramData::BinEdges binEdges(xBinsVec);
  for (size_t i = 0; i < zBinsVec.size() - 1; ++i)
    outWS->setBinEdges(i, binEdges);

  verticalAxis->title() = m_d1Label;

  // Prepare the required theta values
  DetectorAngularCache cache = initAngularCaches(inputWS.get());
  m_theta = cache.thetas;
  m_thetaWidths = cache.thetaWidths;

  const size_t nHistos = inputWS->getNumberHistograms();
  const size_t nBins = inputWS->blocksize();

  // Holds the spectrum-detector mapping
  std::vector<specnum_t> specNumberMapping;
  std::vector<detid_t> detIDMapping;
  // Create a table for the output if we want to debug vertex positioning
  addColumnHeadings(*vertexes, outputDimensions);
  const auto &spectrumInfo = inputWS->spectrumInfo();
  for (size_t i = 0; i < nHistos; ++i) {
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMasked(i) ||
        spectrumInfo.isMonitor(i)) {
      continue;
    }
    const auto &detector = spectrumInfo.detector(i);

    // Compute polygon points
    const double theta = m_theta[i];
    const double thetaWidth = m_thetaWidths[i];
    const double thetaHalfWidth = 0.5 * thetaWidth;
    const double thetaLower = theta - thetaHalfWidth;
    const double thetaUpper = theta + thetaHalfWidth;

    const MantidVec &X = inputWS->readX(i);
    const MantidVec &Y = inputWS->readY(i);
    const MantidVec &E = inputWS->readE(i);
    for (size_t j = 0; j < nBins; ++j) {
      const double lamLower = X[j];
      const double lamUpper = X[j + 1];
      const double signal = Y[j];
      const double error = E[j];

      auto inputQ =
          m_calculator->createQuad(lamUpper, lamLower, thetaUpper, thetaLower);
      FractionalRebinning::rebinToFractionalOutput(inputQ, inputWS, i, j,
                                                   *outWS, zBinsVec);
      // Find which qy bin this point lies in
      const auto qIndex =
          std::upper_bound(zBinsVec.begin(), zBinsVec.end(), inputQ[0].Y()) -
          zBinsVec.begin();
      if (qIndex != 0 && qIndex < static_cast<int>(zBinsVec.size())) {
        // Add this spectra-detector pair to the mapping
        specNumberMapping.push_back(
            outWS->getSpectrum(qIndex - 1).getSpectrumNo());
        detIDMapping.push_back(detector.getID());
      }
      // Debugging
      if (dumpVertexes) {
        writeRow(vertexes, inputQ[0], i, j, signal, error);
        writeRow(vertexes, inputQ[1], i, j, signal, error);
        writeRow(vertexes, inputQ[2], i, j, signal, error);
        writeRow(vertexes, inputQ[3], i, j, signal, error);
      }
    }
  }
  outWS->finalize();
  FractionalRebinning::normaliseOutput(outWS, inputWS);
  // Set the output spectrum-detector mapping
  SpectrumDetectorMapping outputDetectorMap(specNumberMapping, detIDMapping);
  outWS->updateSpectraUsing(outputDetectorMap);
  outWS->getAxis(0)->title() = m_d0Label;
  outWS->setYUnit("");
  outWS->setYUnitLabel("Intensity");

  return outWS;
}
} // namespace DataObjects
} // namespace Mantid
