#include "MantidAlgorithms/BinDetectorScan.h"

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
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(BinDetectorScan)

using namespace API;
using namespace Geometry;
using namespace HistogramData;
using namespace DataObjects;
using namespace Kernel;

void BinDetectorScan::init() {
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
          "ScatteringAngleBinning", boost::make_shared<RebinParamsValidator>()),
      "A comma separated list of the first scattering angle, the scattering "
      "angle step size and the final scattering angle. Optionally this can "
      "also be a single number, which is the angle step size. In this case, "
      "the boundary of binning will be determined by minimum and maximum "
      "scattering angle present in the workspaces.");
  declareProperty(make_unique<PropertyWithValue<std::string>>(
                      "ComponentForHeightAxis", "", Direction::Input),
                  "The name of the component to use for the height axis, that "
                  "is the name of a PSD tube to be used. If specifying this "
                  "then there is no need to give a value for the HeightBinning "
                  "option.");
  declareProperty(
      make_unique<ArrayProperty<double>>(
          "HeightBinning", boost::make_shared<RebinParamsValidator>(true)),
      "A comma separated list of the first y value, the y value step size and "
      "the final y value. Optionally this can also be a single number, which "
      "is the y value step size. In this case, the boundary of binning will "
      "be determined by minimum and maximum y values present in the "
      "workspaces.");
  auto toleranceValidator =
      boost::make_shared<BoundedValidator<double>>(0.0, 0.0);
  toleranceValidator->clearUpper();
  declareProperty("ScatteringAngleTolerance", 0.1, toleranceValidator,
                  "The relative tolerance for the scattering angles before the "
                  "counts are split.");
}

std::map<std::string, std::string> BinDetectorScan::validateInputs() {
  std::map<std::string, std::string> result;

  const std::string componentForHeightAxis =
      getProperty("ComponentForHeightAxis");
  const std::string heightBinning = getProperty("HeightBinning");

  if (componentForHeightAxis.empty() && heightBinning.empty()) {
    std::string message =
        "Either a component, such as a tube, must be specified "
        "to get the height axis, or the binning given explicitly.";
    result["ComponentForHeightAxis"] = message;
    result["HeightBinning"] = message;
  }

  return result;
}

void BinDetectorScan::exec() {
  getInputParameters();

  HistogramData::Points x(m_numPoints, LinearGenerator(m_startScatteringAngle,
                                                       m_stepScatteringAngle));

  MatrixWorkspace_sptr outputWS = create<Workspace2D>(m_numHistograms, x);

  const auto newAxis = new NumericAxis(m_heightAxis);
  newAxis->setUnit("Label");
  auto lblUnit =
      boost::dynamic_pointer_cast<Kernel::Units::Label>(newAxis->unit());
  lblUnit->setLabel("Height", "m");
  newAxis->unit() = lblUnit;
  outputWS->replaceAxis(1, newAxis);

  performBinning(outputWS);

  setProperty("OutputWorkspace", outputWS);
}

void BinDetectorScan::getInputParameters() {
  const std::vector<std::string> inputWorkspaces =
      getProperty("InputWorkspaces");
  auto workspaces = RunCombinationHelper::unWrapGroups(inputWorkspaces);
  RunCombinationHelper combHelper;
  m_workspaceList = combHelper.validateInputWorkspaces(workspaces, g_log);

  getScatteringAngleBinning();
  getHeightAxis();
}

void BinDetectorScan::getScatteringAngleBinning() {
  m_startScatteringAngle = 0;
  m_endScatteringAngle = 0;

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
  } else if (scatteringBinning.size() == 3) {
    if (scatteringBinning[0] > m_startScatteringAngle ||
        scatteringBinning[2] < m_endScatteringAngle)
      g_log.warning() << "Some detectors outside of scattering angle range.\n";
    m_startScatteringAngle = scatteringBinning[0];
    m_stepScatteringAngle = scatteringBinning[1];
    m_endScatteringAngle = scatteringBinning[2];
  }

  m_numPoints = int(ceil((m_endScatteringAngle - m_startScatteringAngle) /
                         m_stepScatteringAngle)) +
                1;
  g_log.information() << "Number of bins in output workspace:" << m_numPoints
                      << std::endl;
  g_log.information() << "Scattering angle binning:" << m_startScatteringAngle
                      << ", " << m_stepScatteringAngle << ", "
                      << m_endScatteringAngle << "\n";
}

