// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SumOverlappingTubes.h"

#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

#include <boost/math/special_functions/round.hpp>

namespace Mantid::Algorithms {

DECLARE_ALGORITHM(SumOverlappingTubes)

using namespace API;
using namespace Geometry;
using namespace HistogramData;
using namespace DataObjects;
using namespace Kernel;

void SumOverlappingTubes::init() {
  declareProperty(std::make_unique<ArrayProperty<std::string>>("InputWorkspaces", std::make_shared<ADSValidator>()),
                  "The names of the input workspaces as a list. You may also "
                  "group workspaces using the GUI or [[GroupWorkspaces]], and "
                  "specify the name of the group instead.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace.");
  std::vector<std::string> outputTypes{"2DTubes", "2D", "1D"};
  declareProperty("OutputType", "2D", std::make_shared<StringListValidator>(outputTypes),
                  "Whether to have the output in raw 2D, with no "
                  "Debye-Scherrer cone correction, 2D or 1D.");
  declareProperty(std::make_unique<ArrayProperty<double>>("ScatteringAngleBinning", "0.05",
                                                          std::make_shared<RebinParamsValidator>(), Direction::Input),
                  "A comma separated list of the first scattering angle, the scattering "
                  "angle step size and the final scattering angle. Optionally this can "
                  "also be a single number, which is the angle step size. In this case, "
                  "the boundary of binning will be determined by minimum and maximum "
                  "scattering angle present in the workspaces.");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("CropNegativeScatteringAngles", false, Direction::Input),
                  "If true the negative scattering angles are cropped (ignored).");
  declareProperty(
      std::make_unique<ArrayProperty<double>>("HeightAxis", std::make_shared<RebinParamsValidator>(true, true)),
      "A comma separated list of the first y value, the y value step size and "
      "the final y value. This can also be a single number, which "
      "is the y value step size. In this case, the boundary of binning will "
      "be determined by minimum and maximum y values present in the "
      "workspaces. This can also be two numbers to give the range desired.");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("Normalise", true, Direction::Input),
                  "If true normalise to the number of entries added for a particular "
                  "scattering angle. ");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("MirrorScatteringAngles", false, Direction::Input),
                  "A flag to mirror the signed 2thetas. ");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("SplitCounts", false, Direction::Input),
                  "A flag to split the counts between adjacent bins");
  auto toleranceValidator = std::make_shared<BoundedValidator<double>>(0.0, 0.0);
  toleranceValidator->clearUpper();
  declareProperty("ScatteringAngleTolerance", 0.0, toleranceValidator,
                  "The relative tolerance for the scattering angles before the "
                  "counts are split.");
  setPropertySettings("ScatteringAngleTolerance",
                      std::make_unique<Kernel::EnabledWhenProperty>("SplitCounts", IS_NOT_DEFAULT));
}

void SumOverlappingTubes::exec() {
  getInputParameters();

  m_progress = std::make_unique<Progress>(this, 0.0, 1.0, m_workspaceList.size());

  // we need histogram data with m_numPoints bins
  HistogramData::BinEdges x(m_numPoints + 1, LinearGenerator(m_startScatteringAngle, m_stepScatteringAngle));

  MatrixWorkspace_sptr outputWS = create<Workspace2D>(m_numHistograms, x);
  outputWS->setDistribution(false);
  outputWS->setSharedRun(m_workspaceList.front()->sharedRun());

  auto newAxis = std::make_unique<NumericAxis>(m_heightAxis);
  newAxis->setUnit("Label");
  auto yLabelUnit = std::dynamic_pointer_cast<Kernel::Units::Label>(newAxis->unit());
  yLabelUnit->setLabel("Height", "m");
  newAxis->unit() = yLabelUnit;
  outputWS->replaceAxis(1, std::move(newAxis));

  outputWS->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("Label");
  Unit_sptr xUnit = outputWS->getAxis(0)->unit();
  std::shared_ptr<Units::Label> xLabel = std::dynamic_pointer_cast<Units::Label>(xUnit);
  xLabel->setLabel("Scattering Angle", "degrees");

  const auto normalisation = performBinning(outputWS);

  if (getProperty("Normalise")) {
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int64_t ii = 0; ii < static_cast<int64_t>(m_numPoints); ++ii)
      for (size_t j = 0; j < m_numHistograms; ++j) {
        // Avoid spurious normalisation for low counting cells
        const auto i = static_cast<size_t>(ii);
        if (normalisation[j][i] < 1e-15)
          continue;
        outputWS->mutableY(j)[i] /= normalisation[j][i];
        outputWS->mutableE(j)[i] /= normalisation[j][i];
      }
  }

  setProperty("OutputWorkspace", outputWS);
}

