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
          "ThetaBinning", boost::make_shared<RebinParamsValidator>()),
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
      "is the y valuee step size. In this case, the boundary of binning will "
      "be determined by minimum and maximum y values present in the "
      "workspaces.");
}

void BinDetectorScan::exec() {
  std::vector<double> thetaBinning = getProperty("ThetaBinning");
  if (thetaBinning.size() == 0) {
    m_stepAngle = thetaBinning[0];
  } else if (thetaBinning.size() == 3) {
    m_startAngle = thetaBinning[0];
    m_stepAngle = thetaBinning[1];
    m_endAngle = thetaBinning[2];
  }

  std::vector<double> heightBinning = getProperty("HeightBinning");
  if (heightBinning.size() == 0) {
    m_stepHeight = heightBinning[0];
  } else if (heightBinning.size() == 3) {
    m_startHeight = heightBinning[0];
    m_stepHeight = heightBinning[1];
    m_endHeight = heightBinning[2];
  }

  const std::vector<std::string> inputWorkspaces =
      getProperty("InputWorkspaces");
  auto workspaces = RunCombinationHelper::unWrapGroups(inputWorkspaces);
  RunCombinationHelper combHelper;
  auto workspaceList = combHelper.validateInputWorkspaces(workspaces, g_log);

  // TODO: loop to check minimum and maximum extents for workspace

  size_t numBins = int(ceil((m_endAngle - m_startAngle) / m_stepAngle));
  g_log.information() << "Number of bins in output workspace:" << numBins
                      << std::endl;
  HistogramData::BinEdges x(numBins + 1,
                            LinearGenerator(m_startAngle, m_stepAngle));
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
  for (auto &ws : workspaceList) {
    const auto &specInfo = ws->spectrumInfo();
    for (size_t i = 0; i < specInfo.size(); ++i) {
      if (specInfo.isMonitor(i))
        continue;
      const auto &pos = specInfo.position(i);
      const auto y = pos.Y();
      auto theta = atan2(pos.X(), pos.Z()) * 180.0 / M_PI;
      if (theta < 0)
        theta += 360.0;
      size_t yIndex = size_t((y - m_startHeight) / m_stepHeight);
      size_t thetaIndex = size_t((theta - m_startAngle) / m_stepAngle);
      if (yIndex > numHistograms || thetaIndex > numBins) {
        // TODO: This is a bit spammy, replace this with check earlier
        g_log.warning()
            << "Found a detector outside the range, skipping. Spectrum number:"
            << i;
        continue;
      }
      auto counts = ws->histogram(i).y()[0];
      auto &yData = outputWS->mutableY(yIndex);
      yData[thetaIndex] += counts;
    }
  }
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
