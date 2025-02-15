// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// LoadNexus
// @author Freddie Akeroyd, STFC ISIS Faility
// @author Ronald Fowler, e_Science  - updated to be wrapper to either
// LoadMuonNeuxs or LoadNexusProcessed
// Dropped the upper case X from Algorithm name (still in filenames)
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadNexus.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidNexus/NexusClasses.h"
#include "MantidNexusCpp/NeXusException.hpp"
#include "MantidNexusCpp/NeXusFile.hpp"

#include <cmath>
#include <memory>

namespace Mantid::DataHandling {

const std::string LoadNexus::muonTD = "muonTD";
const std::string LoadNexus::pulsedTD = "pulsedTD";

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadNexus)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

/// Empty default constructor
LoadNexus::LoadNexus() : Algorithm(), m_filename() {}

/** Initialisation method.
 *
 */
void LoadNexus::init() {
  // Declare required input parameters for all Child Algorithms
  const std::vector<std::string> exts{".nxs", ".nx5", ".xml", ".n*"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "The name of the Nexus file to read, as a full or relative path.");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace to be created as the output of "
                  "the algorithm.  A workspace of this name will be created "
                  "and stored in the Analysis Data Service. For multiperiod "
                  "files, one workspace will be generated for each period.");

  // Declare optional input parameters
  auto mustBePositive = std::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  declareProperty("SpectrumMin", 1, mustBePositive, "Number of first spectrum to read, only for single period data.");
  declareProperty("SpectrumMax", Mantid::EMPTY_INT(), mustBePositive,
                  "Number of last spectrum to read, only for single period data.");
  declareProperty(std::make_unique<ArrayProperty<int>>("SpectrumList"),
                  "List of spectrum numbers to read, only for single period data.");

  declareProperty("EntryNumber", 0, mustBePositive,
                  "0 indicates that every entry is loaded, into a separate "
                  "workspace within a group. "
                  "A positive number identifies one entry to be loaded, into "
                  "one workspace");
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void LoadNexus::exec() {
  // Retrieve the filename and output workspace name from the properties
  m_filename = getPropertyValue("Filename");
  m_workspace = getPropertyValue("OutputWorkspace");

  // Test the file of the given file name as described in the
  // documentation of this algorithm.

  std::vector<std::string> entryName, definition;
  int count = getNexusEntryTypes(m_filename, entryName, definition);
  if (count <= -1) {
    g_log.error("Error reading file " + m_filename);
    throw Exception::FileError("Unable to read data in File:", m_filename);
  } else if (count == 0) {
    g_log.error("Error no entries found in " + m_filename);
    throw Exception::FileError("Error no entries found in ", m_filename);
  }
  if (definition[0] == muonTD || definition[0] == pulsedTD) {
    runLoadMuonNexus();
  } else if (entryName[0] == "mantid_workspace_1") {
    runLoadNexusProcessed();
  } else if (entryName[0] == "raw_data_1") {
    runLoadIsisNexus();
  } else {
    Mantid::NeXus::NXRoot root(m_filename);
    Mantid::NeXus::NXEntry entry = root.openEntry(root.groups().front().nxname);
    try {
      Mantid::NeXus::NXChar nxc = entry.openNXChar("instrument/SNSdetector_calibration_id");
    } catch (...) {
      g_log.error("File " + m_filename + " is a currently unsupported type of NeXus file");
      throw Exception::FileError("Unable to read File:", m_filename);
    }
    runLoadTOFRawNexus();
  }
}

