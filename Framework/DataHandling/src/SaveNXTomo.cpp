// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveNXTomo.h"

#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidDataHandling/FindDetectorsPar.h"

#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"

#include "MantidKernel/CompositeValidator.h"

#include "MantidKernel/MantidVersion.h"

#include "MantidNexusCpp/NeXusException.hpp"

#include "MantidNexusCpp/NeXusFile.hpp"

namespace Mantid::DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveNXTomo)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

const std::string SaveNXTomo::NXTOMO_VER = "2.0";

SaveNXTomo::SaveNXTomo()
    : API::Algorithm(), m_includeError(false), m_overwriteFile(false), m_spectraCount(0), m_filename("") {}

/**
 * Initialise the algorithm
 */
void SaveNXTomo::init() {
  auto wsValidator = std::make_shared<CompositeValidator>();
  // Note: this would be better, but it is too restrictive in
  // practice when saving image workspaces loaded from different
  // formats than FITS or not so standard FITS.
  // wsValidator->add<API::CommonBinsValidator>();
  wsValidator->add<API::HistogramValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspaces", "", Direction::Input, wsValidator),
                  "The name of the workspace(s) to save - this can be the name of a single "
                  "Workspace2D or the name of a WorkspaceGroup in which case all the "
                  "Workspace2Ds included in the group will be saved.");

  declareProperty(
      std::make_unique<API::FileProperty>("Filename", "", FileProperty::Save, std::vector<std::string>(1, ".nxs")),
      "The name of the NXTomo file to write, as a full or relative path");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("OverwriteFile", false, Kernel::Direction::Input),
                  "Replace any existing file of the same name instead of appending data?");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("IncludeError", false, Kernel::Direction::Input),
                  "Write the error values to NXTomo file?");
}

/**
 * Execute the algorithm for the single workspace (no group) case. See
 * processGroups() for when a workspace group is given as input.
 */
void SaveNXTomo::exec() {
  try {
    MatrixWorkspace_sptr m = getProperty("InputWorkspaces");
    m_workspaces.emplace_back(std::dynamic_pointer_cast<Workspace2D>(m));
  } catch (...) {
  }

  if (!m_workspaces.empty())
    processAll();
}

/**
 * Run instead of exec when operating on groups
 */
bool SaveNXTomo::processGroups() {
  try {
    std::string wsName = getPropertyValue("InputWorkspaces");
    WorkspaceGroup_sptr groupWS = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(wsName);

    for (int i = 0; i < groupWS->getNumberOfEntries(); ++i) {
      m_workspaces.emplace_back(std::dynamic_pointer_cast<Workspace2D>(groupWS->getItem(i)));
    }
  } catch (...) {
  }

  if (!m_workspaces.empty())
    processAll();

  return true;
}

/**
 * Main exec routine, called for group or individual workspace processing.
 *
 * @throw NotImplementedError If input workspaces are invalid
 * @throw invalid_argument If workspace doesn't have common binning
 */
void SaveNXTomo::processAll() {
  m_includeError = getProperty("IncludeError");
  m_overwriteFile = getProperty("OverwriteFile");

  for (auto &workspace : m_workspaces) {
    const std::string workspaceID = workspace->id();

    if ((workspaceID.find("Workspace2D") == std::string::npos) &&
        (workspaceID.find("RebinnedOutput") == std::string::npos))
      throw Exception::NotImplementedError("SaveNXTomo passed invalid workspaces. Must be Workspace2D");
  }

  // Retrieve the filename from the properties
  this->m_filename = getPropertyValue("Filename");

  // Populate the dimension array - assume all are the same
  m_dimensions.emplace_back(m_workspaces.size());
  m_dimensions.emplace_back(boost::lexical_cast<int64_t>(m_workspaces[0]->mutableRun().getLogData("Axis1")->value()));
  m_dimensions.emplace_back(boost::lexical_cast<int64_t>(m_workspaces[0]->mutableRun().getLogData("Axis2")->value()));

  m_spectraCount = m_dimensions[1] * m_dimensions[2];

  // Define
  m_infDimensions = m_dimensions;
  m_infDimensions[0] = NX_UNLIMITED;

  // What size slabs are we going to write
  m_slabSize.emplace_back(1);
  m_slabSize.emplace_back(m_dimensions[1]);
  m_slabSize.emplace_back(m_dimensions[2]);

  // Init start to first row
  m_slabStart.emplace_back(0);
  m_slabStart.emplace_back(0);
  m_slabStart.emplace_back(0);

  ::NeXus::File nxFile = setupFile();

  // Create a progress reporting object
  Progress progress(this, 0.0, 1.0, m_workspaces.size());

  for (auto &workspace : m_workspaces) {
    writeSingleWorkspace(workspace, nxFile);
    progress.report();
  }

  nxFile.close();
}

/**
 * Creates the format for the output file if it doesn't exist
 * @returns the structured nexus file to write the data to
 *
 * @throw runtime_error Thrown if nexus file cannot be opened or created
 */
