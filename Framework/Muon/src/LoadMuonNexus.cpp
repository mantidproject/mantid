// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidMuon/LoadMuonNexus.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidNexus/MuonNexusReader.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidNexusCpp/NeXusException.hpp"
#include "MantidNexusCpp/NeXusFile.hpp"

// must be after MantidNexusCpp/NeXusFile.hpp
#include "MantidLegacyNexus/NeXusFile.hpp"

#include <Poco/Path.h>
#include <cmath>
#include <limits>
#include <memory>

namespace Mantid::Algorithms {

using namespace Kernel;
using namespace API;
using Geometry::Instrument;
using namespace Mantid::NeXus;

/// Empty default constructor
LoadMuonNexus::LoadMuonNexus()
    : m_filename(), m_entrynumber(0), m_numberOfSpectra(0), m_numberOfPeriods(0), m_list(false), m_interval(false),
      m_spec_list(), m_spec_min(0), m_spec_max(EMPTY_INT()) {}

/// Initialisation method.
void LoadMuonNexus::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
                  "The name of the Nexus file to load");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of the\n"
                  "algorithm. For multiperiod files, one workspace will be\n"
                  "generated for each period");

  auto mustBePositive = std::make_shared<BoundedValidator<specnum_t>>();
  mustBePositive->setLower(0);
  declareProperty("SpectrumMin", static_cast<specnum_t>(0), mustBePositive,
                  "Index number of the first spectrum to read\n");
  declareProperty("SpectrumMax", static_cast<specnum_t>(EMPTY_INT()), mustBePositive,
                  "Index of last spectrum to read\n"
                  "(default the last spectrum)");

  declareProperty(std::make_unique<ArrayProperty<specnum_t>>("SpectrumList"),
                  "Array, or comma separated list, of indexes of spectra to\n"
                  "load. If a range and a list of spectra are both supplied,\n"
                  "all the specified spectra will be loaded.");
  declareProperty("AutoGroup", false,
                  "Determines whether the spectra are automatically grouped\n"
                  "together based on the groupings in the NeXus file, only\n"
                  "for single period data (default no). Version 1 only.");

  auto mustBeNonNegative = std::make_shared<BoundedValidator<int64_t>>();
  mustBeNonNegative->setLower(0);
  declareProperty("EntryNumber", static_cast<int64_t>(0), mustBeNonNegative,
                  "0 indicates that every entry is loaded, into a separate "
                  "workspace within a group. "
                  "A positive number identifies one entry to be loaded, into "
                  "one workspace");

  std::vector<std::string> FieldOptions{"Transverse", "Longitudinal"};
  declareProperty("MainFieldDirection", "Transverse", std::make_shared<StringListValidator>(FieldOptions),
                  "Output the main field direction if specified in Nexus file "
                  "(run/instrument/detector/orientation, default "
                  "longitudinal). Version 1 only.",
                  Direction::Output);

  declareProperty("TimeZero", 0.0, "Time zero in units of micro-seconds (default to 0.0)", Direction::Output);
  declareProperty("FirstGoodData", 0.0, "First good data in units of micro-seconds (default to 0.0)",
                  Direction::Output);
  declareProperty("LastGoodData", 0.0, "Last good data in the OutputWorkspace's spectra", Kernel::Direction::Output);

  declareProperty(std::make_unique<ArrayProperty<double>>("TimeZeroList", Direction::Output),
                  "A vector of time zero values");

  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>("TimeZeroTable", "", Direction::Output, PropertyMode::Optional),
      "TableWorkspace containing time zero values per spectra.");

  declareProperty("CorrectTime", true, "Boolean flag controlling whether time should be corrected by timezero.",
                  Direction::Input);

  declareProperty(
      std::make_unique<WorkspaceProperty<Workspace>>("DeadTimeTable", "", Direction::Output, PropertyMode::Optional),
      "Table or a group of tables containing detector dead times. Version 1 "
      "only.");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("DetectorGroupingTable", "", Direction::Output,
                                                                 PropertyMode::Optional),
                  "Table or a group of tables with information about the "
                  "detector grouping stored in the file (if any). Version 1 only.");
}

