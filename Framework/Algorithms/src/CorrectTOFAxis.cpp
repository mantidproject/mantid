// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CorrectTOFAxis.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitConversion.h"

namespace Mantid {
namespace Algorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CorrectTOFAxis)

namespace {
/** A private namespace holding some column names for FindEPP algorithm's output
 *  table.
 */
namespace EPPTableLiterals {
/// Title of the fit status column in EPP tables
const static std::string FIT_STATUS_COLUMN("FitStatus");
/// Title of the peak centre column in EPP tables
const static std::string PEAK_CENTRE_COLUMN("PeakCentre");
/// Tag for successfully fitted rows in EPP tables
const static std::string FIT_STATUS_SUCCESS("success");
} // namespace EPPTableLiterals

/** A private namespace listing the different ways to index
 *  spectra in Mantid.
 */
namespace IndexTypes {
/// Tag for detector ids
const static std::string DETECTOR_ID("Detector ID");
/// Tag for spectrum numbers
const static std::string SPECTRUM_NUMBER("Spectrum Number");
/// Tag for workspace indices
const static std::string WORKSPACE_INDEX("Workspace Index");
} // namespace IndexTypes

/** A private namespace listing the properties of CorrectTOFAxis.
 */
namespace PropertyNames {
const static std::string ELASTIC_BIN_INDEX("ElasticBinIndex");
const static std::string EPP_TABLE("EPPTable");
const static std::string FIXED_ENERGY("EFixed");
const static std::string INDEX_TYPE("IndexType");
const static std::string INPUT_WORKSPACE("InputWorkspace");
const static std::string L2("L2");
const static std::string OUTPUT_WORKSPACE("OutputWorkspace");
const static std::string REFERENCE_SPECTRA("ReferenceSpectra");
const static std::string REFERENCE_WORKSPACE("ReferenceWorkspace");
} // namespace PropertyNames

/** A private namespace listing some sample log entries.
 */
namespace SampleLog {
const static std::string INCIDENT_ENERGY("Ei");
const static std::string WAVELENGTH("wavelength");
} // namespace SampleLog

/** Maps given index according to indexMap.
 *
 *  @param index The index to be mapped
 *  @param indexMap The index map
 *
 *  @return The mapped index.
 *
 *  @throw std::runtime_error if index is not in the map
 */
template <typename Map> size_t mapIndex(const int index, const Map &indexMap) {
  try {
    return indexMap.at(index);
  } catch (std::out_of_range &) {
    throw std::runtime_error(PropertyNames::REFERENCE_SPECTRA +
                             " out of range.");
  }
}

/** Converts given index from indexType to workspace index. If
 *  indexType is already IndexType::WORKSPACE_INDEX, then index
 *  is cast to size_t.
 *
 *  @param index Index to be converted
 *  @param indexType Type of index.
 *  @param ws Workspace to get index mappings from.
 *
 *  @return A workspace index corresponding to index.
 */
size_t toWorkspaceIndex(const int index, const std::string &indexType,
                        API::MatrixWorkspace_const_sptr ws) {
  if (indexType == IndexTypes::DETECTOR_ID) {
    const auto indexMap = ws->getDetectorIDToWorkspaceIndexMap();
    return mapIndex(index, indexMap);
  } else if (indexType == IndexTypes::SPECTRUM_NUMBER) {
    const auto indexMap = ws->getSpectrumToWorkspaceIndexMap();
    return mapIndex(index, indexMap);
  } else {
    if (index < 0) {
      throw std::runtime_error(PropertyNames::REFERENCE_SPECTRA +
                               " out of range.");
    }
    return static_cast<size_t>(index);
  }
}

/** Transforms indices according to given maps.
 *  @param spectra A vector of indices to be transformed
 *  @param indexMap A map to use in the transformation
 *  @param workspaceIndices An output parameter for the transformed indices
 */
template <typename Map>
void mapIndices(const std::vector<int> &spectra, const Map &indexMap,
                std::vector<size_t> &workspaceIndices) {
  auto back = std::back_inserter(workspaceIndices);
  std::transform(spectra.cbegin(), spectra.cend(), back, [&indexMap](int i) {
    try {
      return indexMap.at(i);
    } catch (std::out_of_range &) {
      throw std::runtime_error(PropertyNames::REFERENCE_SPECTRA +
                               " out of range.");
    }
  });
}
} // anonymous namespace

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CorrectTOFAxis::name() const { return "CorrectTOFAxis"; }