void LoadNexus::runLoadMuonNexus() {
  // The Load algorithm will choose the correct LoadMuonNexus algorithm
  auto loadMuonNexus = createChildAlgorithm("Load", 0., 1.);

  // Pass through the same input filename
  loadMuonNexus->setPropertyValue("Filename", m_filename);
  // Set the workspace property
  std::string outputWorkspace = "OutputWorkspace";
  loadMuonNexus->setPropertyValue(outputWorkspace, m_workspace);
  loadMuonNexus->setPropertyValue("DeadTimeTable", m_workspace + "_DeadTimeTable");
  loadMuonNexus->setPropertyValue("DetectorGroupingTable", m_workspace + "DetectorGroupingTable");
  loadMuonNexus->setPropertyValue("TimeZeroTable", m_workspace + "TimeZeroTable");

  // Get the array passed in the spectrum_list, if an empty array was passed use
  // the default
  std::vector<int> specList = getProperty("SpectrumList");
  if (!specList.empty())
    loadMuonNexus->setPropertyValue("SpectrumList", getPropertyValue("SpectrumList"));
  //
  int specMax = getProperty("SpectrumMax");
  if (specMax != Mantid::EMPTY_INT()) {
    loadMuonNexus->setPropertyValue("SpectrumMax", getPropertyValue("SpectrumMax"));
    loadMuonNexus->setPropertyValue("SpectrumMin", getPropertyValue("SpectrumMin"));
  }
  loadMuonNexus->setPropertyValue("EntryNumber", getPropertyValue("EntryNumber"));

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  // try
  // {
  loadMuonNexus->execute();
  // }
  // catch (std::runtime_error&)
  // {
  //   g_log.error("Unable to successfully run LoadMuonNexus Child Algorithm");
  //  }
  if (!loadMuonNexus->isExecuted())
    g_log.error("Unable to successfully run LoadMuonNexus2 Child Algorithm");

  setOutputWorkspace(loadMuonNexus);
}

void LoadNexus::runLoadNexusProcessed() {
  auto loadNexusPro = createChildAlgorithm("LoadNexusProcessed", 0., 1.);
  // Pass through the same input filename
  loadNexusPro->setPropertyValue("Filename", m_filename);
  // Set the workspace property
  loadNexusPro->setPropertyValue("OutputWorkspace", m_workspace);

  loadNexusPro->setPropertyValue("SpectrumMin", getPropertyValue("SpectrumMin"));
  loadNexusPro->setPropertyValue("SpectrumMax", getPropertyValue("SpectrumMax"));
  loadNexusPro->setPropertyValue("SpectrumList", getPropertyValue("SpectrumList"));

  // Get the array passed in the spectrum_list, if an empty array was passed use
  // the default

  loadNexusPro->setPropertyValue("EntryNumber", getPropertyValue("EntryNumber"));
  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  loadNexusPro->execute();
  if (!loadNexusPro->isExecuted())
    g_log.error("Unable to successfully run LoadNexusProcessed Child Algorithm");

  setOutputWorkspace(loadNexusPro);
}

void LoadNexus::runLoadIsisNexus() {
  auto loadNexusPro = createChildAlgorithm("LoadISISNexus", 0., 1.);
  // Pass through the same input filename
  loadNexusPro->setPropertyValue("Filename", m_filename);
  // Set the workspace property
  std::string outputWorkspace = "OutputWorkspace";
  loadNexusPro->setPropertyValue(outputWorkspace, m_workspace);
  // Get the array passed in the spectrum_list, if an empty array was passed use
  // the default
  std::vector<int> specList = getProperty("SpectrumList");
  if (!specList.empty())
    loadNexusPro->setPropertyValue("SpectrumList", getPropertyValue("SpectrumList"));
  //
  int specMax = getProperty("SpectrumMax");
  if (specMax != Mantid::EMPTY_INT()) {
    loadNexusPro->setPropertyValue("SpectrumMax", getPropertyValue("SpectrumMax"));
    loadNexusPro->setPropertyValue("SpectrumMin", getPropertyValue("SpectrumMin"));
  }
  loadNexusPro->setPropertyValue("EntryNumber", getPropertyValue("EntryNumber"));

  loadNexusPro->execute();

  if (!loadNexusPro->isExecuted())
    g_log.error("Unable to successfully run LoadISISNexus Child Algorithm");

  setOutputWorkspace(loadNexusPro);
}