::NeXus::File SaveNXTomo::setupFile() {
  // Try and open the file, if it doesn't exist, create it.
  NXhandle fileHandle;
  NXstatus status = NXopen(this->m_filename.c_str(), NXACC_RDWR, &fileHandle);

  if (status != NX_ERROR && !m_overwriteFile) {
    // Appending to an existing file, return reference to the file
    ::NeXus::File nxFile(fileHandle);
    return nxFile;
  }

  // Either overwriting the file or creating a new one now
  if (status != NX_ERROR) {
    ::NeXus::File f(fileHandle);
    f.close();
  }

  // If not overwriting, ensure it has a .nxs extension
  if ((!m_overwriteFile || status == NX_ERROR) && !this->m_filename.ends_with(".nxs"))
    m_filename = m_filename + ".nxs";

  status = NXopen(this->m_filename.c_str(), NXACC_CREATE5, &fileHandle);

  if (status == NX_ERROR)
    throw std::runtime_error("Unable to open or create nexus file.");

  // *********************************
  // Now genererate file and structure

  ::NeXus::File nxFile(fileHandle);

  // Make the top level entry (and open it)
  nxFile.makeGroup("entry1", "NXentry", true);

  // Make an entry to store log values from the original files.
  nxFile.makeGroup("log_info", "NXsubentry", false);

  // Make a sub-group for the entry to work with DAWN software (and open it)
  nxFile.makeGroup("tomo_entry", "NXsubentry", true);

  // Title
  nxFile.writeData("title", this->m_filename);

  // Definition name and version
  nxFile.writeData("definition", "NXtomo");
  nxFile.openData("definition");
  nxFile.putAttr("version", NXTOMO_VER);
  nxFile.closeData();

  // Originating program name and version
  nxFile.writeData("program_name", "mantid");
  nxFile.openData("program_name");
  nxFile.putAttr("version", Mantid::Kernel::MantidVersion::version());
  nxFile.closeData();

  // ******************************************
  // NXinstrument
  nxFile.makeGroup("instrument", "NXinstrument", true);
  // Write the instrument name | could add short_name attribute to name
  nxFile.writeData("name", m_workspaces[0]->getInstrument()->getName());

  // detector group - diamond example file contains
  // {data,distance,image_key,x_pixel_size,y_pixel_size}
  nxFile.makeGroup("detector", "NXdetector", true);

  std::vector<int64_t> infDim;
  infDim.emplace_back(NX_UNLIMITED);

  nxFile.makeData("image_key", ::NeXus::FLOAT64, infDim, false);
  nxFile.closeGroup(); // detector

  // source group // from diamond file contains {current,energy,name,probe,type}
  // - probe = [neutron | x-ray | electron]

  nxFile.closeGroup(); // NXinstrument

  // ******************************************
  // NXsample
  nxFile.makeGroup("sample", "NXsample", true);

  nxFile.makeData("rotation_angle", ::NeXus::FLOAT64, infDim, true);
  // Create a link object for rotation_angle to use later
  NXlink rotationLink = nxFile.getDataID();
  nxFile.closeData();
  nxFile.closeGroup(); // NXsample

  // ******************************************
  // Make the NXmonitor group - Holds base beam intensity for each image

  nxFile.makeGroup("control", "NXmonitor", true);
  nxFile.makeData("data", ::NeXus::FLOAT64, infDim, false);
  nxFile.closeGroup(); // NXmonitor

  nxFile.makeGroup("data", "NXdata", true);
  nxFile.putAttr<int>("NumFiles", 0);

  nxFile.makeLink(rotationLink);

  nxFile.makeData("data", ::NeXus::FLOAT64, m_infDimensions, true);
  // Create a link object for the data
  NXlink dataLink = nxFile.getDataID();
  nxFile.closeData();

  if (m_includeError)
    nxFile.makeData("error", ::NeXus::FLOAT64, m_infDimensions, false);

  nxFile.closeGroup(); // Close Data group

  // Put a link to the data in instrument/detector
  nxFile.openGroup("instrument", "NXinstrument");
  nxFile.openGroup("detector", "NXdetector");
  nxFile.makeLink(dataLink);
  nxFile.closeGroup();
  nxFile.closeGroup();

  nxFile.closeGroup(); // tomo_entry sub-group
  nxFile.closeGroup(); // Top level NXentry

  return nxFile;
}

/**
 * Writes a single workspace into the file
 * @param workspace the workspace to get data from
 * @param nxFile the nexus file to save data into
 */
