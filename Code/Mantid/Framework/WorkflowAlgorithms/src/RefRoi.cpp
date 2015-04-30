//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/RefRoi.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/UnitFactory.h"
#include "Poco/String.h"

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RefRoi)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void RefRoi::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input,
                              boost::make_shared<CommonBinsValidator>()),
      "Workspace to calculate the ROI from");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "Workspace containing the summed up region of interest");
  declareProperty("NXPixel", 304, "Number of pixels in the X direction",
                  Kernel::Direction::Input);
  declareProperty("NYPixel", 256, "Number of pixels in the Y direction",
                  Kernel::Direction::Input);
  declareProperty("XPixelMin", EMPTY_INT(), "Lower bound of ROI in X",
                  Kernel::Direction::Input);
  declareProperty("XPixelMax", EMPTY_INT(), "Upper bound of ROI in X",
                  Kernel::Direction::Input);
  declareProperty("YPixelMin", EMPTY_INT(), "Lower bound of ROI in Y",
                  Kernel::Direction::Input);
  declareProperty("YPixelMax", EMPTY_INT(), "Upper bound of ROI in Y",
                  Kernel::Direction::Input);

  declareProperty(
      "SumPixels", false,
      "If true, all the pixels will be summed,"
      " so that the resulting workspace will be a single histogram");
  declareProperty(
      "NormalizeSum", false,
      "If true, and SumPixels is true, the"
      " resulting histogram will be divided by the number of pixels in the ROI");
  declareProperty(
      "AverageOverIntegratedAxis", false,
      "If true, and SumPixels and NormalizeSum are true, the"
      " resulting histogram will also be divided by the number of pixels integrated over");
  declareProperty(
      "ErrorWeighting", false,
      "If true, error weighting will be used when normalizing");
  declareProperty(
      "IntegrateY", true,
      "If true, the Y direction will be"
      " considered the low-resolution direction and will be integrated over."
      " If false, the X direction will be integrated over. The result will be"
      " a histogram for each of the pixels in the hi-resolution direction of"
      " the 2D detector");
  declareProperty("ConvertToQ", true, "If true, the X-axis will be converted"
                                      " to momentum transfer");
  declareProperty("ScatteringAngle", 0.0, "Value of the scattering angle to use"
                                          " when converting to Q");
}

/// Execute algorithm
void RefRoi::exec() {
  // Get the input workspace
  const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  // Bin boundaries need to be the same, so do the full check on whether they
  // actually are
  if (!API::WorkspaceHelpers::commonBoundaries(inputWS)) {
    g_log.error()
        << "Can only group if the histograms have common bin boundaries\n";
    throw std::invalid_argument(
        "Can only group if the histograms have common bin boundaries");
  }

  // Detector size
  m_nXPixel = getProperty("NXPixel");
  m_nYPixel = getProperty("NYPixel");

  // ROI
  m_xMin = getProperty("XPixelMin");
  if (isEmpty(m_xMin) || m_xMin < 0)
    m_xMin = 0;
  m_xMax = getProperty("XPixelMax");
  if (isEmpty(m_xMax) || m_xMax > m_nXPixel - 1)
    m_xMax = m_nXPixel - 1;

  m_yMin = getProperty("YPixelMin");
  if (isEmpty(m_yMin) || m_yMin < 0)
    m_yMin = 0;
  m_yMax = getProperty("YPixelMax");
  if (isEmpty(m_yMax) || m_yMax > m_nYPixel - 1)
    m_yMax = m_nYPixel - 1;

  extract2D();
}

