// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/Bin2DPowderDiffraction.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/SpectraAxisValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace Mantid::Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;
using namespace Mantid::HistogramData;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Bin2DPowderDiffraction)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string Bin2DPowderDiffraction::name() const { return "Bin2DPowderDiffraction"; }

/// Algorithm's version for identification. @see Algorithm::version
int Bin2DPowderDiffraction::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string Bin2DPowderDiffraction::category() const { return "Diffraction\\Focussing"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string Bin2DPowderDiffraction::summary() const {
  return "Bins TOF powder diffraction event data in 2D space.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void Bin2DPowderDiffraction::init() {
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<SpectraAxisValidator>();
  wsValidator->add<InstrumentValidator>();
  wsValidator->add<HistogramValidator>();

  declareProperty(
      std::make_unique<WorkspaceProperty<EventWorkspace>>("InputWorkspace", "", Direction::Input, wsValidator),
      "An input EventWorkspace must be a Histogram workspace, not Point data. "
      "X-axis units must be wavelength.");

  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");

  const std::string docString = "A comma separated list of first bin boundary, width, last bin boundary. "
                                "Optionally "
                                "this can be followed by a comma and more widths and last boundary "
                                "pairs. "
                                "Negative width values indicate logarithmic binning.";
  auto rebinValidator = std::make_shared<RebinParamsValidator>(true);
  declareProperty(std::make_unique<ArrayProperty<double>>("dSpaceBinning", rebinValidator), docString);
  declareProperty(std::make_unique<ArrayProperty<double>>("dPerpendicularBinning", rebinValidator), docString);

  const std::vector<std::string> exts{".txt", ".dat"};
  declareProperty(std::make_unique<FileProperty>("BinEdgesFile", "", FileProperty::OptionalLoad, exts),
                  "Optional: The ascii file containing the list of bin edges. "
                  "Either this or Axis1- and dPerpendicularBinning need to be specified.");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("NormalizeByBinArea", true),
                  "Normalize the binned workspace by the bin area.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void Bin2DPowderDiffraction::exec() {
  m_inputWS = this->getProperty("InputWorkspace");
  m_numberOfSpectra = static_cast<int>(m_inputWS->getNumberHistograms());
  g_log.debug() << "Number of spectra in input workspace: " << m_numberOfSpectra << "\n";

  MatrixWorkspace_sptr outputWS = createOutputWorkspace();

  const bool normalizeByBinArea = this->getProperty("NormalizeByBinArea");
  if (normalizeByBinArea) {
    const auto startTime = std::chrono::high_resolution_clock::now();
    normalizeToBinArea(outputWS);
    addTimer("normalizeByBinArea", startTime, std::chrono::high_resolution_clock::now());
  }

  setProperty("OutputWorkspace", outputWS);
}
//----------------------------------------------------------------------------------------------
/**
 * @brief Bin2DPowderDiffraction::validateInputs Validate inputs
 * @return
 */
std::map<std::string, std::string> Bin2DPowderDiffraction::validateInputs() {
  std::map<std::string, std::string> result;

  const auto useBinFile = !getPointerToProperty("BinEdgesFile")->isDefault();
  const auto useBinning1 = !getPointerToProperty("dSpaceBinning")->isDefault();
  const auto useBinning2 = !getPointerToProperty("dPerpendicularBinning")->isDefault();
  if (!useBinFile && !useBinning1 && !useBinning2) {
    const std::string msg = "You must specify either dSpaceBinning and "
                            "dPerpendicularBinning, or a BinEdgesFile.";
    result["dSpaceBinning"] = msg;
    result["dPerpendicularBinning"] = msg;
    result["BinEdgesFile"] = msg;
  } else if (useBinFile && (useBinning1 || useBinning2)) {
    const std::string msg = "You must specify either dSpaceBinning and "
                            "dPerpendicularBinning, or a BinEdgesFile, but not both.";
    result["BinEdgesFile"] = msg;
  }

  return result;
}
//----------------------------------------------------------------------------------------------
/**
 * @brief createOutputWorkspace create an output workspace and setup axis
 * @throw std::runtime_error If theta=0 or cos(theta)<=0
 * @return MatrixWorkspace with binned events
 */

MatrixWorkspace_sptr Bin2DPowderDiffraction::createOutputWorkspace() {

  using VectorHelper::createAxisFromRebinParams;
  bool binsFromFile(false);
  size_t dPerpSize = 0;
  size_t dSize = 0;
  MatrixWorkspace_sptr outputWS;
  const auto &spectrumInfo = m_inputWS->spectrumInfo();

  const std::string beFileName = getProperty("BinEdgesFile");
  if (!beFileName.empty())
    binsFromFile = true;

  const auto &oldXEdges = m_inputWS->x(0);
  BinEdges dBins(oldXEdges.size());
  BinEdges dPerpBins(oldXEdges.size());

  auto &dPerp = dPerpBins.mutableRawData();
  std::vector<std::vector<double>> fileXbins;

  // First create the output Workspace filled with zeros
  auto startTime = std::chrono::high_resolution_clock::now();
  if (binsFromFile) {
    dPerp.clear();
    ReadBinsFromFile(dPerp, fileXbins);
    dPerpSize = dPerp.size();
    // unify xbins
    dSize = UnifyXBins(fileXbins);
    g_log.debug() << "Maximal size of Xbins = " << dSize;
    outputWS = WorkspaceFactory::Instance().create(m_inputWS, dPerpSize - 1, dSize, dSize - 1);
    g_log.debug() << "Outws has " << outputWS->getNumberHistograms() << " histograms and " << outputWS->blocksize()
                  << " bins." << std::endl;

    size_t idx = 0;
    for (const auto &Xbins : fileXbins) {
      g_log.debug() << "Xbins size: " << Xbins.size() << std::endl;
      BinEdges binEdges(Xbins);
      outputWS->setBinEdges(idx, binEdges);
      idx++;
    }

  } else {
    static_cast<void>(createAxisFromRebinParams(getProperty("dSpaceBinning"), dBins.mutableRawData()));
    HistogramData::BinEdges binEdges(dBins);
    dPerpSize = createAxisFromRebinParams(getProperty("dPerpendicularBinning"), dPerp);
    dSize = binEdges.size();
    outputWS = WorkspaceFactory::Instance().create(m_inputWS, dPerpSize - 1, dSize, dSize - 1);
    for (size_t idx = 0; idx < dPerpSize - 1; idx++)
      outputWS->setBinEdges(idx, binEdges);
    auto abscissa = std::make_unique<BinEdgeAxis>(dBins.mutableRawData());
    outputWS->replaceAxis(0, std::move(abscissa));
  }
  addTimer("createWorkspace", startTime, std::chrono::high_resolution_clock::now());

  startTime = std::chrono::high_resolution_clock::now();
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("dSpacing");

  auto verticalAxis = std::make_unique<BinEdgeAxis>(dPerp);
  auto verticalAxisRaw = verticalAxis.get();
  // Meta data
  verticalAxis->unit() = UnitFactory::Instance().create("dSpacingPerpendicular");
  verticalAxis->title() = "d_p";
  outputWS->replaceAxis(1, std::move(verticalAxis));

  Progress prog(this, 0.0, 1.0, m_numberOfSpectra);
  auto numSpectra = static_cast<int64_t>(m_numberOfSpectra);
  std::vector<std::vector<double>> newYValues(dPerpSize - 1, std::vector<double>(dSize - 1, 0.0));
  std::vector<std::vector<double>> newEValues(dPerpSize - 1, std::vector<double>(dSize - 1, 0.0));

  // fill the workspace with data
  g_log.debug() << "newYSize = " << dPerpSize << std::endl;
  g_log.debug() << "newXSize = " << dSize << std::endl;
  std::vector<double> dp_vec(verticalAxisRaw->getValues());
  addTimer("fillValues", startTime, std::chrono::high_resolution_clock::now());

  startTime = std::chrono::high_resolution_clock::now();
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *outputWS))
  for (int64_t snum = 0; snum < numSpectra; ++snum) {
    PARALLEL_START_INTERRUPT_REGION
    if (!spectrumInfo.isMasked(snum)) {
      const double theta = 0.5 * spectrumInfo.twoTheta(snum);
      const double sin_theta = sin(theta);
      if (sin_theta == 0) {
        throw std::runtime_error("Spectrum " + std::to_string(snum) + " has sin(theta)=0. Cannot calculate d-Spacing!");
      }
      if (cos(theta) <= 0) {
        throw std::runtime_error("Spectrum " + std::to_string(snum) +
                                 " has cos(theta) <= 0. Cannot calculate d-SpacingPerpendicular!");
      }
      const double log_cos_theta = log(cos(theta));
      EventList &evList = m_inputWS->getSpectrum(snum);

      // Switch to weighted if needed.
      if (evList.getEventType() == TOF)
        evList.switchTo(WEIGHTED);

      std::vector<WeightedEvent> events = evList.getWeightedEvents();

      for (const auto &ev : events) {
        // find d-perpendicular bin
        const auto dp = calcDPerp(ev.tof(), log_cos_theta);
        const auto lowy = std::lower_bound(dp_vec.begin(), dp_vec.end(), dp);
        if ((lowy == dp_vec.end()) || (lowy == dp_vec.begin()))
          continue;
        const auto dp_index = static_cast<size_t>(std::distance(dp_vec.begin(), lowy) - 1);

        // find d bin
        const auto xs = binsFromFile ? fileXbins[dp_index] : dBins.rawData();
        const auto d = calcD(ev.tof(), sin_theta);
        const auto lowx = std::lower_bound(xs.begin(), xs.end(), d);
        if ((lowx == xs.end()) || lowx == xs.begin())
          continue;
        const auto d_index = static_cast<size_t>(std::distance(xs.begin(), lowx) - 1);

        // writing to the same vectors is not thread-safe
        PARALLEL_CRITICAL(newValues) {
          newYValues[dp_index][d_index] += ev.weight();
          newEValues[dp_index][d_index] += ev.errorSquared();
        }
      }
    }
    prog.report("Binning event data...");
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  addTimer("histogram", startTime, std::chrono::high_resolution_clock::now());

  startTime = std::chrono::high_resolution_clock::now();
  size_t idx = 0;
  for (const auto &yVec : newYValues) {
    outputWS->setCounts(idx, yVec);
    idx++;
  }
  idx = 0;
  for (auto &eVec : newEValues) {
    std::transform(eVec.begin(), eVec.end(), eVec.begin(), static_cast<double (*)(double)>(sqrt));
    outputWS->setCountStandardDeviations(idx, eVec);
    idx++;
  }
  addTimer("setValues", startTime, std::chrono::high_resolution_clock::now());

  return outputWS;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief Bin2DPowderDiffraction::ReadBinsFromFile
 * @param[out] Ybins vector of doubles to save the dSpacingPerpendicular bin
 * edges
 * @param[out] Xbins vector of vectors of doubles to save the dSpacing bin edges
 */
void Bin2DPowderDiffraction::ReadBinsFromFile(std::vector<double> &Ybins,
                                              std::vector<std::vector<double>> &Xbins) const {
  const std::string beFileName = getProperty("BinEdgesFile");
  std::ifstream file(beFileName);
  std::string line;
  std::string::size_type n;
  std::string::size_type sz;
  std::vector<double> tmp;
  int dpno = 0;
  while (getline(file, line)) {
    n = line.find("dp =");
    if (n != std::string::npos) {
      if (!tmp.empty()) {
        Xbins.emplace_back(tmp);
        tmp.clear();
      }
      double dp1 = std::stod(line.substr(4), &sz); // 4 is needed to crop 'dp='
      double dp2 = std::stod(line.substr(sz + 4));
      if (dpno < 1) {
        Ybins.emplace_back(dp1);
        Ybins.emplace_back(dp2);
      } else {
        Ybins.emplace_back(dp2);
      }
      dpno++;
    } else if (line.find("#") == std::string::npos) {
      std::stringstream ss(line);
      double d;
      while (ss >> d) {
        tmp.emplace_back(d);
      }
    }
  }
  Xbins.emplace_back(tmp);
  g_log.information() << "Number of Ybins: " << Ybins.size() << std::endl;
  g_log.information() << "Number of Xbins sets: " << Xbins.size() << std::endl;
}
//----------------------------------------------------------------------------------------------
/**
 * @brief Bin2DPowderDiffraction::UnifyXBins unifies size of the vectors in
 *Xbins.
 * Fills the last vector element at the end of the shorter bins.
 * Required to avoid garbage values in the X values after ws->setHistogram.
 * returns the maximal size
 *
 * @param[in] Xbins --- bins to unify. Will be overwritten.
 */
size_t Bin2DPowderDiffraction::UnifyXBins(std::vector<std::vector<double>> &Xbins) const {
  size_t maxSize = std::accumulate(Xbins.cbegin(), Xbins.cend(), size_t(0), [](size_t currentMax, const auto &xbin) {
    return std::max(currentMax, xbin.size());
  });
  // resize all vectors to maximum size, fill last vector element at the end
  for (auto &v : Xbins) {
    if (v.size() < maxSize)
      v.resize(maxSize, v.back());
  }
  return maxSize;
}

void Bin2DPowderDiffraction::normalizeToBinArea(const MatrixWorkspace_sptr &outWS) {
  NumericAxis *verticalAxis = dynamic_cast<NumericAxis *>(outWS->getAxis(1));
  const std::vector<double> &yValues = verticalAxis->getValues();
  auto nhist = outWS->getNumberHistograms();
  g_log.debug() << "Number of hists: " << nhist << " Length of YAxis: " << verticalAxis->length() << std::endl;

  for (size_t idx = 0; idx < nhist; ++idx) {
    double factor = 1.0 / (yValues[idx + 1] - yValues[idx]);
    // divide by the xBinWidth
    outWS->convertToFrequencies(idx);
    auto &freqs = outWS->mutableY(idx);
    using std::placeholders::_1;
    std::transform(freqs.begin(), freqs.end(), freqs.begin(), std::bind(std::multiplies<double>(), factor, _1));
    auto &errors = outWS->mutableE(idx);
    std::transform(errors.begin(), errors.end(), errors.begin(), std::bind(std::multiplies<double>(), factor, _1));
  }
}

double calcD(double wavelength, double sintheta) { return wavelength * 0.5 / sintheta; }

double calcDPerp(double wavelength, double logcostheta) { return sqrt(wavelength * wavelength - 2.0 * logcostheta); }

} // namespace Mantid::Algorithms
