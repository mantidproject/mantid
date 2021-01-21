// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/Q1DWeighted.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/GravitySANSHelper.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/UnitConversion.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

#include <boost/algorithm/string/split.hpp>

constexpr double deg2rad = M_PI / 180.0;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Q1DWeighted)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void Q1DWeighted::init() {
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<HistogramValidator>();
  wsValidator->add<InstrumentValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "Input workspace containing the SANS 2D data");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "Workspace that will contain the I(Q) data");
  declareProperty(
      std::make_unique<ArrayProperty<double>>(
          "OutputBinning", std::make_shared<RebinParamsValidator>()),
      "The new bin boundaries in the form: <math>x_1,\\Delta x_1,x_2,\\Delta "
      "x_2,\\dots,x_n</math>");

  auto positiveInt = std::make_shared<BoundedValidator<int>>();
  positiveInt->setLower(0);
  auto positiveDouble = std::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);

  declareProperty("NPixelDivision", 1, positiveInt,
                  "Number of sub-pixels used for each detector pixel in each "
                  "direction.The total number of sub-pixels will be "
                  "NPixelDivision*NPixelDivision.");

  // Wedge properties
  declareProperty("NumberOfWedges", 2, positiveInt,
                  "Number of wedges to calculate.");
  declareProperty("WedgeAngle", 30.0, positiveDouble,
                  "Opening angle of the wedge, in degrees.");
  declareProperty("WedgeOffset", 0.0, positiveDouble,
                  "Wedge offset relative to the horizontal axis, in degrees.");
  declareProperty(
      std::make_unique<WorkspaceProperty<WorkspaceGroup>>(
          "WedgeWorkspace", "", Direction::Output, PropertyMode::Optional),
      "Name for the WorkspaceGroup containing the wedge I(q) distributions.");

  declareProperty("PixelSizeX", 5.15, positiveDouble,
                  "Pixel size in the X direction (mm).");
  declareProperty("PixelSizeY", 5.15, positiveDouble,
                  "Pixel size in the Y direction (mm).");
  declareProperty(
      "ErrorWeighting", false,
      "Choose whether each pixel contribution will be weighted by 1/error^2.");

  declareProperty("AsymmetricWedges", false,
                  "Choose to produce the results for asymmetric wedges.");

  declareProperty("AccountForGravity", false,
                  "Take the nominal gravity drop into account.");

  declareProperty(
      std::make_unique<WorkspaceProperty<ITableWorkspace>>(
          "TableShape", "", Direction::Input, PropertyMode::Optional),
      "Table containing the shapes for integration. Only sectors supported. If "
      "provided, wedges properties defined above are not taken into account.");
}

void Q1DWeighted::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  bootstrap(inputWS);
  calculate(inputWS);
  finalize(inputWS);
}

/**
 * @brief Q1DWeighted::bootstrap
 * initializes the user inputs
 * @param inputWS : input workspace
 */