/// Algorithm's version for identification. @see Algorithm::version
int CorrectTOFAxis::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string CorrectTOFAxis::category() const {
  return "Inelastic\\Corrections";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CorrectTOFAxis::summary() const {
  return "Corrects the time-of-flight axis with regards to the incident energy "
         "and the L1+L2 distance or a reference workspace.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void CorrectTOFAxis::init() {
  auto tofWorkspace = boost::make_shared<Kernel::CompositeValidator>();
  tofWorkspace->add<API::WorkspaceUnitValidator>("TOF");
  tofWorkspace->add<API::InstrumentValidator>();
  auto mustBePositiveDouble =
      boost::make_shared<Kernel::BoundedValidator<double>>();
  mustBePositiveDouble->setLower(0);
  auto mustBePositiveInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  mustBePositiveInt->setLower(0);

  declareProperty(
      std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
          PropertyNames::INPUT_WORKSPACE, "", Direction::Input, tofWorkspace),
      "An input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      PropertyNames::OUTPUT_WORKSPACE, "", Direction::Output),
                  "An output workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      PropertyNames::REFERENCE_WORKSPACE, "", Direction::Input,
                      API::PropertyMode::Optional, tofWorkspace),
                  "A reference workspace from which to copy the TOF axis as "
                  "well as the 'Ei' and 'wavelength' sample logs.");
  declareProperty(std::make_unique<WorkspaceProperty<API::ITableWorkspace>>(
                      PropertyNames::EPP_TABLE.c_str(), "", Direction::Input,
                      API::PropertyMode::Optional),
                  "An input EPP table.");
  const std::vector<std::string> indexTypes{IndexTypes::DETECTOR_ID,
                                            IndexTypes::SPECTRUM_NUMBER,
                                            IndexTypes::WORKSPACE_INDEX};
  declareProperty(PropertyNames::INDEX_TYPE, IndexTypes::DETECTOR_ID,
                  boost::make_shared<Kernel::StringListValidator>(indexTypes),
                  "The type of indices used in " +
                      PropertyNames::REFERENCE_SPECTRA + " or " +
                      PropertyNames::ELASTIC_BIN_INDEX + " (default: '" +
                      IndexTypes::DETECTOR_ID + "').");
  declareProperty(std::make_unique<Kernel::ArrayProperty<int>>(
                      PropertyNames::REFERENCE_SPECTRA.c_str()),
                  "A list of reference spectra.");
  declareProperty(
      PropertyNames::ELASTIC_BIN_INDEX, EMPTY_INT(), mustBePositiveInt,
      "Bin index of the nominal elastic TOF channel.", Direction::Input);
  declareProperty(
      PropertyNames::FIXED_ENERGY, EMPTY_DBL(), mustBePositiveDouble,
      "Incident energy if the 'EI' sample log is not present/incorrect.",
      Direction::Input);
  declareProperty(PropertyNames::L2, EMPTY_DBL(), mustBePositiveDouble,
                  "Sample to detector distance, in meters.", Direction::Input);
}

/** Validate the algorithm's input properties.
 *  Also does some setup for the exec() method.
 */
