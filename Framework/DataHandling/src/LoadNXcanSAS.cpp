#include "MantidDataHandling/LoadNXcanSAS.h"
#include "MantidKernel/make_unique.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FileFinder.h"
#include "MantidDataHandling/H5Util.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidKernel/Logger.h"

#include <H5Cpp.h>
#include <Poco/Path.h>
#include <Poco/DirectoryIterator.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataHandling::NXcanSAS;

namespace
{

Mantid::Kernel::Logger g_log("LoadNXcanSAS");

struct DataSpaceInformation {
    DataSpaceInformation(size_t dimSpectrumAxis = 0, size_t dimBin = 0)
        : dimSpectrumAxis(dimSpectrumAxis), dimBin(dimBin)
    {
    }
    size_t dimSpectrumAxis;
    size_t dimBin;
};

DataSpaceInformation getDataSpaceInfo(H5::DataSet &dataSet)
{
    DataSpaceInformation dataSpaceInfo;
    auto dataSpace = dataSet.getSpace();
    const auto rank = dataSpace.getSimpleExtentNdims();
    if (rank > 2) {
        std::invalid_argument("LoadNXcanSAS: Cannot load a data set "
                              "with more than 2 dimensions.");
    }

    hsize_t dims[2] = {0, 0};
    dataSpace.getSimpleExtentDims(dims);
    // If dims[1] is 0, then the first entry is the number of data points
    // If dims[1] is not 0, then the second entry is the number of data points
    if (dims[1] == 0) {
      dims[1] = dims[0];
      dims[0] = 1; // One histogram
    }

    dataSpaceInfo.dimSpectrumAxis = dims[0];
    dataSpaceInfo.dimBin = dims[1];
    return dataSpaceInfo;
}

std::string getNameOfEntry(H5::H5File &root)
{
    std::string entryName = "";
    auto numberOfObjects = root.getNumObjs();
    if (numberOfObjects != 1) {
        throw std::invalid_argument("LoadNXcanSAS: Trying to load multiperiod "
                                    "data. This is currently not supported.");
    }

    auto objectType = root.getObjTypeByIdx(0);
    if (objectType != H5G_GROUP) {
        throw std::invalid_argument(
            "LoadNXcanSAS: The object below the root is not a H5::Group.");
    }

    return root.getObjnameByIdx(0);
}

Mantid::API::MatrixWorkspace_sptr createWorkspace(H5::Group &entry)
{
    // Get the data space o the entry
    auto dataGroup = entry.openGroup(sasDataGroupName);
    auto intensity = dataGroup.openDataSet(sasDataI);
    auto dimInfo = getDataSpaceInfo(intensity);

    // Create a workspace based on the dataSpace information
    return Mantid::API::WorkspaceFactory::Instance().create(
        "Workspace2D", dimInfo.dimSpectrumAxis /*NHisto*/,
        dimInfo.dimBin /*xdata*/, dimInfo.dimBin /*ydata*/);
}

// ----- LOGS

void loadLogs(H5::Group &entry, Mantid::API::MatrixWorkspace_sptr workspace)
{
    auto &run = workspace->mutableRun();

    // Load UserFile (optional)
    auto process = entry.openGroup(sasProcessGroupName);
    auto userFile = Mantid::DataHandling::H5Util::readString(
        process, sasProcessTermUserFile);
    if (!userFile.empty()) {
        run.addLogData(new PropertyWithValue<std::string>(
            sasProcessUserFileInLogs, userFile));
    }

    // Load Run (optional)
    auto runNumber
        = Mantid::DataHandling::H5Util::readString(entry, sasEntryRun);
    if (!runNumber.empty()) {
        run.addLogData(
            new PropertyWithValue<std::string>(sasEntryRunInLogs, runNumber));
    }

    // Load Title (optional)
    auto title = Mantid::DataHandling::H5Util::readString(entry, sasEntryTitle);
    if (!title.empty()) {
        workspace->setTitle(title);
    }
}

// ----- INSTRUMENT
std::string extractIdfFileOnCurrentSystem(std::string idf)
{
    // If the idf is is not empty extract the last element from the file
    if (idf.empty()) {
        return "";
    }

    // Get the specified IDF name
    Poco::Path path(idf);
    auto fileName = path.getFileName();

    // Compare against all available IDFs
    const std::vector<std::string> &directoryNames
        = Mantid::Kernel::ConfigService::Instance().getInstrumentDirectories();
    Poco::DirectoryIterator end_iter;
    for (const auto &directoryName : directoryNames) {
        for (Poco::DirectoryIterator dir_itr(directoryName);
             dir_itr != end_iter; ++dir_itr) {
            if (Poco::File(dir_itr->path()).isFile()) {
                if (fileName == Poco::Path(dir_itr->path()).getFileName()) {
                    return Poco::Path(dir_itr->path()).absolute().toString();
                }
            }
        }
    }
    return "";
}

void loadInstrument(H5::Group &entry,
                    Mantid::API::MatrixWorkspace_sptr workspace)
{
    auto instrument = entry.openGroup(sasInstrumentGroupName);

    // Get instrument name
    auto instrumentName = Mantid::DataHandling::H5Util::readString(
        instrument, sasInstrumentName);
    if (instrumentName.empty()) {
        return;
    }

    // Get IDF
    auto idf = Mantid::DataHandling::H5Util::readString(instrument,
                                                        sasInstrumentIDF);
    idf = extractIdfFileOnCurrentSystem(idf);

    // Try to load the instrument. If it fails we will continue nevertheless.
    try {
        auto instAlg
            = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
                "LoadInstrument");
        instAlg->initialize();
        instAlg->setChild(true);
        instAlg->setProperty("Workspace", workspace);
        instAlg->setProperty("InstrumentName", instrumentName);
        if (!idf.empty()) {
            instAlg->setProperty("Filename", idf);
        }
        instAlg->setProperty("RewriteSpectraMap", "True");
        instAlg->execute();
    } catch (std::invalid_argument &) {
        g_log.information(
            "Invalid argument to LoadInstrument Child Algorithm.");
    } catch (std::runtime_error &) {
        g_log.information(
            "Unable to successfully run LoadInstrument Child Algorithm.");
    }
}