void Q1DWeighted::bootstrap(const MatrixWorkspace_const_sptr &inputWS) {
  // Get pixel size and pixel sub-division
  m_pixelSizeX = getProperty("PixelSizeX");
  m_pixelSizeY = getProperty("PixelSizeY");
  m_pixelSizeX /= 1000.;
  m_pixelSizeY /= 1000.;
  m_nSubPixels = getProperty("NPixelDivision");

  // Get weighting option
  m_errorWeighting = getProperty("ErrorWeighting");

  // Get gravity flag
  m_correctGravity = getProperty("AccountForGravity");

  // Calculate the output binning
  const std::vector<double> binParams = getProperty("OutputBinning");

  m_nQ = static_cast<size_t>(
             VectorHelper::createAxisFromRebinParams(binParams, m_qBinEdges)) -
         1;

  // number of spectra in the input
  m_nSpec = inputWS->getNumberHistograms();

  // get the number of wavelength bins in the input, note that the input is a
  // histogram
  m_nLambda = inputWS->readY(0).size();

  m_wedgesInnerRadius = std::vector<double>();
  m_wedgesOuterRadius = std::vector<double>();
  m_wedgesCenterX = std::vector<double>();
  m_wedgesCenterY = std::vector<double>();
  m_wedgesCenterAngle = std::vector<double>();
  m_wedgesAngleRange = std::vector<double>();
  m_wedgesIsAsymmetric = std::vector<bool>();
  m_asymmWedges = getProperty("AsymmetricWedges");

  if (getPropertyValue("TableShape").empty()) {
    const int wedges = getProperty("NumberOfWedges");
    m_nWedges = static_cast<size_t>(wedges);

    // Get wedge properties
    const double wedgeOffset = getProperty("WedgeOffset");
    const double wedgeAngle = getProperty("WedgeAngle");

    // Define wedges parameters in a general way
    for (size_t iw = 0; iw < m_nWedges; ++iw) {
      m_wedgesInnerRadius.push_back(0);
      // Negative outer radius is taken as a convention for infinity
      m_wedgesOuterRadius.push_back(-1);
      m_wedgesCenterX.push_back(0);
      m_wedgesCenterY.push_back(0);

      double centerAngle =
          M_PI * static_cast<double>(iw) / static_cast<double>(m_nWedges);
      if (m_asymmWedges)
        centerAngle *= 2;
      centerAngle += wedgeOffset * deg2rad;
      m_wedgesCenterAngle.push_back(centerAngle);
      m_wedgesAngleRange.push_back(wedgeAngle * deg2rad);

      m_wedgesIsAsymmetric.push_back(m_asymmWedges);
    }
  } else {
    getTableShapes();
    m_nWedges = m_wedgesInnerRadius.size();

    for (size_t i = 0; i < m_nWedges; ++i) {
      if (!m_asymmWedges && m_wedgesIsAsymmetric[i]) {
        std::stringstream ss;
        ss << "No symmetric counterpart provided for a shape. Wedge " << i + 1
           << " will have results asymmetricaly computed (i.e. only on the "
              "provided sector).";
        g_log.warning(ss.str());
      }
    }
  }

  // we store everything in 3D arrays
  // index 1 : is for the wedges + the one for the full integration,
  //           if there are no wedges, the 1st dimension will be 1
  // index 2 : will iterate over lambda bins
  // index 3 : will iterate over Q bins
  // we want to do this, since we want to average the I(Q) in each lambda bin
  // then average all the I(Q)s together
  m_intensities = std::vector<std::vector<std::vector<double>>>(
      m_nWedges + 1, std::vector<std::vector<double>>(
                         m_nLambda, std::vector<double>(m_nQ, 0.0)));
  m_errors = m_intensities;
  m_normalisation = m_intensities;
}

/**
 * @brief Q1DWeighted::getTableShapes
 * if the user provided a shape table, parse the stored values and get the
 * viewport and the sector shapes
 */
void Q1DWeighted::getTableShapes() {
  ITableWorkspace_sptr shapeWs = getProperty("TableShape");
  size_t rowCount = shapeWs->rowCount();

  std::map<std::string, std::vector<double>> viewportParams;

  // by convention, the last row is supposed to be the viewport
  getViewportParams(shapeWs->String(rowCount - 1, 1), viewportParams);

  for (size_t i = 0; i < rowCount - 1; ++i) {
    std::map<std::string, std::vector<std::string>> paramMap;
    std::vector<std::string> splitParams;
    boost::algorithm::split(splitParams, shapeWs->String(i, 1),
                            boost::algorithm::is_any_of("\n"));
    std::vector<std::string> params;

    for (std::string val : splitParams) {
      if (val.empty())
        continue;
      boost::algorithm::split(params, val, boost::algorithm::is_any_of("\t"));

      // NB : the first value of the vector also is the key, and is not a
      // meaningful value
      paramMap[params[0]] = params;
    }
    if (paramMap["Type"][1] == "sector")
      getSectorParams(paramMap["Parameters"], viewportParams);
    else
      g_log.information("Non-supported shape found in the table. Ignored.");
  }
}

/**
 * @brief Q1DWeighted::getViewportParams
 * get the parameters defining the viewport of the instrument view when the
 * shapes were created, and store them in a map
 * @param viewport the parameters as they were saved in the shape table
 * @param viewportParams the map to fill
 */