void RefRoi::extract2D() {
  const MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  bool convert_to_q = getProperty("ConvertToQ");
  double theta = getProperty("ScatteringAngle");
  bool integrate_y = getProperty("IntegrateY");
  bool sum_pixels = getProperty("SumPixels");
  bool normalize = getProperty("NormalizeSum");
  bool error_weighting = getProperty("ErrorWeighting");

  int nHisto = integrate_y ? m_nXPixel : m_nYPixel;
  int xmin = integrate_y ? 0 : m_xMin;
  int xmax = integrate_y ? m_nXPixel - 1 : m_xMax;
  int ymin = integrate_y ? m_yMin : 0;
  int ymax = integrate_y ? m_yMax : m_nYPixel - 1;

  if (sum_pixels) {
    nHisto = 1;
    if (integrate_y) {
      xmin = m_xMin;
      xmax = m_xMax;
    } else {
      ymin = m_yMin;
      ymax = m_yMax;
    }
  }

  // Create output workspace
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(
      inputWS, nHisto, inputWS->readX(0).size(), inputWS->blocksize());

  // Process X axis
  MantidVec &XOut0 = outputWS->dataX(0);
  const MantidVec &XIn0 = inputWS->readX(0);
  if (convert_to_q) {
    // Check that the X-axis is in wavelength units
    const std::string unit = inputWS->getAxis(0)->unit()->caption();
    if (Poco::icompare(unit, "Wavelength") != 0) {
      g_log.error() << "RefRoi expects units of wavelength to convert to Q"
                    << std::endl;
      throw std::runtime_error(
          "RefRoi expects units of wavelength to convert to Q");
    }

    for (size_t t = 0; t < XOut0.size(); t++) {
      size_t t_index = XIn0.size() - 1 - t;
      XOut0[t] = 4.0 * M_PI * sin(theta * M_PI / 180.0) / XIn0[t_index];
    }
    outputWS->getAxis(0)->unit() =
        UnitFactory::Instance().create("MomentumTransfer");
    outputWS->setYUnitLabel("Reflectivity");
    outputWS->isDistribution(true);
  } else {
    XOut0 = inputWS->readX(0);
  }

  // Make sure the inner loop is always the one we integrate over
  int main_axis_min = integrate_y ? xmin : ymin;
  int main_axis_max = integrate_y ? xmax : ymax;
  int integrated_axis_min = integrate_y ? ymin : xmin;
  int integrated_axis_max = integrate_y ? ymax : xmax;

  for (int i = main_axis_min; i <= main_axis_max; i++) {
    size_t output_index = i;
    if (sum_pixels)
      output_index = 0;

    MantidVec &YOut = outputWS->dataY(output_index);
    MantidVec &EOut = outputWS->dataE(output_index);
    MantidVec signal_vector(YOut.size(), 0.0);
    MantidVec error_vector(YOut.size(), 0.0);

    for (int j = integrated_axis_min; j <= integrated_axis_max; j++) {
      int index = integrate_y ? m_nYPixel * i + j : m_nYPixel * j + i;
      const MantidVec &YIn = inputWS->readY(index);
      const MantidVec &EIn = inputWS->readE(index);

      for (size_t t = 0; t < YOut.size(); t++) {
        size_t t_index = convert_to_q ? YOut.size() - 1 - t : t;
        if (sum_pixels && normalize && error_weighting) {
          signal_vector[t] += YIn[t_index];
          error_vector[t] += EIn[t_index] * EIn[t_index];
        } else {
          YOut[t] += YIn[t_index];
          EOut[t] += EIn[t_index] * EIn[t_index];
        }
      }
    }

    if (sum_pixels && normalize && error_weighting) {
      for (size_t t = 0; t < YOut.size(); t++) {
        size_t t_index = convert_to_q ? YOut.size() - 1 - t : t;
        double error_squared = error_vector[t_index] == 0 ? 1 : error_vector[t_index];
        YOut[t] += signal_vector[t_index] / error_squared;
        EOut[t] += 1.0 / error_squared;
      }
    }
  }

  // Check whether we want to divide by the number of pixels along
  // the axis we integrated over.
  bool average_integrated = getProperty("AverageOverIntegratedAxis");
  double n_integrated = 1.0;
  if (sum_pixels && normalize && average_integrated) {
    n_integrated = integrated_axis_max - integrated_axis_min + 1;
  }

  for (int i = 0; i < nHisto; i++) {
    outputWS->dataX(i) = XOut0;
    MantidVec &YOut = outputWS->dataY(i);
    MantidVec &EOut = outputWS->dataE(i);
    for (size_t t = 0; t < EOut.size(); t++) {
      if (sum_pixels && normalize) {
        if (error_weighting) {
          YOut[t] = YOut[t] / EOut[t] / n_integrated;
          EOut[t] = sqrt(1.0/EOut[t]) / n_integrated;
        } else {
          EOut[t] = sqrt(EOut[t]);
          YOut[t] = YOut[t] / (main_axis_max - main_axis_min + 1) / n_integrated;
          EOut[t] = EOut[t] / (main_axis_max - main_axis_min + 1) / n_integrated;
        }
      } else {
        EOut[t] = sqrt(EOut[t]);
      };
    }
  }

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
