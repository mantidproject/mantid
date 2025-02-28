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

#include "MantidNexus/NeXusException.hpp"

#include "MantidNexus/NeXusFile.hpp"

namespace Mantid::DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveNXTomo)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

const std::string SaveNXTomo::NXTOMO_VER = "2.0";

SaveNXTomo::SaveNXTomo()
    : API::Algorithm(), m_includeError(false), m_overwriteFile(false), m_spectraCount(0), m_filename(""),
      m_nxFile(nullptr) {}

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

  setupFile();

  // Create a progress reporting object
  Progress progress(this, 0.0, 1.0, m_workspaces.size());

  for (auto const &workspace : m_workspaces) {
    writeSingleWorkspace(workspace);
    progress.report();
  }

  m_nxFile->close();
}

/**
 * Creates the format for the output file if it doesn't exist
 *
 * @throw runtime_error Thrown if nexus file cannot be opened or created
 */
void SaveNXTomo::setupFile() {
  // if this pointer already points to a file, make sure it is closed
  if (m_nxFile) {
    m_nxFile->close();
    m_nxFile = nullptr;
  }

  // if we can overwrite, try to open the file
  if (!m_overwriteFile) {
    try {
      m_nxFile = std::make_unique<::NeXus::File>(m_filename, NXACC_RDWR);
      return;
    } catch (NeXus::Exception &) {
    }
  }

  // If not overwriting, or no existing file found above, create new file
  if (!m_nxFile) {
    m_nxFile = std::make_unique<::NeXus::File>(m_filename, NXACC_CREATE5);
  }
  // Make the top level entry (and open it)
  m_nxFile->makeGroup("entry1", "NXentry", true);

  // Make an entry to store log values from the original files.
  m_nxFile->makeGroup("log_info", "NXsubentry", false);

  // Make a sub-group for the entry to work with DAWN software (and open it)
  m_nxFile->makeGroup("tomo_entry", "NXsubentry", true);

  // Title
  m_nxFile->writeData("title", this->m_filename);

  // Definition name and version
  m_nxFile->writeData("definition", "NXtomo");
  m_nxFile->openData("definition");
  m_nxFile->putAttr("version", NXTOMO_VER);
  m_nxFile->closeData();

  // Originating program name and version
  m_nxFile->writeData("program_name", "mantid");
  m_nxFile->openData("program_name");
  m_nxFile->putAttr("version", Mantid::Kernel::MantidVersion::version());
  m_nxFile->closeData();

  // ******************************************
  // NXinstrument
  m_nxFile->makeGroup("instrument", "NXinstrument", true);
  // Write the instrument name | could add short_name attribute to name
  m_nxFile->writeData("name", m_workspaces[0]->getInstrument()->getName());

  // detector group - diamond example file contains
  // {data,distance,image_key,x_pixel_size,y_pixel_size}
  m_nxFile->makeGroup("detector", "NXdetector", true);

  std::vector<int64_t> infDim;
  infDim.emplace_back(NX_UNLIMITED);

  m_nxFile->makeData("image_key", NXnumtype::FLOAT64, infDim, false);
  m_nxFile->closeGroup(); // detector

  // source group // from diamond file contains {current,energy,name,probe,type}
  // - probe = [neutron | x-ray | electron]

  m_nxFile->closeGroup(); // NXinstrument

  // ******************************************
  // NXsample
  m_nxFile->makeGroup("sample", "NXsample", true);

  m_nxFile->makeData("rotation_angle", NXnumtype::FLOAT64, infDim, true);
  // Create a link object for rotation_angle to use later
  NXlink rotationLink = m_nxFile->getDataID();
  m_nxFile->closeData();
  m_nxFile->closeGroup(); // NXsample

  // ******************************************
  // Make the NXmonitor group - Holds base beam intensity for each image

  m_nxFile->makeGroup("control", "NXmonitor", true);
  m_nxFile->makeData("data", NXnumtype::FLOAT64, infDim, false);
  m_nxFile->closeGroup(); // NXmonitor

  m_nxFile->makeGroup("data", "NXdata", true);
  m_nxFile->putAttr<int>("NumFiles", 0);

  m_nxFile->makeLink(rotationLink);

  m_nxFile->makeData("data", NXnumtype::FLOAT64, m_infDimensions, true);
  // Create a link object for the data
  NXlink dataLink = m_nxFile->getDataID();
  m_nxFile->closeData();

  if (m_includeError)
    m_nxFile->makeData("error", NXnumtype::FLOAT64, m_infDimensions, false);

  m_nxFile->closeGroup(); // Close Data group

  // Put a link to the data in instrument/detector
  m_nxFile->openGroup("instrument", "NXinstrument");
  m_nxFile->openGroup("detector", "NXdetector");
  m_nxFile->makeLink(dataLink);
  m_nxFile->closeGroup();
  m_nxFile->closeGroup();

  m_nxFile->closeGroup(); // tomo_entry sub-group
  m_nxFile->closeGroup(); // Top level NXentry
}