void Q1DWeighted::getViewportParams(
    std::string &viewport,
    std::map<std::string, std::vector<double>> &viewportParams) {
  std::vector<std::string> params;
  boost::algorithm::split(params, viewport,
                          boost::algorithm::is_any_of("\t, \n"));

  if (params[0] != "Translation") {
    g_log.error("No viewport found in the shape table. Please provide a table "
                "using shapes drawn in the Full3D projection.");
  }

  // Translation
  viewportParams[params[0]] = std::vector<double>(2);
  viewportParams[params[0]][0] = std::stod(params[1]);
  viewportParams[params[0]][1] = std::stod(params[2]);

  // Zoom
  viewportParams[params[3]] = std::vector<double>(1);
  viewportParams[params[3]][0] = std::stod(params[4]);

  // Rotation quaternion
  viewportParams[params[5]] = std::vector<double>(4);
  viewportParams[params[5]][0] = std::stod(params[6]);
  viewportParams[params[5]][1] = std::stod(params[7]);
  viewportParams[params[5]][2] = std::stod(params[8]);
  viewportParams[params[5]][3] = std::stod(params[9]);

  if (fabs(viewportParams["Rotation"][0]) > 1e-10 ||
      fabs(viewportParams["Rotation"][1]) > 1e-10 ||
      fabs(viewportParams["Rotation"][3]) > 1e-10 ||
      viewportParams["Rotation"][2] != 1.) {
    g_log.warning("The shapes were created using a rotated viewport not using "
                  "Z- projection, which is not supported. Results are likely "
                  "to be erroneous.");
  }
}

/**
 * @brief Q1DWeighted::getSectorParams
 * @param params the vector of strings containing the data defining the sector
 * @param viewport the previously created map of the viewport's parameters
 */
void Q1DWeighted::getSectorParams(
    std::vector<std::string> &params,
    std::map<std::string, std::vector<double>> &viewport) {
  double zoom = viewport["Zoom"][0];

  double innerRadius = std::stod(params[1]) / zoom;
  double outerRadius = std::stod(params[2]) / zoom;

  double startAngle = std::stod(params[3]);
  double endAngle = std::stod(params[4]);

  double centerAngle = (startAngle + endAngle) / 2;
  if (endAngle < startAngle)
    centerAngle = fmod(centerAngle + M_PI, 2 * M_PI);

  double angleRange = std::fmod(endAngle - startAngle, 2 * M_PI);
  angleRange = angleRange >= 0 ? angleRange : angleRange + 2 * M_PI;

  // since the viewport was in Z-, the axis are inverted so we have to take the
  // symmetry of the angle
  centerAngle = fmod(3 * M_PI - centerAngle, 2 * M_PI);

  double xOffset = viewport["Translation"][0];
  double yOffset = viewport["Translation"][1];

  double centerX = -(std::stod(params[5]) - xOffset) / zoom;
  double centerY = (std::stod(params[6]) - yOffset) / zoom;

  if (m_asymmWedges ||
      !checkIfSymetricalWedge(innerRadius, outerRadius, centerX, centerY,
                              centerAngle, angleRange)) {

    m_wedgesInnerRadius.push_back(innerRadius);
    m_wedgesOuterRadius.push_back(outerRadius);
    m_wedgesCenterAngle.push_back(centerAngle);
    m_wedgesAngleRange.push_back(angleRange);
    m_wedgesCenterX.push_back(centerX);
    m_wedgesCenterY.push_back(centerY);
    m_wedgesIsAsymmetric.push_back(true);
  }
}

/**
 * @brief Q1DWeighted::checkIfSymetricalWedge
 * Check if the symetrical wedge to the one defined by the parameters is already
 * registered in the parameter list
 * @param innerRadius the inner radius
 * @param outerRadius the outer radius
 * @param centerX the x position of the center
 * @param centerY the y position of the center
 * @param centerAngle the center of the wedge
 * @param angleRange the angular range of the wedge
 * @return true if a symetrical wedge already exists
 */
bool Q1DWeighted::checkIfSymetricalWedge(double innerRadius, double outerRadius,
                                         double centerX, double centerY,
                                         double centerAngle,
                                         double angleRange) {

  for (size_t i = 0; i < m_wedgesInnerRadius.size(); ++i) {
    double diffAngle = fabs(fmod(centerAngle - m_wedgesCenterAngle[i], M_PI));

    if (innerRadius == m_wedgesInnerRadius[i] &&
        outerRadius == m_wedgesOuterRadius[i] &&
        centerX == m_wedgesCenterX[i] && centerY == m_wedgesCenterY[i] &&
        fabs(angleRange - m_wedgesAngleRange[i]) < 1e-3 &&
        (diffAngle < 1e-3 || fabs(diffAngle - M_PI) < 1e-3)) {
      m_wedgesIsAsymmetric[i] = false;
      return true;
    }
  }
  return false;
}

/**
 * @brief Q1DWeighted::calculate
 * Performs the azimuthal averaging for each wavelength bin
 * @param inputWS : the input workspace
 */