void BinDetectorScan::getHeightAxis() {
  std::string componentName = getProperty("ComponentForHeightAxis");
  if (componentName.length() > 0) {
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
    std::vector<double> heightBinning = getProperty("HeightBinning");
    if (heightBinning.size() != 3)
      throw std::runtime_error(
          "Currently height binning must have start, step and end values.");
    double height = heightBinning[0];
    while (height < heightBinning[2]) {
      m_heightAxis.push_back(height);
      height += heightBinning[1];
    }
  }

  m_numHistograms = m_heightAxis.size();

  g_log.information() << "Number of histograms in output workspace:"
                      << m_numHistograms << std::endl;
  g_log.information() << "Height binning:" << m_heightAxis[0] << ", "
                      << m_heightAxis[m_numHistograms] << "\n";
}

void BinDetectorScan::performBinning(MatrixWorkspace_sptr &outputWS) {
  const double scatteringAngleTolerance =
      getProperty("ScatteringAngleTolerance");

  // loop over spectra
  for (auto &ws : m_workspaceList) {
    const auto &specInfo = ws->spectrumInfo();
    for (size_t i = 0; i < specInfo.size(); ++i) {
      if (specInfo.isMonitor(i))
        continue;
      const auto &pos = specInfo.position(i);
      const auto y = pos.Y();
      auto theta = -atan2(pos.X(), pos.Z()) * 180.0 / M_PI;

      auto it = std::lower_bound(m_heightAxis.begin(), m_heightAxis.end(), y);
      size_t index = std::distance(m_heightAxis.begin(), it);
      size_t yIndex =
          fabs(m_heightAxis[index] - y) < fabs(m_heightAxis[index - 1] - y)
              ? index
              : index - 1;

      // TODO: theta < 0
      size_t thetaIndex = size_t(
          (theta - m_startScatteringAngle) / m_stepScatteringAngle + 0.5);
      const double deltaX =
          fabs(m_startScatteringAngle +
               double(thetaIndex) * m_stepScatteringAngle - theta);

      if (yIndex >= m_numHistograms || thetaIndex >= m_numPoints)
        continue;
      if (deltaX > m_stepScatteringAngle * scatteringAngleTolerance) {
        const double deltaXPlus =
            fabs(m_startScatteringAngle +
                 double(thetaIndex + 1) * m_stepScatteringAngle - theta);
        const double deltaXMinus =
            fabs(m_startScatteringAngle +
                 double(thetaIndex - 1) * m_stepScatteringAngle - theta);
        size_t n;
        double deltaXOther = 0.0;
        if (deltaXMinus < deltaXPlus) {
          n = thetaIndex - 1;
          deltaXOther = deltaXMinus;
        } else {
          n = thetaIndex + 1;
          deltaXOther = deltaXPlus;
        }

        auto counts = ws->histogram(i).y()[0];
        auto &yData = outputWS->mutableY(yIndex);
        yData[n] += counts * deltaX / m_stepScatteringAngle;
        yData[thetaIndex] += counts * deltaXOther / m_stepScatteringAngle;
        g_log.error() << theta << "''" << deltaXPlus << "''" << deltaXMinus
                      << std::endl;
        g_log.error() << deltaXOther / m_stepScatteringAngle << std::endl;
        g_log.error() << deltaX / m_stepScatteringAngle << std::endl;
        g_log.error() << yData[n] << "++" << yData[thetaIndex] << std::endl;
      } else {
        auto counts = ws->histogram(i).y()[0];
        auto &yData = outputWS->mutableY(yIndex);
        yData[thetaIndex] += counts;
      }
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