std::map<std::string, std::string> CorrectTOFAxis::validateInputs() {
  std::map<std::string, std::string> issues;
  m_inputWs = getProperty(PropertyNames::INPUT_WORKSPACE);
  m_referenceWs = getProperty(PropertyNames::REFERENCE_WORKSPACE);
  if (m_referenceWs) {
    m_referenceWs = getProperty(PropertyNames::REFERENCE_WORKSPACE);
    if (m_inputWs->getNumberHistograms() !=
        m_referenceWs->getNumberHistograms()) {
      issues[PropertyNames::REFERENCE_WORKSPACE] =
          "Number of histograms don't match with" +
          PropertyNames::INPUT_WORKSPACE + ".";
    }
    for (size_t i = 0; i < m_inputWs->getNumberHistograms(); ++i) {
      if (m_inputWs->x(i).size() != m_referenceWs->x(i).size()) {
        issues[PropertyNames::REFERENCE_WORKSPACE] =
            "X axis sizes don't match with " + PropertyNames::INPUT_WORKSPACE +
            ".";
        break;
      }
    }
    if (!m_referenceWs->run().hasProperty(SampleLog::INCIDENT_ENERGY)) {
      issues[PropertyNames::REFERENCE_WORKSPACE] =
          "'Ei' is missing from the sample logs.";
    }
    if (!m_referenceWs->run().hasProperty(SampleLog::WAVELENGTH)) {
      issues[PropertyNames::REFERENCE_WORKSPACE] =
          "'wavelength' is missing from the sample logs.";
    }
    // If reference workspace is given, the rest of the properties are
    // skipped.
    return issues;
  }
  // If no reference workspace, we either use a predefined elastic channel
  // or EPP tables to declare the elastic TOF.
  const int elasticBinIndex = getProperty(PropertyNames::ELASTIC_BIN_INDEX);
  const std::vector<int> spectra =
      getProperty(PropertyNames::REFERENCE_SPECTRA);
  const double l2 = getProperty(PropertyNames::L2);
  if (elasticBinIndex != EMPTY_INT()) {
    const std::string indexType = getProperty(PropertyNames::INDEX_TYPE);
    m_elasticBinIndex = toWorkspaceIndex(elasticBinIndex, indexType, m_inputWs);
    if (spectra.empty() && l2 == EMPTY_DBL()) {
      issues[PropertyNames::REFERENCE_SPECTRA] =
          "Either " + PropertyNames::REFERENCE_SPECTRA + " or " +
          PropertyNames::L2 + " has to be specified.";
      return issues;
    }
  } else {
    m_eppTable = getProperty(PropertyNames::EPP_TABLE);
    if (!m_eppTable) {
      issues[PropertyNames::EPP_TABLE] = "No EPP table specified nor " +
                                         PropertyNames::ELASTIC_BIN_INDEX +
                                         " specified.";
      return issues;
    }
    const auto peakPositionColumn =
        m_eppTable->getColumn(EPPTableLiterals::PEAK_CENTRE_COLUMN);
    const auto fitStatusColumn =
        m_eppTable->getColumn(EPPTableLiterals::FIT_STATUS_COLUMN);
    if (!peakPositionColumn || !fitStatusColumn) {
      issues[PropertyNames::EPP_TABLE] =
          "EPP table doesn't contain the expected columns.";
      return issues;
    }
    if (spectra.empty()) {
      issues[PropertyNames::REFERENCE_SPECTRA] =
          "No reference spectra selected.";
      return issues;
    }
  }
  m_workspaceIndices = referenceWorkspaceIndices();
  std::sort(m_workspaceIndices.begin(), m_workspaceIndices.end());
  m_workspaceIndices.erase(
      std::unique(m_workspaceIndices.begin(), m_workspaceIndices.end()),
      m_workspaceIndices.end());
  const auto &spectrumInfo = m_inputWs->spectrumInfo();
  for (const auto i : m_workspaceIndices) {
    if (spectrumInfo.isMonitor(i)) {
      issues[PropertyNames::REFERENCE_SPECTRA] =
          "Monitor found among the given spectra.";
      break;
    }
    if (!spectrumInfo.hasDetectors(i)) {
      issues[PropertyNames::REFERENCE_SPECTRA] =
          "No detectors attached to workspace index " + std::to_string(i) + ".";
      break;
    }
    if (m_eppTable) {
      const auto peakPositionColumn =
          m_eppTable->getColumn(EPPTableLiterals::PEAK_CENTRE_COLUMN);
      if (i >= peakPositionColumn->size()) {
        issues[PropertyNames::REFERENCE_SPECTRA] =
            "Workspace index " + std::to_string(i) +
            " not found in the EPP table.";
      }
    }
  }

  if (getPointerToProperty(PropertyNames::FIXED_ENERGY)->isDefault()) {
    if (!m_inputWs->run().hasProperty(SampleLog::INCIDENT_ENERGY)) {
      issues[PropertyNames::INPUT_WORKSPACE] =
          "'Ei' is missing from the sample logs.";
    }
  }
  return issues;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void CorrectTOFAxis::exec() {
  m_inputWs = getProperty(PropertyNames::INPUT_WORKSPACE);
  API::MatrixWorkspace_sptr outputWs =
      getProperty(PropertyNames::OUTPUT_WORKSPACE);
  if (outputWs != m_inputWs) {
    outputWs = m_inputWs->clone();
  }
  if (m_referenceWs) {
    useReferenceWorkspace(outputWs);
  } else {
    correctManually(outputWs);
  }
  setProperty(PropertyNames::OUTPUT_WORKSPACE, outputWs);
}

/** Correct with regards to a reference workspace.
 *  Copies the X axis as well as the 'Ei' and 'wavelength' sample logs to the
 *  corrected workspace.
 *  @param outputWs The corrected workspace
 */
void CorrectTOFAxis::useReferenceWorkspace(API::MatrixWorkspace_sptr outputWs) {
  const int64_t histogramCount =
      static_cast<int64_t>(m_referenceWs->getNumberHistograms());
  PARALLEL_FOR_IF(threadSafe(*m_referenceWs, *outputWs))
  for (int64_t i = 0; i < histogramCount; ++i) {
    PARALLEL_START_INTERUPT_REGION
    std::copy(m_referenceWs->x(i).cbegin(), m_referenceWs->x(i).cend(),
              outputWs->mutableX(i).begin());
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  if (outputWs->run().hasProperty(SampleLog::INCIDENT_ENERGY)) {
    outputWs->mutableRun()
        .getProperty(SampleLog::INCIDENT_ENERGY)
        ->setValueFromProperty(
            *m_referenceWs->run().getProperty(SampleLog::INCIDENT_ENERGY));
  }
  if (outputWs->run().hasProperty(SampleLog::WAVELENGTH)) {
    outputWs->mutableRun()
        .getProperty(SampleLog::WAVELENGTH)
        ->setValueFromProperty(
            *m_referenceWs->run().getProperty(SampleLog::WAVELENGTH));
  }
}

/** Do manual TOF axis correction.
 *  Resolves the L1 and average L2 distances and calculates the time-of-flight
 *  corresponding to the given incident energy. The X axis of the input
 *  workspace is shifted correspondingly. If the incident energy is given
 *  specifically, also adjusts the 'Ei' and 'wavelength' sample logs.
 *  @param outputWs The corrected workspace
 */
void CorrectTOFAxis::correctManually(API::MatrixWorkspace_sptr outputWs) {
  const auto &spectrumInfo = m_inputWs->spectrumInfo();
  const double l1 = spectrumInfo.l1();
  double l2 = 0;
  double epp = 0;
  g_log.information() << "EPP: " << epp << ".\n";
  if (m_eppTable) {
    averageL2AndEPP(spectrumInfo, l2, epp);
  } else {
    epp = m_inputWs->points(0)[m_elasticBinIndex];
    const double l2Property = getProperty(PropertyNames::L2);
    l2 = l2Property == EMPTY_DBL() ? averageL2(spectrumInfo) : l2Property;
  }
  double Ei = getProperty(PropertyNames::FIXED_ENERGY);
  if (Ei == EMPTY_DBL()) {
    Ei = m_inputWs->run().getPropertyAsSingleValue(SampleLog::INCIDENT_ENERGY);
  } else {
    // Save user-given Ei and wavelength to the output workspace.
    outputWs->mutableRun().addProperty(SampleLog::INCIDENT_ENERGY, Ei, true);
    const double wavelength = Kernel::UnitConversion::run(
        "Energy", "Wavelength", Ei, l1, l2, 0, Kernel::DeltaEMode::Direct, 0);
    outputWs->mutableRun().addProperty(SampleLog::WAVELENGTH, wavelength, true);
  }
  // In microseconds.
  const double TOF = (l1 + l2) /
                     std::sqrt(2 * Ei * PhysicalConstants::meV /
                               PhysicalConstants::NeutronMass) *
                     1e6;
  g_log.information() << "Calculated TOF for L1+L2 distance of " << l1 + l2
                      << "m: " << TOF << '\n';
  const double shift = TOF - epp;
  g_log.debug() << "TOF shift: " << shift << '\n';
  const int64_t histogramCount =
      static_cast<int64_t>(m_inputWs->getNumberHistograms());
  PARALLEL_FOR_IF(threadSafe(*m_inputWs, *outputWs))
  for (int64_t i = 0; i < histogramCount; ++i) {
    PARALLEL_START_INTERUPT_REGION
    outputWs->mutableX(i) += shift;
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

/** Calculates the average L2 distance between the sample and given
 *  detectors.
 *  @param spectrumInfo A spectrum info for the input workspace
 *  @param l2 An output parameter for the average L2 distance
 *  @param epp An output parameter for the average position
 *         of the detectors' elastic peak
 */
void CorrectTOFAxis::averageL2AndEPP(const API::SpectrumInfo &spectrumInfo,
                                     double &l2, double &epp) {
  auto peakPositionColumn =
      m_eppTable->getColumn(EPPTableLiterals::PEAK_CENTRE_COLUMN);
  auto fitStatusColumn =
      m_eppTable->getColumn(EPPTableLiterals::FIT_STATUS_COLUMN);
  double l2Sum = 0;
  double eppSum = 0;
  size_t n = 0;
  const int64_t indexCount = static_cast<int64_t>(m_workspaceIndices.size());
  // cppcheck-suppress syntaxError
  PRAGMA_OMP(parallel for if (m_eppTable->threadSafe())
             reduction(+: n, l2Sum, eppSum))
  for (int64_t i = 0; i < indexCount; ++i) {
    PARALLEL_START_INTERUPT_REGION
    const size_t index = m_workspaceIndices[i];
    interruption_point();
    if (fitStatusColumn->cell<std::string>(index) ==
        EPPTableLiterals::FIT_STATUS_SUCCESS) {
      if (!spectrumInfo.isMasked(index)) {
        const double d = spectrumInfo.l2(index);
        l2Sum += d;
        const double epp = (*peakPositionColumn)[index];
        eppSum += epp;
        ++n;
        g_log.debug() << "Including workspace index " << index
                      << " - distance: " << d << " EPP: " << epp << ".\n";
      } else {
        g_log.debug() << "Excluding masked workspace index " << index << ".\n";
      }
    } else {
      g_log.debug()
          << "Excluding detector with unsuccessful fit at workspace index "
          << index << ".\n";
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  if (n == 0) {
    throw std::runtime_error("No successful detector fits found in " +
                             PropertyNames::EPP_TABLE);
  }
  l2 = l2Sum / static_cast<double>(n);
  g_log.information() << "Average L2 distance: " << l2 << ".\n";
  epp = eppSum / static_cast<double>(n);
  g_log.information() << "Average EPP: " << epp << ".\n";
}

double CorrectTOFAxis::averageL2(const API::SpectrumInfo &spectrumInfo) {
  double l2Sum = 0;
  size_t n = 0;
  const int64_t indexCount = static_cast<int64_t>(m_workspaceIndices.size());
  PRAGMA_OMP(parallel for reduction(+: n, l2Sum))
  for (int64_t i = 0; i < indexCount; ++i) {
    PARALLEL_START_INTERUPT_REGION
    const size_t index = m_workspaceIndices[i];
    interruption_point();
    if (!spectrumInfo.isMasked(index)) {
      const double d = spectrumInfo.l2(index);
      ++n;
      l2Sum += d;
    } else {
      g_log.debug() << "Excluding masked workspace index " << index << ".\n";
    }
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  if (n == 0) {
    throw std::runtime_error("No unmasked detectors found in " +
                             PropertyNames::REFERENCE_SPECTRA);
  }
  const double l2 = l2Sum / static_cast<double>(indexCount);
  g_log.information() << "Average L2 distance: " << l2 << ".\n";
  return l2;
}

/** Transform spectrum numbers or detector IDs to workspace indices.
 *  @return The transformed workspace indices.
 */
std::vector<size_t> CorrectTOFAxis::referenceWorkspaceIndices() const {
  const std::vector<int> indices =
      getProperty(PropertyNames::REFERENCE_SPECTRA);
  const std::string indexType = getProperty(PropertyNames::INDEX_TYPE);
  std::vector<size_t> workspaceIndices(indices.size());
  std::transform(indices.cbegin(), indices.cend(), workspaceIndices.begin(),
                 [&indexType, this](int index) {
                   return toWorkspaceIndex(index, indexType, m_inputWs);
                 });
  return workspaceIndices;
}

} // namespace Algorithms
} // namespace Mantid
