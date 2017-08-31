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
  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
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
  declareProperty(
      make_unique<ArrayProperty<double>>(
          "HeightBinning", boost::make_shared<RebinParamsValidator>()),
      "A comma separated list of the first y value, the y value step size and "
      "the final y value. Optionally this can also be a single number, which "
      "is the y value step size. In this case, the boundary of binning will "
      "be determined by minimum and maximum y values present in the "
      "workspaces.");
}

void BinDetectorScan::exec() {
  getInputParameters();

  size_t numBins = int(ceil((m_endScatteringAngle - m_startScatteringAngle) /
                            m_stepScatteringAngle));
  g_log.information() << "Number of bins in output workspace:" << numBins
                      << std::endl;
  HistogramData::BinEdges x(
      numBins + 1,
      LinearGenerator(m_startScatteringAngle, m_stepScatteringAngle));
  size_t numHistograms =
      int(ceil((m_endHeight - m_startHeight) / m_stepHeight));
  g_log.information() << "Number of histograms in output workspace:"
                      << numHistograms << std::endl;
  MatrixWorkspace_sptr outputWS = create<Workspace2D>(numHistograms, x);

  std::vector<double> yAxis;
  for (size_t i = 0; i < numHistograms; i++)
    yAxis.push_back(m_startHeight + m_stepHeight * (double(i) + 0.5));

  const auto newAxis = new NumericAxis(yAxis);
  newAxis->setUnit("Label");
  auto lblUnit =
      boost::dynamic_pointer_cast<Kernel::Units::Label>(newAxis->unit());
  lblUnit->setLabel("Height", "m");
  newAxis->unit() = lblUnit;
  outputWS->replaceAxis(1, newAxis);

  // loop over spectra
  for (auto &ws : m_workspaceList) {
    const auto &specInfo = ws->spectrumInfo();
    for (size_t i = 0; i < specInfo.size(); ++i) {
      if (specInfo.isMonitor(i))
        continue;
      const auto &pos = specInfo.position(i);
      const auto y = pos.Y();
      auto theta = atan2(pos.X(), pos.Z()) * 180.0 / M_PI;
      size_t yIndex = size_t((y - m_startHeight) / m_stepHeight);
      size_t thetaIndex =
          size_t((theta - m_startScatteringAngle) / m_stepScatteringAngle);
      if (yIndex > numHistograms || thetaIndex > numBins) {
        continue;
      }
      auto counts = ws->histogram(i).y()[0];
      auto &yData = outputWS->mutableY(yIndex);
      yData[thetaIndex] += counts;
    }
  }
  setProperty("OutputWorkspace", outputWS);
}

void BinDetectorScan::getInputParameters() {
  const std::vector<std::string> inputWorkspaces =
      getProperty("InputWorkspaces");
  auto workspaces = RunCombinationHelper::unWrapGroups(inputWorkspaces);
  RunCombinationHelper combHelper;
  m_workspaceList = combHelper.validateInputWorkspaces(workspaces, g_log);

  m_startScatteringAngle = 0;
  m_endScatteringAngle = 0;

  // Loop to check minimum and maximum extents for workspace
  for (auto &ws : m_workspaceList) {
    const auto &specInfo = ws->spectrumInfo();
    for (size_t i = 0; i < specInfo.size(); ++i) {
      if (specInfo.isMonitor(i))
        continue;
      const auto &pos = specInfo.position(i);
      double thetaAngle = atan2(pos.X(), pos.Z()) * 180.0 / M_PI;
      m_startScatteringAngle = std::min(m_startScatteringAngle, thetaAngle);
      m_endScatteringAngle = std::max(m_endScatteringAngle, thetaAngle);
    }
  }

  g_log.information() << "Found start scattering angle of "
                      << m_startScatteringAngle
                      << " and end scattering angle of " << m_endScatteringAngle
                      << "\n";

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

  std::vector<double> heightBinning = getProperty("HeightBinning");
  if (heightBinning.size() == 1) {
    m_stepHeight = heightBinning[0];
  } else if (heightBinning.size() == 3) {
    if (heightBinning[0] > m_startHeight || heightBinning[2] < m_endHeight)
      g_log.warning() << "Some detectors outside of height range.\n";
    m_startHeight = heightBinning[0];
    m_stepHeight = heightBinning[1];
    m_endHeight = heightBinning[2];
  }

  g_log.information() << "Scattering angle binning:" << m_startScatteringAngle
                      << ", " << m_stepScatteringAngle << ", "
                      << m_endScatteringAngle << "\n";
  g_log.information() << "Height binning:" << m_startHeight << ", "
                      << m_stepHeight << ", " << m_endHeight << "\n";
}

} // namespace Algorithms
} // namespace Mantid