void LoadNexus::runLoadTOFRawNexus() {
  auto loadNexusPro = createChildAlgorithm("LoadTOFRawNexus", 0., 1.);
  // Pass through the same input filename
  loadNexusPro->setPropertyValue("Filename", m_filename);
  // Set the workspace property
  std::string outputWorkspace = "OutputWorkspace";
  loadNexusPro->setPropertyValue(outputWorkspace, m_workspace);
  // Get the array passed in the spectrum_list, if an empty array was passed use
  // the default
  std::vector<int> specList = getProperty("SpectrumList");
  if (!specList.empty())
    loadNexusPro->setPropertyValue("SpectrumList", getPropertyValue("SpectrumList"));
  //
  int specMax = getProperty("SpectrumMax");
  if (specMax != Mantid::EMPTY_INT()) {
    loadNexusPro->setPropertyValue("SpectrumMax", getPropertyValue("SpectrumMax"));
    loadNexusPro->setPropertyValue("SpectrumMin", getPropertyValue("SpectrumMin"));
  }

  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    loadNexusPro->execute();
  } catch (std::runtime_error &) {
    g_log.error("Unable to successfully run LoadTOFRawNexus Child Algorithm");
  }
  if (!loadNexusPro->isExecuted())
    g_log.error("Unable to successfully run LoadTOFRawNexus Child Algorithm");

  setOutputWorkspace(loadNexusPro);
}

/**
 * Set the output workspace(s) if the load's return workspace has type
 * API::Workspace
 * @param loader :: Shared pointer to load algorithm
 */
void LoadNexus::setOutputWorkspace(const API::IAlgorithm_sptr &loader) {
  // Go through each OutputWorkspace property and check whether we need to make
  // a counterpart here
  const std::vector<Property *> &loaderProps = loader->getProperties();
  const size_t count = loader->propertyCount();
  for (size_t i = 0; i < count; ++i) {
    Property *prop = loaderProps[i];
    if (dynamic_cast<IWorkspaceProperty *>(prop) && prop->direction() == Direction::Output) {
      const std::string &propName = prop->name();
      if (!this->existsProperty(propName)) {
        declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(propName, loader->getPropertyValue(propName),
                                                                       Direction::Output));
      }
      Workspace_sptr wkspace = loader->getProperty(propName);
      setProperty(propName, wkspace);
    }
  }
}

/** Get all the Nexus entry types for a file
 *
 * Try to open named Nexus file and return all entries plus the definition found
 *for each.
 * If definition not found, try and return "analysis" field (Muon V1 files)
 * Closes file on exit.
 *
 * @param fileName :: file to open
 * @param entryName :: vector that gets filled with strings with entry names
 * @param definition :: vector that gets filled with the "definition" or
 *"analysis" string.
 * @return count of entries if OK, -1 failed to open file.
 */
int LoadNexus::getNexusEntryTypes(const std::string &fileName, std::vector<std::string> &entryName,
                                  std::vector<std::string> &definition) {
  std::unique_ptr<::NeXus::File> fileH;

  try {
    fileH = std::make_unique<::NeXus::File>(fileName);
  } catch (::NeXus::Exception &) {
    return -1;
  }
  entryName.clear();
  definition.clear();

  //
  // Loop through all entries looking for the definition section in each (or
  // analysis for MuonV1)
  //
  std::vector<std::string> entryList;

  std::pair<std::string, std::string> entry;
  while (true) {
    entry = fileH->getNextEntry();
    if (entry == ::NeXus::EOD_ENTRY)
      break;

    if (entry.second == "NXentry")
      entryList.emplace_back(entry.first);
  }

  // for each entry found, look for "analysis" or "definition" text data fields
  // and return value plus entry name

  for (auto &item : entryList) {
    fileH->openGroup(item, "NXentry");
    // loop through field names in this entry
    while (true) {
      entry = fileH->getNextEntry();
      if (entry == ::NeXus::EOD_ENTRY)
        break;
      // if a data field
      if (entry.second == "SDS") {
        // if one of the two names we are looking for
        if (entry.first == "definition" || entry.first == "analysis") {
          std::string value;
          fileH->readData(entry.first, value);
          definition.emplace_back(value);
          entryName.emplace_back(item);
        }
      }
    }
    fileH->closeGroup();
  }

  return (static_cast<int>(entryName.size()));
}

} // namespace Mantid::DataHandling