/**
 * Writes a single workspace into the file
 * @param workspace the workspace to get data from
 */
void SaveNXTomo::writeSingleWorkspace(const Workspace2D_sptr &workspace) {
  try {
    m_nxFile->openPath("/entry1/tomo_entry/data");
  } catch (...) {
    throw std::runtime_error("Unable to create a valid NXTomo file");
  }

  int numFiles = 0;
  m_nxFile->getAttr<int>("NumFiles", numFiles);

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

  m_nxFile->openData("rotation_angle");
  m_nxFile->putSlab(rotValue, numFiles, 1);
  m_nxFile->closeData();

  // Copy data out, remake data with dimension of old size plus new elements.
  // Insert previous data.
  m_nxFile->openData("data");

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

  m_nxFile->putSlab(dataArr, m_slabStart, m_slabSize);

  m_nxFile->closeData();

  m_nxFile->putAttr("NumFiles", numFiles + 1);

  m_nxFile->closeGroup();

  // Write additional log information, intensity and image key
  writeLogValues(workspace, numFiles);
  writeIntensityValue(workspace, numFiles);
  writeImageKeyValue(workspace, numFiles);
  delete[] dataArr;
}

void SaveNXTomo::writeImageKeyValue(const DataObjects::Workspace2D_sptr &workspace, int thisFileInd) {
  // Add ImageKey to instrument/image_key if present, use 0 if not
  try {
    m_nxFile->openPath("/entry1/tomo_entry/instrument/detector");
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

  m_nxFile->openData("image_key");
  m_nxFile->putSlab(keyValue, thisFileInd, 1);
  m_nxFile->closeData();

  m_nxFile->closeGroup();
}

void SaveNXTomo::writeLogValues(const DataObjects::Workspace2D_sptr &workspace, int thisFileInd) {
  // Add Log information (minus special values - Rotation, ImageKey, Intensity)
  // Unable to add multidimensional string data, storing strings as
  // multidimensional data set of uint8 values
  try {
    m_nxFile->openPath("/entry1/log_info");
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
        m_nxFile->openData(prop->name());
      } catch (::NeXus::Exception &) {
        // Create the data entry if it doesn't exist yet, and open.
        std::vector<int64_t> infDim;
        infDim.emplace_back(NX_UNLIMITED);
        infDim.emplace_back(NX_UNLIMITED);
        m_nxFile->makeData(prop->name(), NXnumtype::UINT8, infDim, true);
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
      m_nxFile->putSlab(valueAsStr.data(), start, size);

      m_nxFile->closeData();
    }
  }
}

void SaveNXTomo::writeIntensityValue(const DataObjects::Workspace2D_sptr &workspace, int thisFileInd) {
  // Add Intensity to control if present, use 1 if not
  try {
    m_nxFile->openPath("/entry1/tomo_entry/control");
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

  m_nxFile->openData("data");
  m_nxFile->putSlab(intensityValue, thisFileInd, 1);
  m_nxFile->closeData();
}

} // namespace Mantid::DataHandling