void Q1DWeighted::calculate(const MatrixWorkspace_const_sptr &inputWS) {
  // Set up the progress
  Progress progress(this, 0.0, 1.0, m_nSpec * m_nLambda);

  const auto &spectrumInfo = inputWS->spectrumInfo();
  const V3D sourcePos = spectrumInfo.sourcePosition();
  const V3D samplePos = spectrumInfo.samplePosition();
  // Beam line axis, to compute scattering angle
  const V3D beamLine = samplePos - sourcePos;

  const auto up =
      inputWS->getInstrument()->getReferenceFrame()->vecPointingUp();

  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS))
  // first we loop over spectra
  for (int index = 0; index < static_cast<int>(m_nSpec); ++index) {
    PARALLEL_START_INTERUPT_REGION
    const auto i = static_cast<size_t>(index);
    // skip spectra with no detectors, monitors or masked spectra
    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i) ||
        spectrumInfo.isMasked(i)) {
      continue;
    }

    // store masked bins
    std::vector<size_t> maskedBins;
    // check if we have masked bins
    if (inputWS->hasMaskedBins(i)) {
      maskedBins = inputWS->maskedBinsIndices(i);
    }

    // get readonly references to the input data
    const auto &XIn = inputWS->x(i);
    const auto &YIn = inputWS->y(i);
    const auto &EIn = inputWS->e(i);

    // get the position of the pixel wrt sample (normally 0,0,0).
    const V3D pos = spectrumInfo.position(i) - samplePos;

    // prepare a gravity helper, this is much faster than calculating
    // on-the-fly, see the caching in the helper
    GravitySANSHelper gravityHelper(spectrumInfo, i, 0.0);

    // loop over lambda bins
    for (size_t j = 0; j < m_nLambda; ++j) {

      // skip if the bin is masked
      if (std::binary_search(maskedBins.cbegin(), maskedBins.cend(), j)) {
        continue;
      }

      const double wavelength = (XIn[j] + XIn[j + 1]) / 2.;

      V3D correction;
      if (m_correctGravity) {
        correction = up * gravityHelper.gravitationalDrop(wavelength);
      }

      // Each pixel might be sub-divided in the number of pixels given as
      // input parameter (NPixelDivision x NPixelDivision)
      for (int isub = 0; isub < m_nSubPixels * m_nSubPixels; ++isub) {

        // Find the position offset for this sub-pixel in real space
        const double subY =
            m_pixelSizeY *
            ((isub % m_nSubPixels) - (m_nSubPixels - 1.0) / 2.0) / m_nSubPixels;
        const double subX = m_pixelSizeX *
                            (floor(static_cast<double>(isub) / m_nSubPixels) -
                             (m_nSubPixels - 1.0) * 0.5) /
                            m_nSubPixels;

        // calculate Q
        const V3D position = pos - V3D(subX, subY, 0.0) + correction;
        const double sinTheta = sin(0.5 * position.angle(beamLine));
        const double q = 4.0 * M_PI * sinTheta / wavelength;

        if (q < m_qBinEdges.front() || q > m_qBinEdges.back()) {
          continue;
        }

        // after check above, no need to wrap this in try catch
        const size_t k = VectorHelper::indexOfValueFromEdges(m_qBinEdges, q);

        double w = 1.0;
        if (m_errorWeighting) {
          // When using the error as weight we have:
          //    w_i = 1/s_i^2   where s_i is the uncertainty on the ith
          //    pixel.
          //
          //    I(q_i) = (sum over i of I_i * w_i) / (sum over i of w_i)
          //       where all pixels i contribute to the q_i bin, and I_i is
          //       the intensity in the ith pixel.
          //
          //    delta I(q_i) = 1/sqrt( (sum over i of w_i) )  using simple
          //    error propagation.
          double err = 1.0;
          if (EIn[j] > 0)
            err = EIn[j];
          w /= m_nSubPixels * m_nSubPixels * err * err;
        }
        PARALLEL_CRITICAL(iqnorm) {
          // Fill in the data for full azimuthal integral
          m_intensities[0][j][k] += YIn[j] * w;
          m_errors[0][j][k] += w * w * EIn[j] * EIn[j];
          m_normalisation[0][j][k] += w;
        }

        for (size_t iw = 0; iw < m_nWedges; ++iw) {
          double centerAngle = m_wedgesCenterAngle[iw];
          const V3D subPix = V3D(position.X(), position.Y(), 0.0);
          const V3D center = V3D(m_wedgesCenterX[iw], m_wedgesCenterY[iw], 0);
          double angle =
              fabs((subPix - center)
                       .angle(V3D(cos(centerAngle), sin(centerAngle), 0.0)));

          if ((angle < m_wedgesAngleRange[iw] * 0.5 ||
               (!m_wedgesIsAsymmetric[iw] &&
                fabs(M_PI - angle) < m_wedgesAngleRange[iw] * 0.5)) &&
              subPix.distance(center) > m_wedgesInnerRadius[iw] &&
              (m_wedgesOuterRadius[iw] <= 0 ||
               subPix.distance(center) <= m_wedgesOuterRadius[iw])) {
            PARALLEL_CRITICAL(iqnorm_wedges) {
              // first index 0 is the full azimuth, need to offset+1
              m_intensities[iw + 1][j][k] += YIn[j] * w;
              m_errors[iw + 1][j][k] += w * w * EIn[j] * EIn[j];
              m_normalisation[iw + 1][j][k] += w;
            }
          }
        }
      }
      progress.report("Computing I(Q)");
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
} // namespace Algorithms

