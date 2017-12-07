#include "MantidAlgorithms/SumOverlappingTubes.h"

#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(SumOverlappingTubes)

using namespace API;
using namespace Geometry;
using namespace HistogramData;
using namespace DataObjects;
using namespace Kernel;

void SumOverlappingTubes::init() {
  declareProperty(make_unique<ArrayProperty<std::string>>(
                      "InputWorkspaces", boost::make_shared<ADSValidator>()),
                  "The names of the input workspaces as a list. You may also "
                  "group workspaces using the GUI or [[GroupWorkspaces]], and "
                  "specify the name of the group instead.");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace.");
  std::vector<std::string> outputTypes{"2D", "2DStraight", "1DStraight"};
  declareProperty("OutputType", "2D",
                  boost::make_shared<StringListValidator>(outputTypes),
                  "Whether to have the output in 2D, 2D with straightened "
                  "Debye-Scherrer cones, or 1D.");
  declareProperty(
      make_unique<ArrayProperty<double>>(
          "ScatteringAngleBinning", "0.05",
          boost::make_shared<RebinParamsValidator>(), Direction::Input),
      "A comma separated list of the first scattering angle, the scattering "
      "angle step size and the final scattering angle. Optionally this can "
      "also be a single number, which is the angle step size. In this case, "
      "the boundary of binning will be determined by minimum and maximum "
      "scattering angle present in the workspaces.");
  declareProperty(
      make_unique<PropertyWithValue<bool>>("CropNegativeScatteringAngles",
                                           false, Direction::Input),
      "If true the negative scattering angles are cropped (ignored).");
  declareProperty(make_unique<PropertyWithValue<std::string>>(
                      "ComponentForHeightAxis", "tube_1", Direction::Input),
                  "The name of the component to use for the height axis, that "
                  "is the name of a PSD tube to be used. If specifying this "
                  "then there is no need to give a value for the HeightBinning "
                  "option.");
  declareProperty(
      make_unique<ArrayProperty<double>>(
          "HeightAxis", boost::make_shared<RebinParamsValidator>(true, true)),
      "A comma separated list of the first y value, the y value step size and "
      "the final y value. This can also be a single number, which "
      "is the y value step size. In this case, the boundary of binning will "
      "be determined by minimum and maximum y values present in the "
      "workspaces. For the 1DStraight case only this can also be two numbers, "
      "to give the range desired.");
  declareProperty(
      make_unique<PropertyWithValue<bool>>("Normalise", true, Direction::Input),
      "If true normalise to the number of entries added for a particular "
      "scattering angle. If the maximum entries accross all the scattering "
      "angles is N_MAX, and the number of entries for a scattering angle is N, "
      "the normalisation is performed as N_MAX / N.");
  auto toleranceValidator =
      boost::make_shared<BoundedValidator<double>>(0.0, 0.0);
  toleranceValidator->clearUpper();
  declareProperty("ScatteringAngleTolerance", 0.0, toleranceValidator,
                  "The relative tolerance for the scattering angles before the "
                  "counts are split.");
}

std::map<std::string, std::string> SumOverlappingTubes::validateInputs() {
  std::map<std::string, std::string> result;

  const std::string componentForHeightAxis =
      getProperty("ComponentForHeightAxis");
  const std::string heightAxis = getProperty("HeightAxis");

  if (componentForHeightAxis.empty() && heightAxis.empty()) {
    std::string message =
        "Either a component, such as a tube, must be specified "
        "to get the height axis, or the binning given explicitly.";
    result["ComponentForHeightAxis"] = message;
    result["HeightBinning"] = message;
  }

  return result;
}

void SumOverlappingTubes::exec() {
  getInputParameters();

  HistogramData::Points x(m_numPoints, LinearGenerator(m_startScatteringAngle,
                                                       m_stepScatteringAngle));

  MatrixWorkspace_sptr outputWS = create<Workspace2D>(m_numHistograms, x);

  const auto newAxis = new NumericAxis(m_heightAxis);
  newAxis->setUnit("Label");
  auto yLabelUnit =
      boost::dynamic_pointer_cast<Kernel::Units::Label>(newAxis->unit());
  yLabelUnit->setLabel("Height", "m");
  newAxis->unit() = yLabelUnit;
  outputWS->replaceAxis(1, newAxis);

  outputWS->getAxis(0)->unit() =
      Kernel::UnitFactory::Instance().create("Label");
  Unit_sptr xUnit = outputWS->getAxis(0)->unit();
  boost::shared_ptr<Units::Label> xLabel =
      boost::dynamic_pointer_cast<Units::Label>(xUnit);
  xLabel->setLabel("Tube Angle", "degrees");

  const auto normalisation = performBinning(outputWS);

  auto maxEntry = 0.0;
  for (const auto &vector : normalisation)
    maxEntry =
        std::max(*std::max_element(vector.begin(), vector.end()), maxEntry);
  if (getProperty("Normalise"))
    for (size_t i = 0; i < m_numPoints; ++i)
      for (size_t j = 0; j < m_numHistograms; ++j) {
        // Avoid spurious normalisation for low counting cells
        if (normalisation[j][i] < 1e-15)
          continue;
        outputWS->mutableY(j)[i] *= maxEntry / normalisation[j][i];
        outputWS->mutableE(j)[i] *= maxEntry / normalisation[j][i];
      }

  setProperty("OutputWorkspace", outputWS);
}

