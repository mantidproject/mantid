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
  declareProperty(
      make_unique<ArrayProperty<double>>(
          "ScatteringAngleBinning", "0.05",
          boost::make_shared<RebinParamsValidator>(), Direction::Input),
      "A comma separated list of the first scattering angle, the scattering "
      "angle step size and the final scattering angle. Optionally this can "
      "also be a single number, which is the angle step size. In this case, "
      "the boundary of binning will be determined by minimum and maximum "
      "scattering angle present in the workspaces.");
  declareProperty(make_unique<PropertyWithValue<std::string>>(
                      "ComponentForHeightAxis", "tube_1", Direction::Input),
                  "The name of the component to use for the height axis, that "
                  "is the name of a PSD tube to be used. If specifying this "
                  "then there is no need to give a value for the HeightBinning "
                  "option.");
  declareProperty(
      make_unique<ArrayProperty<double>>(
          "HeightAxis", boost::make_shared<RebinParamsValidator>(true)),
      "A comma separated list of the first y value, the y value step size and "
      "the final y value. Optionally this can also be a single number, which "
      "is the y value step size. In this case, the boundary of binning will "
      "be determined by minimum and maximum y values present in the "
      "workspaces.");
  auto toleranceValidator =
      boost::make_shared<BoundedValidator<double>>(0.0, 0.0);
  toleranceValidator->clearUpper();
  declareProperty("ScatteringAngleTolerance", 0.0, toleranceValidator,
                  "The relative tolerance for the scattering angles before the "
                  "counts are split.");
  declareProperty(
      make_unique<PropertyWithValue<bool>>("Normalise", true, Direction::Input),
      "If true normalise to the number of entries added for a particular "
      "scattering angle. If the maximum entries accross all the scattering "
      "angles is N_MAX, and the number of entries for a scattering angle is N, "
      "the normalisation is performed as N_MAX / N.");
  std::vector<std::string> outputTypes{"2D", "2DStraight", "1DStraight"};
  declareProperty("OutputType", "2D",
                  boost::make_shared<StringListValidator>(outputTypes),
                  "Whether to have the output in 2D, 2D with straightened "
                  "Debye-Scherrer cones, or 1D.");
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

  const auto maxEntry =
      *std::max_element(normalisation.begin(), normalisation.end());
  if (getProperty("Normalise"))
    for (size_t i = 0; i < outputWS->spectrumInfo().size(); ++i)
      for (size_t j = 0; j < outputWS->histogram(i).size(); ++j)
        outputWS->mutableY(i)[j] *= maxEntry / normalisation[j];

  setProperty("OutputWorkspace", outputWS);
}

void SumOverlappingTubes::getInputParameters() {
  const std::vector<std::string> inputWorkspaces =
      getProperty("InputWorkspaces");
  auto workspaces = RunCombinationHelper::unWrapGroups(inputWorkspaces);
  RunCombinationHelper combHelper;
  m_workspaceList = combHelper.validateInputWorkspaces(workspaces, g_log);

  m_outputType = getPropertyValue("OutputType");

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
      double thetaAngle = -atan2(pos.X(), pos.Z()) * 180.0 / M_PI;
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

  if ((m_outputType == "2DStraight" || m_outputType == "1DStraight") &&
      m_startScatteringAngle < 0)
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
    if (heightBinning.size() != 3)
      throw std::runtime_error(
          "Height binning must have start, step and end values.");
    if (m_outputType == "1DStraight") {
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

std::vector<double>
SumOverlappingTubes::performBinning(MatrixWorkspace_sptr &outputWS) {
  const double scatteringAngleTolerance =
      getProperty("ScatteringAngleTolerance");

  std::vector<double> normalisation(m_numPoints, 0.0);

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
        angle = -atan2(pos.X(), pos.Z());
      else
        angle = specInfo.twoTheta(i);
      angle *= 180.0 / M_PI;

      int angleIndex =
          int((angle - m_startScatteringAngle) / m_stepScatteringAngle + 0.5);

      // point is out of range, a warning should have been generated already for
      // the theta index
      if (angleIndex < 0 || angleIndex >= int(m_numPoints))
        continue;

      const double deltaAngle = distanceFromAngle(angleIndex, angle);
      auto counts = ws->histogram(i).y()[0];
      auto &yData = outputWS->mutableY(heightIndex);

      if (deltaAngle > m_stepScatteringAngle * scatteringAngleTolerance) {
        // counts are split between bins if outside this tolerance

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

        yData[angleIndex] +=
            counts * deltaAngleNeighbor / m_stepScatteringAngle;

        if (heightIndex == 0)
          normalisation[angleIndex] +=
              (deltaAngleNeighbor / m_stepScatteringAngle);

        if (angleIndexNeighbor >= 0 && angleIndexNeighbor < int(yData.size())) {
          yData[angleIndexNeighbor] +=
              counts * deltaAngle / m_stepScatteringAngle;
          if (heightIndex == 0)
            normalisation[angleIndexNeighbor] +=
                (deltaAngle / m_stepScatteringAngle);
        }
      } else {
        yData[angleIndex] += counts;
        if (heightIndex == 0)
          normalisation[angleIndex]++;
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