/**
 * @brief Q1DWeighted::finalize
 * performs final averaging and sets the output workspaces
 * @param inputWS : the input workspace
 */
void Q1DWeighted::finalize(const MatrixWorkspace_const_sptr &inputWS) {
  MatrixWorkspace_sptr outputWS =
      createOutputWorkspace(inputWS, m_nQ, m_qBinEdges);
  setProperty("OutputWorkspace", outputWS);

  // Create workspace group that holds output workspaces for wedges
  auto wsgroup = std::make_shared<WorkspaceGroup>();
  if (m_nWedges != 0) {
    // Create wedge workspaces
    for (size_t iw = 0; iw < m_nWedges; ++iw) {
      MatrixWorkspace_sptr wedgeWs =
          createOutputWorkspace(inputWS, m_nQ, m_qBinEdges);
      wedgeWs->mutableRun().addProperty(
          "wedge_angle", m_wedgesCenterAngle[iw] / deg2rad, "degrees", true);
      wsgroup->addWorkspace(wedgeWs);
    }
    // set the output property
    std::string outputWSGroupName = getPropertyValue("WedgeWorkspace");
    if (outputWSGroupName.empty()) {
      std::string outputWSName = getPropertyValue("OutputWorkspace");
      outputWSGroupName = outputWSName + "_wedges";
      setPropertyValue("WedgeWorkspace", outputWSGroupName);
    }
    setProperty("WedgeWorkspace", wsgroup);
  }

  for (size_t iout = 0; iout < m_nWedges + 1; ++iout) {

    auto ws = (iout == 0) ? outputWS
                          : std::dynamic_pointer_cast<MatrixWorkspace>(
                                wsgroup->getItem(iout - 1));
    auto &YOut = ws->mutableY(0);
    auto &EOut = ws->mutableE(0);

    std::vector<double> normLambda(m_nQ, 0.0);

    for (size_t il = 0; il < m_nLambda; ++il) {
      PARALLEL_FOR_IF(Kernel::threadSafe(*ws))
      for (int iq = 0; iq < static_cast<int>(m_nQ); ++iq) {
        PARALLEL_START_INTERUPT_REGION
        const double norm = m_normalisation[iout][il][iq];
        if (norm != 0.) {
          YOut[iq] += m_intensities[iout][il][iq] / norm;
          EOut[iq] += m_errors[iout][il][iq] / (norm * norm);
          normLambda[iq] += 1.;
        }
        PARALLEL_END_INTERUPT_REGION
      }
      PARALLEL_CHECK_INTERUPT_REGION
    }

    for (size_t i = 0; i < m_nQ; ++i) {
      YOut[i] /= normLambda[i];
      EOut[i] = sqrt(EOut[i]) / normLambda[i];
    }
  }
}

/**
 * @brief Q1DWeighted::createOutputWorkspace
 * @param parent : the parent workspace
 * @param nBins : number of bins in the histograms
 * @param binEdges : bin edges
 * @return output I(Q) workspace
 */
MatrixWorkspace_sptr
Q1DWeighted::createOutputWorkspace(const MatrixWorkspace_const_sptr &parent,
                                   const size_t nBins,
                                   const std::vector<double> &binEdges) {

  MatrixWorkspace_sptr outputWS =
      WorkspaceFactory::Instance().create(parent, 1, nBins + 1, nBins);
  outputWS->getAxis(0)->unit() =
      UnitFactory::Instance().create("MomentumTransfer");
  outputWS->setBinEdges(0, binEdges);
  outputWS->setYUnitLabel("1/cm");
  outputWS->setDistribution(true);
  return outputWS;
}

} // namespace Algorithms
} // namespace Mantid