void SaveNXTomo::writeSingleWorkspace(const Workspace2D_sptr &workspace, ::NeXus::File &nxFile) {
  try {
    nxFile.openPath("/entry1/tomo_entry/data");
  } catch (...) {
    throw std::runtime_error("Unable to create a valid NXTomo file");
  }

  int numFiles = 0;
  nxFile.getAttr<int>("NumFiles", numFiles);

  // Change slab start to after last data position
  m_slabStart[0] = numFiles;
  m_slabSize[0] = 1;

  // Set the rotation value for this WS
  std::vector<double> rotValue;
  rotValue.emplace_back(0);

  if (workspace->run().hasProperty("Rotation")) {
    std::string tmpVal = workspace->run().getLogData("Rotation")->value();
    try {
      rotValue[0] = boost::lexical_cast<double>(tmpVal);
    } catch (...) {
    }
    // Invalid Cast is handled below
  }

  nxFile.openData("rotation_angle");
  nxFile.putSlab(rotValue, numFiles, 1);
  nxFile.closeData();

  // Copy data out, remake data with dimension of old size plus new elements.
  // Insert previous data.
  nxFile.openData("data");

  auto dataArr = new double[m_spectraCount];

  // images can be as one-spectrum-per-pixel, or one-spectrum-per-row
  bool spectrumPerPixel = (1 == workspace->y(0).size());
  for (int64_t i = 0; i < m_dimensions[1]; ++i) {
    const auto &Y = workspace->y(i);
    for (int64_t j = 0; j < m_dimensions[2]; ++j) {
      if (spectrumPerPixel) {
        dataArr[i * m_dimensions[1] + j] = workspace->y(i * m_dimensions[1] + j)[0];
      } else {
        dataArr[i * m_dimensions[1] + j] = Y[j];
      }
    }
  }

  nxFile.putSlab(dataArr, m_slabStart, m_slabSize);

  nxFile.closeData();

  nxFile.putAttr("NumFiles", numFiles + 1);

  nxFile.closeGroup();

  // Write additional log information, intensity and image key
  writeLogValues(workspace, nxFile, numFiles);
  writeIntensityValue(workspace, nxFile, numFiles);
  writeImageKeyValue(workspace, nxFile, numFiles);
  delete[] dataArr;
}

void SaveNXTomo::writeImageKeyValue(const DataObjects::Workspace2D_sptr &workspace, ::NeXus::File &nxFile,
                                    int thisFileInd) {
  // Add ImageKey to instrument/image_key if present, use 0 if not
  try {
    nxFile.openPath("/entry1/tomo_entry/instrument/detector");
  } catch (...) {
    throw std::runtime_error("Unable to create a valid NXTomo file");
  }

  // Set the default key value for this WS
  std::vector<double> keyValue;
  keyValue.emplace_back(0);

  if (workspace->run().hasProperty("ImageKey")) {
    std::string tmpVal = workspace->run().getLogData("ImageKey")->value();
    try {
      keyValue[0] = boost::lexical_cast<double>(tmpVal);
    } catch (...) {
    }
    // Invalid Cast is handled below
  }

  nxFile.openData("image_key");
  nxFile.putSlab(keyValue, thisFileInd, 1);
  nxFile.closeData();

  nxFile.closeGroup();
}

void SaveNXTomo::writeLogValues(const DataObjects::Workspace2D_sptr &workspace, ::NeXus::File &nxFile,
                                int thisFileInd) {
  // Add Log information (minus special values - Rotation, ImageKey, Intensity)
  // Unable to add multidimensional string data, storing strings as
  // multidimensional data set of uint8 values
  try {
    nxFile.openPath("/entry1/log_info");
  } catch (...) {
    throw std::runtime_error("Unable to create a valid NXTomo file");
  }

  // Loop through all log values, create it if it doesn't exist. Then append
  // value
  const auto &logVals = workspace->run().getLogData();

  for (const auto &prop : logVals) {
    if (prop->name() != "ImageKey" && prop->name() != "Rotation" && prop->name() != "Intensity" &&
        prop->name() != "Axis1" && prop->name() != "Axis2") {
      try {
        nxFile.openData(prop->name());
      } catch (::NeXus::Exception &) {
        // Create the data entry if it doesn't exist yet, and open.
        std::vector<int64_t> infDim;
        infDim.emplace_back(NX_UNLIMITED);
        infDim.emplace_back(NX_UNLIMITED);
        nxFile.makeData(prop->name(), ::NeXus::UINT8, infDim, true);
      }
      auto valueAsStr = prop->value();
      size_t strSize = valueAsStr.length();
      // If log value is from FITS file as it should be,
      // it won't be greater than this. Otherwise Shorten it
      if (strSize > 80)
        strSize = 80;
      std::vector<int64_t> start = {thisFileInd, 0};
      std::vector<int64_t> size = {1, static_cast<int64_t>(strSize)};
      // single item
      nxFile.putSlab(valueAsStr.data(), start, size);

      nxFile.closeData();
    }
  }
}

void SaveNXTomo::writeIntensityValue(const DataObjects::Workspace2D_sptr &workspace, ::NeXus::File &nxFile,
                                     int thisFileInd) {
  // Add Intensity to control if present, use 1 if not
  try {
    nxFile.openPath("/entry1/tomo_entry/control");
  } catch (...) {
    throw std::runtime_error("Unable to create a valid NXTomo file");
  }

  std::vector<double> intensityValue;
  intensityValue.emplace_back(1);

  if (workspace->run().hasProperty("Intensity")) {
    std::string tmpVal = workspace->run().getLogData("Intensity")->value();
    try {
      intensityValue[0] = boost::lexical_cast<double>(tmpVal);
    } catch (...) {
    }
    // Invalid Cast is handled below
  }

  nxFile.openData("data");
  nxFile.putSlab(intensityValue, thisFileInd, 1);
  nxFile.closeData();
}

} // namespace Mantid::DataHandling