/// Validates the optional 'spectra to read' properties, if they have been set
void LoadMuonNexus::checkOptionalProperties() {
  // read in the settings passed to the algorithm
  m_spec_list = getProperty("SpectrumList");
  m_spec_max = getProperty("SpectrumMax");
  // Are we using a list of spectra or all the spectra in a range?
  m_list = !m_spec_list.empty();
  m_interval = (m_spec_max != EMPTY_INT());
  if (m_spec_max == EMPTY_INT())
    m_spec_max = 0;

  // Check validity of spectra list property, if set
  if (m_list) {
    const specnum_t minlist = *min_element(m_spec_list.begin(), m_spec_list.end());
    const specnum_t maxlist = *max_element(m_spec_list.begin(), m_spec_list.end());
    if (maxlist > m_numberOfSpectra || minlist == 0) {
      g_log.error("Invalid list of spectra");
      throw std::invalid_argument("Inconsistent properties defined");
    }
  }

  // Check validity of spectra range, if set
  if (m_interval) {
    m_spec_min = getProperty("SpectrumMin");
    if (m_spec_max < m_spec_min || m_spec_max > m_numberOfSpectra) {
      g_log.error("Invalid Spectrum min/max properties");
      throw std::invalid_argument("Inconsistent properties defined");
    }
  }
}

/// Run the Child Algorithm LoadInstrument
void LoadMuonNexus::runLoadInstrument(const DataObjects::Workspace2D_sptr &localWorkspace) {

  auto loadInst = createChildAlgorithm("LoadInstrument");

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadInst->setPropertyValue("InstrumentName", m_instrument_name);
    loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", localWorkspace);
    loadInst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(false));
    loadInst->execute();
  } catch (std::invalid_argument &) {
    g_log.information("Invalid argument to LoadInstrument Child Algorithm");
  } catch (std::runtime_error &) {
    g_log.information("Unable to successfully run LoadInstrument Child Algorithm");
  }

  // If loading instrument definition file fails,
  // we may get instrument by some other means yet to be decided upon
  // at present just create a dummy instrument with the correct name.
  if (!loadInst->isExecuted()) {
    auto inst = std::make_shared<Geometry::Instrument>();
    inst->setName(m_instrument_name);
    localWorkspace->setInstrument(inst);
  }
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadMuonNexus::confidence(Kernel::NexusDescriptor &descriptor) const {
  UNUSED_ARG(descriptor);
  return 0; // Not to be used but LoadMuonNexus2, which inherits from this will
}

/**
 * Create Algorithm to add a sample log to a workspace
 */
Mantid::API::Algorithm_sptr LoadMuonNexus::createSampleLogAlgorithm(DataObjects::Workspace2D_sptr &ws) {
  Mantid::API::Algorithm_sptr logAlg = createChildAlgorithm("AddSampleLog");
  logAlg->setProperty("Workspace", ws);
  return logAlg;
}

/**
 * Function to add a single int as a sample log to a workspace
 */
void LoadMuonNexus::addToSampleLog(const std::string &logName, const int logNumber, DataObjects::Workspace2D_sptr &ws) {
  auto alg = createSampleLogAlgorithm(ws);
  alg->setProperty("LogType", "Number");
  alg->setProperty("NumberType", "Int");
  alg->setProperty("LogName", logName);
  alg->setProperty("LogText", std::to_string(logNumber));
  alg->executeAsChildAlg();
}

/**
 * Fucntion to add a single string as a sample log to a workspace
 */
void LoadMuonNexus::addToSampleLog(const std::string &logName, const std::string &logString,
                                   DataObjects::Workspace2D_sptr &ws) {
  auto alg = createSampleLogAlgorithm(ws);
  alg->setProperty("LogType", "String");
  alg->setProperty("LogName", logName);
  alg->setProperty("LogText", logString);
  alg->executeAsChildAlg();
}
} // namespace Mantid::Algorithms