void SumOverlappingTubes::getInputParameters() {
  const std::vector<std::string> inputWorkspaces =
      getProperty("InputWorkspaces");
  auto workspaces = RunCombinationHelper::unWrapGroups(inputWorkspaces);
  RunCombinationHelper combHelper;
  m_workspaceList = combHelper.validateInputWorkspaces(workspaces, g_log);

  m_outputType = getPropertyValue("OutputType");

  // For D2B at the ILL the detectors are flipped when comparing with other
  // powder diffraction instruments such as D20. It is still desired to show
  // angles as positive however, so here we check if we need to multiple angle
  // calculations by -1.
  m_mirrorDetectors = 1;
  auto mirrorDetectors =
      m_workspaceList.front()->getInstrument()->getBoolParameter(
          "mirror_detector_angles");
  if (!mirrorDetectors.empty() && mirrorDetectors[0])
    m_mirrorDetectors = -1;

  getScatteringAngleBinning();
  getHeightAxis();
}

void SumOverlappingTubes::getScatteringAngleBinning() {
  m_startScatteringAngle = 180.0;
  m_endScatteringAngle = 0.0;

  // Loop to check minimum and maximum extents for workspace
  for (auto &ws : m_workspaceList) {
    const auto &specInfo = ws->spectrumInfo();
    for (size_t i = 0; i < specInfo.size(); ++i) {
      if (specInfo.isMonitor(i))
        continue;
      const auto &pos = specInfo.position(i);
      double thetaAngle =
          m_mirrorDetectors * atan2(pos.X(), pos.Z()) * 180.0 / M_PI;
      m_startScatteringAngle = std::min(m_startScatteringAngle, thetaAngle);
      m_endScatteringAngle = std::max(m_endScatteringAngle, thetaAngle);
    }
  }

  std::vector<double> scatteringBinning = getProperty("ScatteringAngleBinning");
  if (scatteringBinning.size() == 1) {
    m_stepScatteringAngle = scatteringBinning[0];

    // Round to the nearest m_stepScatteringAngle, normally detectors will be
    // aiming for this.
    const auto roundingFactor = 1.0 / m_stepScatteringAngle;
    m_startScatteringAngle =
        std::round(m_startScatteringAngle * roundingFactor) / roundingFactor;
    m_endScatteringAngle =
        std::round(m_endScatteringAngle * roundingFactor) / roundingFactor;
  } else if (scatteringBinning.size() == 3) {
    if (scatteringBinning[0] > m_startScatteringAngle ||
        scatteringBinning[2] < m_endScatteringAngle)
      g_log.warning() << "Some detectors outside of scattering angle range.\n";
    m_startScatteringAngle = scatteringBinning[0];
    m_stepScatteringAngle = scatteringBinning[1];
    m_endScatteringAngle = scatteringBinning[2];
  }

  if (getProperty("CropNegativeScatteringAngles") && m_startScatteringAngle < 0)
    m_startScatteringAngle = 0.0;

  m_numPoints = int(ceil((m_endScatteringAngle - m_startScatteringAngle) /
                         m_stepScatteringAngle)) +
                1;
  g_log.information() << "Number of bins in output workspace:" << m_numPoints
                      << std::endl;
  g_log.information() << "Scattering angle binning:" << m_startScatteringAngle
                      << ", " << m_stepScatteringAngle << ", "
                      << m_endScatteringAngle << "\n";
}

void SumOverlappingTubes::getHeightAxis() {
  const std::string componentName = getProperty("ComponentForHeightAxis");
  std::vector<double> heightBinning = getProperty("HeightAxis");
  if (componentName.length() > 0 && heightBinning.empty()) {
    // Try to get the component. It should be a tube with pixels in the
    // y-direction, the height bins are then taken as the detector positions.
    const auto &ws = m_workspaceList.front();
    const auto &inst = ws->getInstrument()->baseInstrument();
    const auto comp = inst->getComponentByName(componentName);
    if (!comp)
      throw std::runtime_error("Component " + componentName +
                               " could not be found.");
    const auto &compAss = dynamic_cast<const ICompAssembly &>(*comp);
    std::vector<IComponent_const_sptr> children;
    compAss.getChildren(children, false);
    for (const auto &thing : children)
      m_heightAxis.push_back(thing->getPos().Y());
  } else {
    if (heightBinning.size() != 3) {
      if (heightBinning.size() == 2 && m_outputType == "1DStraight") {
        m_heightAxis.push_back(heightBinning[0]);
        m_heightAxis.push_back(heightBinning[1]);
      } else
        throw std::runtime_error("Height binning must have start, step and end "
                                 "values (except for 1DStraight option).");
    } else if (m_outputType == "1DStraight") {
      m_heightAxis.push_back(heightBinning[0]);
      m_heightAxis.push_back(heightBinning[2]);
    } else {
      double height = heightBinning[0];
      while (height < heightBinning[2]) {
        m_heightAxis.push_back(height);
        height += heightBinning[1];
      }
    }
  }

  m_startHeight = *min_element(m_heightAxis.begin(), m_heightAxis.end());
  m_endHeight = *max_element(m_heightAxis.begin(), m_heightAxis.end());

  if (m_outputType == "1DStraight")
    m_heightAxis = {(m_heightAxis.front() + m_heightAxis.back()) * 0.5};

  m_numHistograms = m_heightAxis.size();

  g_log.information() << "Number of histograms in output workspace:"
                      << m_numHistograms << ".\n";
  g_log.information() << "Height axis:" << m_heightAxis[0] << " to "
                      << m_heightAxis[m_numHistograms - 1] << " with "
                      << m_heightAxis.size() << " entries.\n";
}