void SumOverlappingTubes::getInputParameters() {

  // This is flag for flipping the sign of 2theta
  m_mirrorDetectors = getProperty("MirrorScatteringAngles") ? -1 : 1;

  const std::vector<std::string> inputWorkspaces = getProperty("InputWorkspaces");
  auto workspaces = RunCombinationHelper::unWrapGroups(inputWorkspaces);
  RunCombinationHelper combHelper;
  m_workspaceList = combHelper.validateInputWorkspaces(workspaces, g_log);
  m_outputType = getPropertyValue("OutputType");
  const auto &instrument = m_workspaceList.front()->getInstrument();
  std::string componentName = "";
  auto componentNameParam = instrument->getStringParameter("detector_for_height_axis");
  if (!componentNameParam.empty())
    componentName = componentNameParam[0];
  getScatteringAngleBinning();
  getHeightAxis(componentName);
}

void SumOverlappingTubes::getScatteringAngleBinning() {
  m_startScatteringAngle = 180.0;
  m_endScatteringAngle = -180.0;

  // Loop to check minimum and maximum extents for workspace
  for (auto &ws : m_workspaceList) {
    const auto &specInfo = ws->spectrumInfo();
    for (size_t i = 0; i < specInfo.size(); ++i) {
      if (specInfo.isMonitor(i) || specInfo.isMasked(i))
        continue;
      const auto &pos = specInfo.position(i);
      const double theta = atan2(pos.X(), pos.Z()) * m_mirrorDetectors * 180 / M_PI;
      m_startScatteringAngle = std::min(m_startScatteringAngle, theta);
      m_endScatteringAngle = std::max(m_endScatteringAngle, theta);
    }
  }

  const std::vector<double> scatteringBinning = getProperty("ScatteringAngleBinning");
  if (scatteringBinning.size() == 1) {
    m_stepScatteringAngle = scatteringBinning[0];
    // Extend the boundaries by half of the step size
    m_startScatteringAngle -= m_stepScatteringAngle / 2.;
    m_endScatteringAngle += m_stepScatteringAngle / 2.;
  } else if (scatteringBinning.size() == 3) {
    if (scatteringBinning[0] > m_startScatteringAngle || scatteringBinning[2] < m_endScatteringAngle)
      g_log.warning() << "Some detectors outside of scattering angle range.\n";
    m_startScatteringAngle = scatteringBinning[0];
    m_stepScatteringAngle = scatteringBinning[1];
    m_endScatteringAngle = scatteringBinning[2];
  }

  if (getProperty("CropNegativeScatteringAngles")) {
    if (m_endScatteringAngle < 0) {
      throw std::runtime_error("No positive scattering angle range");
    }
    if (m_startScatteringAngle < 0) {
      m_startScatteringAngle += std::floor(-m_startScatteringAngle / m_stepScatteringAngle) * m_stepScatteringAngle;
    }
  }

  m_numPoints =
      static_cast<size_t>(std::floor((m_endScatteringAngle - m_startScatteringAngle) / m_stepScatteringAngle));
  g_log.information() << "Number of bins:" << m_numPoints << std::endl;
  g_log.information() << "Scattering angle binning:" << m_startScatteringAngle << ", " << m_stepScatteringAngle << ", "
                      << m_endScatteringAngle << "\n";

  if (m_startScatteringAngle >= m_endScatteringAngle) {
    throw std::runtime_error("Wrong scattering angle range, check your binning/data");
  }
}

void SumOverlappingTubes::getHeightAxis(const std::string &componentName) {
  std::vector<double> heightBinning = getProperty("HeightAxis");
  m_heightAxis.clear();
  if (componentName.length() == 0 && heightBinning.empty())
    throw std::runtime_error("No detector_for_height_axis parameter for this "
                             "instrument. Please enter a value for the "
                             "HeightAxis parameter.");
  if ((componentName.length() > 0 && heightBinning.empty()) || (m_outputType != "1D" && heightBinning.size() == 2)) {
    // Try to get the component. It should be a tube with pixels in the
    // y-direction, the height bins are then taken as the detector positions.
    const auto &componentInfo = m_workspaceList.front()->componentInfo();
    const auto componentIndex = componentInfo.indexOfAny(componentName);
    const auto &detsInSubtree = componentInfo.detectorsInSubtree(componentIndex);
    for (const auto detIndex : detsInSubtree) {
      const auto posY = componentInfo.position({detIndex, 0}).Y();
      if (heightBinning.size() == 2 && (posY < heightBinning[0] || posY > heightBinning[1]))
        continue;
      m_heightAxis.emplace_back(posY);
    }
  } else {
    if (heightBinning.size() != 3) {
      if (heightBinning.size() == 2 && m_outputType == "1D") {
        m_heightAxis.emplace_back(heightBinning[0]);
        m_heightAxis.emplace_back(heightBinning[1]);
      } else
        throw std::runtime_error("Height binning must have start, step and end "
                                 "values (except for 1D option).");
    } else if (m_outputType == "1D") {
      m_heightAxis.emplace_back(heightBinning[0]);
      m_heightAxis.emplace_back(heightBinning[2]);
    } else {
      double height = heightBinning[0];
      while (height < heightBinning[2]) {
        m_heightAxis.emplace_back(height);
        height += heightBinning[1];
      }
    }
  }

  if (m_heightAxis.size() == 0) {
    throw std::runtime_error("Height axis is undefined");
  }

  m_startHeight = *min_element(m_heightAxis.begin(), m_heightAxis.end());
  m_endHeight = *max_element(m_heightAxis.begin(), m_heightAxis.end());

  if (m_outputType == "1D") {
    const double singleValue = (m_heightAxis.front() + m_heightAxis.back()) * 0.5;
    m_heightAxis.clear();
    m_heightAxis.emplace_back(singleValue);
  }

  m_numHistograms = m_heightAxis.size();

  g_log.information() << "Number of histograms in output workspace:" << m_numHistograms << ".\n";
  g_log.information() << "Height axis:" << m_heightAxis[0] << " to " << m_heightAxis[m_numHistograms - 1] << " with "
                      << m_heightAxis.size() << " entries.\n";
}