// ----- DATA
WorkspaceDimensionality getWorkspaceDimensionality(H5::Group& dataGroup) {
  WorkspaceDimensionality dimensionality(WorkspaceDimensionality::other);
  auto intensity = dataGroup.openDataSet(sasDataI);
  auto dataSpaceInfo = getDataSpaceInfo(intensity);

  if (dataSpaceInfo.dimSpectrumAxis == 1 && dataSpaceInfo.dimBin > 0) {
    dimensionality = WorkspaceDimensionality::oneD;
  } else if (dataSpaceInfo.dimSpectrumAxis > 1 && dataSpaceInfo.dimBin > 0) {
    dimensionality = WorkspaceDimensionality::twoD;
  } else {
    dimensionality = WorkspaceDimensionality::other;
  }
  return dimensionality;
}

std::string getUnit(H5::DataSet& dataSet) {
  return Mantid::DataHandling::H5Util::readAttributeAsString(dataSet,sasUnitAttr);
}

void loadData1D(H5::Group& dataGroup, Mantid::API::MatrixWorkspace_sptr workspace) {
  // Load the Q value
  Mantid::MantidVec qData = Mantid::DataHandling::H5Util::readArray1DCoerce<double>(dataGroup, sasDataQ);
  auto& dataQ = workspace->dataX(0);
  dataQ.swap(qData);
  workspace->getAxis(0)->setUnit("MomentumTransfer");

  // Load the I value
  Mantid::MantidVec iData = Mantid::DataHandling::H5Util::readArray1DCoerce<double>(dataGroup, sasDataI);
  auto& dataI = workspace->dataY(0);
  dataI.swap(iData);

  // Load the Idev value
  Mantid::MantidVec iDevData = Mantid::DataHandling::H5Util::readArray1DCoerce<double>(dataGroup, sasDataIdev);
  auto& dataIdev = workspace->dataE(0);
  dataIdev.swap(iDevData);

  // Load the Qdev value (optional)

  std::cout << "======================" <<std::endl;
  std::cout << workspace->isHistogramData() <<std::endl;
  std::cout << "======================" <<std::endl;


}

void loadData(H5::Group& entry, Mantid::API::MatrixWorkspace_sptr workspace) {
  auto dataGroup = entry.openGroup(sasDataGroupName);
  auto dimensionality = getWorkspaceDimensionality(dataGroup);
  switch(dimensionality) {
   case(WorkspaceDimensionality::oneD):
      loadData1D(dataGroup, workspace);
      break;
   case(WorkspaceDimensionality::twoD):
      break;
   default:
      throw std::invalid_argument("LoadNXcanSAS: Cannot load workspace which not 1D or 2D.");
  }
}

}

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadNXcanSAS)

/// constructor
LoadNXcanSAS::LoadNXcanSAS() {}

void LoadNXcanSAS::init()
{
    // Declare required input parameters for algorithm
    const std::vector<std::string> exts{".nxs", ".h5"};
    declareProperty(
        Kernel::make_unique<Mantid::API::FileProperty>(
            "Filename", "", FileProperty::Load, exts),
        "The name of the NXcanSAS file to read, as a full or relative path.");
    declareProperty(
        Kernel::
            make_unique<Mantid::API::WorkspaceProperty<Mantid::API::Workspace>>(
                "OutputWorkspace", "", Direction::Output),
        "The name of the workspace to be created as the output of "
        "the algorithm.  A workspace of this name will be created "
        "and stored in the Analysis Data Service. For multiperiod "
        "files, one workspace may be generated for each period. "
        "Currently only one workspace can be saved at a time so "
        "multiperiod Mantid files are not generated.");

    declareProperty(
        Kernel::make_unique<PropertyWithValue<bool>>("LoadTransmission", false,
                                                     Direction::Input),
        "Load the transmission related data from the file if it is present "
        "(optional, default False).");
}

int LoadNXcanSAS::confidence(Kernel::FileDescriptor &descriptor) const
{
    return 0;
}

void LoadNXcanSAS::exec()
{
    const std::string fileName = getPropertyValue("Filename");
    const bool loadTransmissions = getProperty("LoadTransmission");
    H5::H5File file(fileName, H5F_ACC_RDONLY);

    auto entryName = getNameOfEntry(file);
    auto entry = file.openGroup(entryName);

    // Create the output workspace
    auto ws = createWorkspace(entry);

    // Load the logs
    loadLogs(entry, ws);

    // Load instrument
    loadInstrument(entry, ws);

    // Load data
    loadData(entry, ws);

    // Load Transmissions
    if (loadTransmissions) {
        // Load sample transmission

        // Load can transmission
    }
    file.close();
    setProperty("OutputWorkspace", ws);
}

} // namespace DataHandling
} // namespace Mantid