std::vector<std::vector<double>>
SumOverlappingTubes::performBinning(MatrixWorkspace_sptr &outputWS) {
  const double scatteringAngleTolerance =
      getProperty("ScatteringAngleTolerance");

  std::vector<std::vector<double>> normalisation(
      m_numHistograms, std::vector<double>(m_numPoints, 0.0));

  // loop over all workspaces
  for (auto &ws : m_workspaceList) {
    // loop over spectra
    const auto &specInfo = ws->spectrumInfo();
    for (size_t i = 0; i < specInfo.size(); ++i) {
      if (specInfo.isMonitor(i))
        continue;

      const auto &pos = specInfo.position(i);
      const auto height = pos.Y();

      const double tolerance = 1e-6;
      if (height < m_startHeight - tolerance ||
          height > m_endHeight + tolerance)
        continue;

      size_t heightIndex;
      try {
        heightIndex =
            Kernel::VectorHelper::indexOfValueFromCenters(m_heightAxis, height);
      } catch (std::out_of_range &) {
        continue;
      }

      double angle;
      if (m_outputType == "2D")
        angle = atan2(pos.X(), pos.Z());
      else
        angle = specInfo.signedTwoTheta(i);
      angle *= m_mirrorDetectors * 180.0 / M_PI;

      int angleIndex =
          int((angle - m_startScatteringAngle) / m_stepScatteringAngle + 0.5);

      // point is out of range, a warning should have been generated already for
      // the theta index
      if (angleIndex < 0 || angleIndex >= int(m_numPoints))
        continue;

      const double deltaAngle = distanceFromAngle(angleIndex, angle);
      auto counts = ws->histogram(i).y()[0];
      auto error = ws->histogram(i).e()[0];
      auto &yData = outputWS->mutableY(heightIndex);
      auto &eData = outputWS->mutableE(heightIndex);

      // counts are split between bins if outside this tolerance
      if (deltaAngle > m_stepScatteringAngle * scatteringAngleTolerance) {
        g_log.debug() << "Splitting counts for workspace " << ws->getName()
                      << " at spectrum " << i << " for angle " << angle
                      << ".\n";

        int angleIndexNeighbor;
        if (distanceFromAngle(angleIndex - 1, angle) <
            distanceFromAngle(angleIndex + 1, angle))
          angleIndexNeighbor = angleIndex - 1;
        else
          angleIndexNeighbor = angleIndex + 1;

        double deltaAngleNeighbor =
            distanceFromAngle(angleIndexNeighbor, angle);

        const auto scalingFactor = deltaAngleNeighbor / m_stepScatteringAngle;
        const auto newError = error * scalingFactor;
        yData[angleIndex] += counts * scalingFactor;
        eData[angleIndex] =
            sqrt(eData[angleIndex] * eData[angleIndex] + newError * newError);

        normalisation[heightIndex][angleIndex] +=
            (deltaAngleNeighbor / m_stepScatteringAngle);

        if (angleIndexNeighbor >= 0 && angleIndexNeighbor < int(m_numPoints)) {
          const auto scalingFactorNeighbor = deltaAngle / m_stepScatteringAngle;
          const auto newErrorNeighbor = error * scalingFactorNeighbor;
          yData[angleIndexNeighbor] += counts * scalingFactorNeighbor;
          eData[angleIndexNeighbor] =
              sqrt(eData[angleIndexNeighbor] * eData[angleIndexNeighbor] +
                   newErrorNeighbor * newErrorNeighbor);
          normalisation[heightIndex][angleIndexNeighbor] +=
              (deltaAngle / m_stepScatteringAngle);
        }
      } else {
        yData[angleIndex] += counts;
        eData[angleIndex] =
            sqrt(eData[angleIndex] * eData[angleIndex] + error * error);
        normalisation[heightIndex][angleIndex]++;
      }
    }
  }

  return normalisation;
}

double SumOverlappingTubes::distanceFromAngle(const int angleIndex,
                                              const double angle) const {
  return fabs(m_startScatteringAngle +
              double(angleIndex) * m_stepScatteringAngle - angle);
}

} // namespace Algorithms
} // namespace Mantid