std::vector<std::vector<double>> SumOverlappingTubes::performBinning(MatrixWorkspace_sptr &outputWS) {
  const double scatteringAngleTolerance = getProperty("ScatteringAngleTolerance");
  const bool splitCounts = getProperty("SplitCounts");

  std::vector<std::vector<double>> normalisation(m_numHistograms, std::vector<double>(m_numPoints, 0.0));

  // loop over all workspaces
  for (auto &ws : m_workspaceList) {
    m_progress->report("Processing workspace " + std::string(ws->getName()));
    // loop over spectra
    const auto &specInfo = ws->spectrumInfo();
    PARALLEL_FOR_IF(Kernel::threadSafe(*ws, *outputWS))
    for (int i = 0; i < static_cast<int>(specInfo.size()); ++i) {
      PARALLEL_START_INTERRUPT_REGION
      if (specInfo.isMonitor(i) || specInfo.isMasked(i))
        continue;

      const auto &pos = specInfo.position(i);
      const auto height = pos.Y();

      const double tolerance = 1e-6;
      if (height < m_startHeight - tolerance || height > m_endHeight + tolerance)
        continue;

      size_t heightIndex;
      try {
        heightIndex = Kernel::VectorHelper::indexOfValueFromCenters(m_heightAxis, height);
      } catch (std::out_of_range &) {
        continue;
      }

      double angle;
      if (m_outputType == "2DTubes")
        angle = atan2(pos.X(), pos.Z());
      else
        angle = specInfo.signedTwoTheta(i);
      angle *= m_mirrorDetectors * 180.0 / M_PI;

      const auto angleIndex = static_cast<int>(std::floor((angle - m_startScatteringAngle) / m_stepScatteringAngle));

      // point is out of range, a warning should have been generated already for
      // the theta index
      if (angleIndex < 0 || angleIndex >= int(m_numPoints))
        continue;

      const double deltaAngle = distanceFromAngle(angleIndex, angle);
      const auto counts = ws->histogram(i).y()[0];
      const auto error = ws->histogram(i).e()[0];

      PARALLEL_CRITICAL(Histogramming2ThetaVsHeight) {
        auto &yData = outputWS->mutableY(heightIndex);
        auto &eData = outputWS->mutableE(heightIndex);
        // counts are split between bins if outside this tolerance
        if (splitCounts && deltaAngle > m_stepScatteringAngle * scatteringAngleTolerance) {
          int angleIndexNeighbor;
          if (distanceFromAngle(angleIndex - 1, angle) < distanceFromAngle(angleIndex + 1, angle))
            angleIndexNeighbor = angleIndex - 1;
          else
            angleIndexNeighbor = angleIndex + 1;

          double deltaAngleNeighbor = distanceFromAngle(angleIndexNeighbor, angle);

          const auto scalingFactor = deltaAngleNeighbor / m_stepScatteringAngle;
          const auto newError = error * scalingFactor;
          yData[angleIndex] += counts * scalingFactor;
          eData[angleIndex] = sqrt(eData[angleIndex] * eData[angleIndex] + newError * newError);

          normalisation[heightIndex][angleIndex] += (deltaAngleNeighbor / m_stepScatteringAngle);

          if (angleIndexNeighbor >= 0 && angleIndexNeighbor < int(m_numPoints)) {
            const auto scalingFactorNeighbor = deltaAngle / m_stepScatteringAngle;
            const auto newErrorNeighbor = error * scalingFactorNeighbor;
            yData[angleIndexNeighbor] += counts * scalingFactorNeighbor;
            eData[angleIndexNeighbor] =
                sqrt(eData[angleIndexNeighbor] * eData[angleIndexNeighbor] + newErrorNeighbor * newErrorNeighbor);
            normalisation[heightIndex][angleIndexNeighbor] += (deltaAngle / m_stepScatteringAngle);
          }
        } else {
          yData[angleIndex] += counts;
          eData[angleIndex] = sqrt(eData[angleIndex] * eData[angleIndex] + error * error);
          normalisation[heightIndex][angleIndex]++;
        }
      }
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION
  }

  return normalisation;
}

double SumOverlappingTubes::distanceFromAngle(const int angleIndex, const double angle) const {
  return fabs(m_startScatteringAngle + double(angleIndex) * m_stepScatteringAngle - angle);
}

} // namespace Mantid::Algorithms
