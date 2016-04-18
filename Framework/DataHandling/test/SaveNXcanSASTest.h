#ifndef MANTID_DATAHANDLING_SAVENXCANSASTEST_H_
#define MANTID_DATAHANDLING_SAVENXCANSASTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/SaveNXcanSAS.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidDataHandling/H5Util.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitFactory.h"
#include <Poco/File.h>
#include <H5Cpp.h>
#include <sstream>

namespace {
const std::string nxclass = "NX_class";
const std::string suffix = "01";

struct NXcanSASTestParameters {
  NXcanSASTestParameters() { initParameters(); }

  void initParameters() {
    filename = "SaveNXcanSASTestFile.h5";
    size = 100;
    value = 10.23;
    error = 3.45;
    xerror = 2.3759 / 3.6;
    hasDx = true;
    xmin = 1.0;
    xmax = 100.0;
    runNumber = "1234";
    userFile = "my_user_file";
    workspaceTitle = "sample_worksapce";
    instrumentName = "SANS2D";
    radiationSource = "Spallation Neutron Source";
    invalidDetectors = false;
    ymin = 1.0;
    ymax = 120.0;
    is2dData = false;
  }

  std::string filename;
  int size;
  double value;
  double error;
  double xerror;
  bool hasDx;
  double xmin;
  double xmax;
  double ymin;
  double ymax;
  std::string runNumber;
  std::string userFile;
  std::string workspaceTitle;
  std::string instrumentName;
  std::string radiationSource;
  std::vector<std::string> detectors;
  bool invalidDetectors;
  bool is2dData;
  std::string idf;
};

struct NXcanSASTestTransmissionParameters {
  NXcanSASTestTransmissionParameters() { initParameters(); }
  void initParameters() {
    size = 100;
    value = 12.34;
    error = 3.2345;
    xmin = 1.0;
    xmax = 100.0;
    usesTransmission = false;
  }

  int size;
  double value;
  double error;
  double xmin;
  double xmax;
  std::string name;
  bool usesTransmission;
};

std::string concatenateStringVector(std::vector<std::string> stringVector) {
  std::ostringstream os;
  for (auto &element : stringVector) {
    os << element;
    os << Mantid::DataHandling::NXcanSAS::sasSeparator;
  }

  return os.str();
}

std::string getIDFfromWorkspace(Mantid::API::MatrixWorkspace_sptr workspace) {
  auto instrument = workspace->getInstrument();
  auto name = instrument->getFullName();
  auto date = workspace->getWorkspaceStartDate();
  return workspace->getInstrumentFilename(name, date);
}

void setXValuesOn1DWorkspaceWithPointData(
    Mantid::API::MatrixWorkspace_sptr workspace, double xmin, double xmax) {
  auto &xValues = workspace->dataX(0);
  auto size = xValues.size();
  double binWidth = (xmax - xmin) / static_cast<double>(size - 1);
  for (size_t index = 0; index < size; ++index) {
    xValues[index] = xmin;
    xmin += binWidth;
  }
}
}

using Mantid::DataHandling::SaveNXcanSAS;
using namespace Mantid::DataHandling::NXcanSAS;

class SaveNXcanSASTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created
  // statically
  // This means the constructor isn't called when running other tests
  static SaveNXcanSASTest *createSuite() { return new SaveNXcanSASTest(); }
  static void destroySuite(SaveNXcanSASTest *suite) { delete suite; }

  void test_that_workspace_without_momentum_transfer_units_is_invalid() {
    // Arrange
    auto ws = WorkspaceCreationHelper::Create1DWorkspaceConstantWithXerror(
        10 /*size*/, 1.23 /*value&*/, 2.3 /*error*/, 23.4 /*xerror*/);
    const std::string filename = "SaveNXcanSASTestFile.h5";

    // Act + Assert
    auto saveAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "SaveNXcanSAS");
    saveAlg->setChild(true);
    saveAlg->initialize();
    saveAlg->setProperty("Filename", filename);
    TSM_ASSERT_THROWS_ANYTHING(
        "Should not save file without momentum transfer units.",
        saveAlg->setProperty("InputWorkspace", ws));
  }

  void test_that_histogram_data_cannot_be_saved() {
    // Arrange
    auto ws = WorkspaceCreationHelper::Create2DWorkspaceBinned(
        1 /*nhist*/, 10 /*nbins*/, 1.0 /*xmin*/, 1.0 /*increment*/);
    ws->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("MomentumTransfer");
    const std::string filename = "SaveNXcanSASTestFile.h5";
    // Act + Assert
    auto saveAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "SaveNXcanSAS");
    saveAlg->initialize();
    saveAlg->setProperty("Filename", filename);
    saveAlg->setProperty("InputWorkspace", ws);
    TSM_ASSERT_THROWS_ANYTHING(
        "Should not run since we are providing a histogram.",
        saveAlg->execute());
  }

  void test_that_1D_workspace_without_transmissions_is_saved_correctly() {
    // Arrange
    NXcanSASTestParameters parameters;
    removeFile(parameters.filename);

    parameters.detectors.push_back("front-detector");
    parameters.detectors.push_back("rear-detector");
    parameters.invalidDetectors = false;

    auto ws = provide1DWorkspace(parameters);
    setXValuesOn1DWorkspaceWithPointData(ws, parameters.xmin, parameters.xmax);

    parameters.idf = getIDFfromWorkspace(ws);

    // Act
    save_file_no_issues(ws, parameters);

    // Assert
    do_assert(parameters);

    // Clean up
    removeFile(parameters.filename);
  }

  void test_that_unknown_detector_names_are_not_saved() {
    // Arrange
    NXcanSASTestParameters parameters;
    removeFile(parameters.filename);

    parameters.detectors.push_back("wrong-detector1");
    parameters.detectors.push_back("wrong-detector2");
    parameters.invalidDetectors = true;

    auto ws = provide1DWorkspace(parameters);

    setXValuesOn1DWorkspaceWithPointData(ws, parameters.xmin, parameters.xmax);

    parameters.idf = getIDFfromWorkspace(ws);

    // Act
    save_file_no_issues(ws, parameters);

    // Assert
    do_assert(parameters);

    // Clean up
    removeFile(parameters.filename);
  }

  void
  test_that_1D_workspace_without_transmissions_and_without_xerror_is_saved_correctly() {
    // Arrange
    NXcanSASTestParameters parameters;
    removeFile(parameters.filename);

    parameters.detectors.push_back("front-detector");
    parameters.detectors.push_back("rear-detector");
    parameters.invalidDetectors = false;

    parameters.hasDx = false;

    auto ws = provide1DWorkspace(parameters);
    setXValuesOn1DWorkspaceWithPointData(ws, parameters.xmin, parameters.xmax);

    parameters.idf = getIDFfromWorkspace(ws);

    // Act
    save_file_no_issues(ws, parameters);

    // Assert
    do_assert(parameters);

    // Clean up
    removeFile(parameters.filename);
  }

  void test_that_1D_workspace_with_transmissions_is_saved_correctly() {
    // Arrange
    NXcanSASTestParameters parameters;
    removeFile(parameters.filename);

    parameters.detectors.push_back("front-detector");
    parameters.detectors.push_back("rear-detector");
    parameters.invalidDetectors = false;

    parameters.hasDx = true;

    auto ws = provide1DWorkspace(parameters);
    setXValuesOn1DWorkspaceWithPointData(ws, parameters.xmin, parameters.xmax);

    parameters.idf = getIDFfromWorkspace(ws);

    // Create transmission
    NXcanSASTestTransmissionParameters transmissionParameters;
    transmissionParameters.name = sasTransmissionSpectrumNameSampleAttrValue;
    transmissionParameters.usesTransmission = true;

    NXcanSASTestTransmissionParameters transmissionCanParameters;
    transmissionCanParameters.name = sasTransmissionSpectrumNameCanAttrValue;
    transmissionCanParameters.usesTransmission = true;

    auto transmission = getTransmissionWorkspace(transmissionParameters);
    setXValuesOn1DWorkspaceWithPointData(
        transmission, transmissionParameters.xmin, transmissionParameters.xmax);

    auto transmissionCan = getTransmissionWorkspace(transmissionCanParameters);
    setXValuesOn1DWorkspaceWithPointData(transmissionCan,
                                         transmissionCanParameters.xmin,
                                         transmissionCanParameters.xmax);

    // Act
    save_file_no_issues(ws, parameters, transmission, transmissionCan);

    // Assert
    do_assert(parameters, transmissionParameters, transmissionCanParameters);

    // Clean up
    removeFile(parameters.filename);
  }

  void test_that_2D_workspace_is_saved_correctly() {
    // Arrange
    // Arrange
    NXcanSASTestParameters parameters;
    removeFile(parameters.filename);

    parameters.detectors.push_back("front-detector");
    parameters.detectors.push_back("rear-detector");
    parameters.invalidDetectors = false;

    parameters.is2dData = true;

    auto ws = provide2DWorkspace(parameters);
    set2DValues(ws);

    parameters.idf = getIDFfromWorkspace(ws);

    // Act
    save_file_no_issues(ws, parameters);

    // Assert
    do_assert(parameters);

    // Clean up
    // removeFile(parameters.filename);
  }

private:
  void removeFile(std::string filename) {
    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }

  void add_sample_log(Mantid::API::MatrixWorkspace_sptr workspace,
                      const std::string &logName, const std::string &logValue) {
    auto logAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "AddSampleLog");
    logAlg->initialize();
    logAlg->setChild(true);
    logAlg->setProperty("Workspace", workspace);
    logAlg->setProperty("LogName", logName);
    logAlg->setProperty("LogText", logValue);
    logAlg->execute();
  }

  void set_logs(Mantid::API::MatrixWorkspace_sptr workspace,
                const std::string &runNumber, const std::string &userFile) {
    if (!runNumber.empty()) {
      add_sample_log(workspace, "run_number", runNumber);
    }

    if (!userFile.empty()) {
      add_sample_log(workspace, "UserFile", userFile);
    }
  }

  void set_instrument(Mantid::API::MatrixWorkspace_sptr workspace,
                      const std::string &instrumentName) {
    auto instAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "LoadInstrument");
    instAlg->initialize();
    instAlg->setChild(true);
    instAlg->setProperty("Workspace", workspace);
    instAlg->setProperty("InstrumentName", instrumentName);
    instAlg->setProperty("RewriteSpectraMap", "False");
    instAlg->execute();
  }

  Mantid::API::MatrixWorkspace_sptr
  provide1DWorkspace(NXcanSASTestParameters &parameters) {
    Mantid::API::MatrixWorkspace_sptr ws;
    if (parameters.hasDx) {
      ws = WorkspaceCreationHelper::Create1DWorkspaceConstantWithXerror(
          parameters.size, parameters.value, parameters.error,
          parameters.xerror);
    } else {
      ws = WorkspaceCreationHelper::Create1DWorkspaceConstant(
          parameters.size, parameters.value, parameters.error);
    }

    ws->setTitle(parameters.workspaceTitle);
    ws->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("MomentumTransfer");

    // Add sample logs
    set_logs(ws, parameters.runNumber, parameters.userFile);

    // Set instrument
    set_instrument(ws, parameters.instrumentName);

    return ws;
  }

  Mantid::API::MatrixWorkspace_sptr
  getTransmissionWorkspace(NXcanSASTestTransmissionParameters &parameters) {
    auto ws = WorkspaceCreationHelper::Create1DWorkspaceConstant(
        parameters.size, parameters.value, parameters.error);
    ws->setTitle(parameters.name);
    ws->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    return ws;
  }

  Mantid::API::MatrixWorkspace_sptr
  provide2DWorkspace(NXcanSASTestParameters &parameters) {
    auto ws = provide1DWorkspace(parameters);

    std::string axisBinning = std::to_string(parameters.xmin) + ",1," +
                              std::to_string(parameters.xmax);
    std::string axis2Binning = std::to_string(parameters.ymin) + ",1," +
                               std::to_string(parameters.ymax);

    // Convert to Histogram data
    auto toHistAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "ConvertToHistogram");
    std::string toHistoOutputName("toHistOutput");
    toHistAlg->initialize();
    toHistAlg->setChild(true);
    toHistAlg->setProperty("InputWorkspace", ws);
    toHistAlg->setProperty("OutputWorkspace", toHistoOutputName);
    toHistAlg->execute();
    ws = toHistAlg->getProperty("OutputWorkspace");

    // Convert Spectrum Axis
    auto axisAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "ConvertSpectrumAxis");
    std::string toAxisOutputName("toAxisOutput");
    axisAlg->initialize();
    axisAlg->setChild(true);
    axisAlg->setProperty("InputWorkspace", ws);
    axisAlg->setProperty("OutputWorkspace", toAxisOutputName);
    axisAlg->setProperty("Target", "Theta");
    axisAlg->execute();
    ws = axisAlg->getProperty("OutputWorkspace");

    // Rebin 2D
    auto rebin2DAlg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("Rebin2D");
    std::string rebinOutputName("rebinOutput");
    rebin2DAlg->initialize();
    rebin2DAlg->setChild(true);
    rebin2DAlg->setProperty("InputWorkspace", ws);
    rebin2DAlg->setProperty("OutputWorkspace", rebinOutputName);
    rebin2DAlg->setProperty("Axis1Binning", axisBinning);
    rebin2DAlg->setProperty("Axis2Binning", axis2Binning);
    rebin2DAlg->execute();
    ws = rebin2DAlg->getProperty("OutputWorkspace");

    // Convert to Point data
    auto toPointAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "ConvertToPointData");
    std::string toPointOutputName("toPointOutput");
    toPointAlg->initialize();
    toPointAlg->setChild(true);
    toPointAlg->setProperty("InputWorkspace", ws);
    toPointAlg->setProperty("OutputWorkspace", toPointOutputName);
    toPointAlg->execute();
    ws = toPointAlg->getProperty("OutputWorkspace");

    // Set the units to momentum transfer
    ws->getAxis(1)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("MomentumTransfer");

    return ws;
  }

  void set2DValues(Mantid::API::MatrixWorkspace_sptr ws) {
    const auto numberOfHistograms = ws->getNumberHistograms();

    for (size_t index = 0; index < numberOfHistograms; ++index) {
      auto &data = ws->dataY(index);
      data = Mantid::MantidVec(data.size(), static_cast<double>(index));
    }
  }

  void save_file_no_issues(
      Mantid::API::MatrixWorkspace_sptr workspace,
      NXcanSASTestParameters &parameters,
      Mantid::API::MatrixWorkspace_sptr transmission = nullptr,
      Mantid::API::MatrixWorkspace_sptr transmissionCan = nullptr) {
    auto saveAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "SaveNXcanSAS");
    saveAlg->initialize();
    saveAlg->setProperty("Filename", parameters.filename);
    saveAlg->setProperty("InputWorkspace", workspace);
    saveAlg->setProperty("RadiationSource", parameters.radiationSource);
    if (!parameters.detectors.empty()) {
      std::string detectorsAsString =
          concatenateStringVector(parameters.detectors);
      saveAlg->setProperty("DetectorNames", detectorsAsString);
    }

    if (transmission) {
      saveAlg->setProperty("Transmission", transmission);
    }
    if (transmissionCan) {
      saveAlg->setProperty("TransmissionCan", transmissionCan);
    }

    TSM_ASSERT_THROWS_NOTHING("Should not throw anything", saveAlg->execute());
    TSM_ASSERT("Should have executed", saveAlg->isExecuted());
  }

  void do_assert_sasentry(H5::Group &entry, const std::string &run,
                          const std::string &title) {
    using namespace Mantid::DataHandling::NXcanSAS;
    auto numAttributes = entry.getNumAttrs();
    TSM_ASSERT_EQUALS("Should have three attributes", 2, numAttributes);

    // NX_class attribute
    auto classAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(entry, nxclass);
    TSM_ASSERT_EQUALS("Should be SASentry class", classAttribute,
                      sasEntryClassAttr);

    // Version attribute
    auto versionAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
        entry, sasEntryVersionAttr);
    TSM_ASSERT_EQUALS("Version should be 1.0", versionAttribute,
                      sasEntryVersionAttrValue);

    // Definition data set
    auto definitionDataSet = entry.openDataSet(sasEntryDefinition);
    auto definitionValue =
        Mantid::DataHandling::H5Util::readString(definitionDataSet);
    TSM_ASSERT_EQUALS("File definition should be NXcanSAS", definitionValue,
                      sasEntryDefinitionFormat);

    // Run data set
    auto runDataSet = entry.openDataSet(sasEntryRun);
    auto runValue = Mantid::DataHandling::H5Util::readString(runDataSet);
    TSM_ASSERT_EQUALS("Run number should have been stored.", runValue, run);

    // Title data set
    auto titleDataSet = entry.openDataSet(sasEntryTitle);
    auto titleValue = Mantid::DataHandling::H5Util::readString(titleDataSet);
    TSM_ASSERT_EQUALS(
        "The title should have been stored as the workspace name.", titleValue,
        title);
  }

  void do_assert_source(H5::Group &source, const std::string &radiationSource) {

    auto numAttributes = source.getNumAttrs();
    TSM_ASSERT_EQUALS("Should have 1 attribute", 1, numAttributes);

    // NX_class attribute
    auto classAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(source, nxclass);
    TSM_ASSERT_EQUALS("Should be SASsource class", classAttribute,
                      sasInstrumentSourceClassAttr);

    // Radiation data set
    auto radiationDataSet = source.openDataSet(sasInstrumentSourceRadiation);
    auto radiationValue =
        Mantid::DataHandling::H5Util::readString(radiationDataSet);
    TSM_ASSERT_EQUALS("Radiation sources should match.", radiationValue,
                      radiationSource);
  }

  void do_assert_detector(H5::Group &instrument,
                          std::vector<std::string> detectors) {
    for (auto &detector : detectors) {
      auto detectorGroup =
          instrument.openGroup(sasInstrumentDetectorGroupName + detector);

      auto numAttributes = detectorGroup.getNumAttrs();
      TSM_ASSERT_EQUALS("Should have 1 attribute", 1, numAttributes);

      // NX_class attribute
      auto classAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
          detectorGroup, nxclass);
      TSM_ASSERT_EQUALS("Should be SASdetector class", classAttribute,
                        sasInstrumentDetectorClassAttr);

      // Detector name data set
      auto name = detectorGroup.openDataSet(sasInstrumentDetectorName);
      auto nameValue = Mantid::DataHandling::H5Util::readString(name);
      TSM_ASSERT_EQUALS("Radiation sources should match.", nameValue, detector);

      // SDD  data set
      auto sdd = detectorGroup.openDataSet(sasInstrumentDetectorSdd);
      TS_ASSERT_THROWS_NOTHING(Mantid::DataHandling::H5Util::readString(sdd));
    }
  }

  void do_assert_no_detectors(H5::Group &instrument) {
    // iterate over all groups and confirm that no SASdetecor is present
    auto numObjects = instrument.getNumObjs();
    for (hsize_t index = 0; index < numObjects; ++index) {
      auto objectType = instrument.getObjTypeByIdx(index);
      if (objectType == H5G_GROUP) {
        auto subGroupName = instrument.getObjnameByIdx(index);
        auto subGroup = instrument.openGroup(subGroupName);
        auto classAttribute =
            Mantid::DataHandling::H5Util::readAttributeAsString(subGroup,
                                                                nxclass);
        TSM_ASSERT("Should not be a detector",
                   classAttribute != sasInstrumentDetectorClassAttr);
      }
    }
  }

  void do_assert_instrument(H5::Group &instrument,
                            const std::string &instrumentName,
                            const std::string &idf,
                            const std::string &radiationSource,
                            const std::vector<std::string> &detectors,
                            bool invalidDetectors) {
    auto numAttributes = instrument.getNumAttrs();
    TSM_ASSERT_EQUALS("Should have 1 attribute", 1, numAttributes);

    // NX_class attribute
    auto classAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
        instrument, nxclass);
    TSM_ASSERT_EQUALS("Should be SASentry class", classAttribute,
                      sasInstrumentClassAttr);

    // Name data set
    auto instrumentNameDataSet = instrument.openDataSet(sasInstrumentName);
    auto instrumentNameValue =
        Mantid::DataHandling::H5Util::readString(instrumentNameDataSet);
    TSM_ASSERT_EQUALS("Name of the instrument should have been stored",
                      instrumentNameValue, instrumentName);

    // IDF data set
    auto idfDataSet = instrument.openDataSet(sasInstrumentIDF);
    auto idfValue = Mantid::DataHandling::H5Util::readString(idfDataSet);
    TSM_ASSERT_EQUALS("The idf should have been stored", idfValue, idf);

    // Check source
    auto source = instrument.openGroup(sasInstrumentSourceGroupName);
    do_assert_source(source, radiationSource);

    // Check detectors
    if (!invalidDetectors) {
      do_assert_detector(instrument, detectors);
    } else {
      // Make sure that no SASdetector group exists
      do_assert_no_detectors(instrument);
    }
  }

  void do_assert_process(H5::Group &process, const std::string &userFile) {
    auto numAttributes = process.getNumAttrs();
    TSM_ASSERT_EQUALS("Should have 1 attribute", 1, numAttributes);

    // NX_class attribute
    auto classAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(process, nxclass);
    TSM_ASSERT_EQUALS("Should be SASprocess class", classAttribute,
                      sasProcessClassAttr);

    // Date data set
    auto dateDataSet = process.openDataSet(sasProcessDate);
    TS_ASSERT_THROWS_NOTHING(
        Mantid::DataHandling::H5Util::readString(dateDataSet));

    // SVN data set
    auto svnDataSet = process.openDataSet(sasProcessTermSvn);
    TS_ASSERT_THROWS_NOTHING(
        Mantid::DataHandling::H5Util::readString(svnDataSet));

    // Name data set
    auto nameDataSet = process.openDataSet(sasProcessName);
    auto nameValue = Mantid::DataHandling::H5Util::readString(nameDataSet);
    TSM_ASSERT_EQUALS("Should have the Mantid NXcanSAS process name", nameValue,
                      sasProcessNameValue);

    // User file
    auto userFileDataSet = process.openDataSet(sasProcessTermUserFile);
    auto userFileValue =
        Mantid::DataHandling::H5Util::readString(userFileDataSet);
    TSM_ASSERT_EQUALS("Should have the Mantid NXcanSAS process name",
                      userFileValue, userFile);
  }

  void do_assert_1D_vector_with_same_entries(H5::DataSet &dataSet,
                                             double referenceValue, int size) {
    auto data =
        Mantid::DataHandling::H5Util::readArray1DCoerce<double>(dataSet);
    TS_ASSERT_EQUALS(data.size(), static_cast<size_t>(size));
    TS_ASSERT_EQUALS(data[0], referenceValue);
  }

  void do_assert_1D_vector_with_increasing_entries(H5::DataSet &dataSet,
                                                   double min, double increment,
                                                   int size) {
    auto data =
        Mantid::DataHandling::H5Util::readArray1DCoerce<double>(dataSet);
    TS_ASSERT_EQUALS(data.size(), static_cast<size_t>(size));
    for (size_t index = 0; index < data.size(); ++index) {
      TS_ASSERT_EQUALS(data[index], min);
      min += increment;
    }
  }

  void do_assert_that_Q_dev_information_is_not_present(H5::Group &data) {
    // Check that Q_uncertainty attribute is not saved
    bool missingQUncertaintyAttribute = false;
    try {
      data.openAttribute(sasDataQUncertaintyAttr);
    } catch (H5::AttributeIException &) {
      missingQUncertaintyAttribute = true;
    }
    TSM_ASSERT("Should not have a Q_uncertainty attribute",
               missingQUncertaintyAttribute);

    // Check that Qdev data set does not exist
    bool missingQDevDataSet = false;
    try {
      data.openDataSet(sasDataQdev);
    } catch (H5::GroupIException &) {
      missingQDevDataSet = true;
    } catch (H5::FileIException &) {
      missingQDevDataSet = true;
    }
    TSM_ASSERT("Should not have a Qdev data set", missingQDevDataSet);

    // Check that Q does not have an uncertainty set
    bool missingQUncertaintyOnQDataSet = false;
    try {
      auto qDataSet = data.openDataSet(sasDataQ);
      auto uncertainty = qDataSet.openAttribute(sasUncertaintyAttr);
    } catch (H5::AttributeIException &) {
      missingQUncertaintyOnQDataSet = true;
    }
    TSM_ASSERT("Data set should not have an uncertainty",
               missingQUncertaintyOnQDataSet);
  }

  void do_assert_data(H5::Group &data, int size, double value, double error,
                      double xmin, double xmax, double xerror, bool hasDx) {
    auto numAttributes = data.getNumAttrs();
    if (hasDx) {
      TSM_ASSERT_EQUALS("Should have 6 attribute", 6, numAttributes);
    } else {
      TSM_ASSERT_EQUALS(
          "Should have 5 attribute, since Q_uncertainty is not present", 5,
          numAttributes);
    }

    // NX_class attribute
    auto classAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(data, nxclass);
    TSM_ASSERT_EQUALS("Should be SASdata class", classAttribute,
                      sasDataClassAttr);

    // I_axes attribute
    auto intensityAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(data,
                                                            sasDataIAxesAttr);
    TSM_ASSERT_EQUALS("Should be just Q", intensityAttribute, sasDataQ);

    // I_uncertainty attribute
    auto errorAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
        data, sasDataIUncertaintyAttr);
    TSM_ASSERT_EQUALS("Should be just Idev", errorAttribute, sasDataIdev);

    // Q_indices attribute
    auto qAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
        data, sasDataQIndicesAttr);
    TSM_ASSERT_EQUALS("Should be just 0", qAttribute, "0");

    // Signal attribute
    auto signalAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(data, sasSignal);
    TSM_ASSERT_EQUALS("Should be just I", signalAttribute, sasDataI);

    // I data set
    auto intensityDataSet = data.openDataSet(sasDataI);
    do_assert_1D_vector_with_same_entries(intensityDataSet, value, size);

    // I data set uncertainty attribute
    auto uncertaintyIAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(intensityDataSet,
                                                            sasUncertaintyAttr);
    TSM_ASSERT_EQUALS("Should be just Idev", uncertaintyIAttribute,
                      sasDataIdev);

    // I dev data set
    auto errorDataSet = data.openDataSet(sasDataIdev);
    do_assert_1D_vector_with_same_entries(errorDataSet, error, size);

    // Q data set
    auto qDataSet = data.openDataSet(sasDataQ);
    double increment = (xmax - xmin) / static_cast<double>(size - 1);
    do_assert_1D_vector_with_increasing_entries(qDataSet, xmin, increment,
                                                size);

    if (hasDx) {
      // Q data set uncertainty attribute
      auto uncertaintyQAttribute =
          Mantid::DataHandling::H5Util::readAttributeAsString(
              qDataSet, sasUncertaintyAttr);
      TSM_ASSERT_EQUALS("Should be just Qdev", uncertaintyQAttribute,
                        sasDataQdev);

      // Q error data set
      auto xErrorDataSet = data.openDataSet(sasDataQdev);
      do_assert_1D_vector_with_same_entries(xErrorDataSet, xerror, size);

      // Q_uncertainty attribute on the SASdata group
      auto qErrorAttribute =
          Mantid::DataHandling::H5Util::readAttributeAsString(
              data, sasDataQUncertaintyAttr);
      TSM_ASSERT_EQUALS("Should be just Qdev", qErrorAttribute, sasDataQdev);
    } else {
      do_assert_that_Q_dev_information_is_not_present(data);
    }
  }

  void do_assert_2D_data(H5::Group &data, int size, double value, double error,
                         double xmin, double xmax, double ymin, double ymax) {
    auto numAttributes = data.getNumAttrs();
    TSM_ASSERT_EQUALS(
        "Should have 5 attributes, since Q_uncertainty is not present", 5,
        numAttributes);

    // NX_class attribute
    auto classAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(data, nxclass);
    TSM_ASSERT_EQUALS("Should be SASdata class", classAttribute,
                      sasDataClassAttr);

    // I_axes attribute
    auto intensityAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(data,
                                                            sasDataIAxesAttr);
    TSM_ASSERT_EQUALS("Should be just Q,Q", intensityAttribute,
                      sasDataQ + sasSeparator + sasDataQ);

    // I_uncertainty attribute
    auto errorAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
        data, sasDataIUncertaintyAttr);
    TSM_ASSERT_EQUALS("Should be just Idev", errorAttribute, sasDataIdev);

    // Q_indices attribute
    auto qAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
        data, sasDataQIndicesAttr);
    TSM_ASSERT_EQUALS("Should be just 0,1", qAttribute, "0,1");

    // Signal attribute
    auto signalAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(data, sasSignal);
    TSM_ASSERT_EQUALS("Should be just I", signalAttribute, sasDataI);

    // TODO: ADD MORE TESTING ONCE THE READER IS AVAILABLE
  }

  void do_assert_transmission(H5::Group &entry,
                              NXcanSASTestTransmissionParameters &parameters) {
    if (!parameters.usesTransmission) {
      return;
    }

    auto transmission = entry.openGroup(sasTransmissionSpectrumGroupName + "_" +
                                        parameters.name);

    // NX_class attribute
    auto classAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
        transmission, nxclass);
    TSM_ASSERT_EQUALS("Should be SAStransmission_spectrum class",
                      classAttribute, sasTransmissionSpectrumClassAttr);

    // Name attribute
    auto nameAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
        transmission, sasTransmissionSpectrumNameAttr);
    TSM_ASSERT_EQUALS("Should be either can or sample", nameAttribute,
                      parameters.name);

    // T indices attribute
    auto tIndicesAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(
            transmission, sasTransmissionSpectrumTIndices);
    TSM_ASSERT_EQUALS("Should be T", tIndicesAttribute,
                      sasTransmissionSpectrumT);

    // T uncertainty attribute
    auto tUncertaintyIndicesAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(
            transmission, sasTransmissionSpectrumTUncertainty);
    TSM_ASSERT_EQUALS("Should be Tdev", tUncertaintyIndicesAttribute,
                      sasTransmissionSpectrumTdev);

    // Signal attribute
    auto signalAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
        transmission, sasSignal);
    TSM_ASSERT_EQUALS("Should be T", signalAttribute, sasTransmissionSpectrumT);

    // Timestamp attribute
    TS_ASSERT_THROWS_NOTHING(
        Mantid::DataHandling::H5Util::readAttributeAsString(
            transmission, sasTransmissionSpectrumTimeStampAttr));

    // T data set
    auto tDataSet = transmission.openDataSet(sasTransmissionSpectrumT);
    do_assert_1D_vector_with_same_entries(tDataSet, parameters.value,
                                          parameters.size);

    // Tdev data set
    auto tErrorDataSet = transmission.openDataSet(sasTransmissionSpectrumTdev);
    do_assert_1D_vector_with_same_entries(tErrorDataSet, parameters.error,
                                          parameters.size);

    // Lambda data set
    auto lambdaDataSet =
        transmission.openDataSet(sasTransmissionSpectrumLambda);
    double increment = (parameters.xmax - parameters.xmin) /
                       static_cast<double>(parameters.size - 1);
    do_assert_1D_vector_with_increasing_entries(lambdaDataSet, parameters.xmin,
                                                increment, parameters.size);
  }

  void do_assert(NXcanSASTestParameters &parameters,
                 NXcanSASTestTransmissionParameters transmissionParameters =
                     NXcanSASTestTransmissionParameters(),
                 NXcanSASTestTransmissionParameters transmissionCanParameters =
                     NXcanSASTestTransmissionParameters()) {
    H5::H5File file(parameters.filename, H5F_ACC_RDONLY);

    // Check sasentry
    auto entry = file.openGroup(sasEntryGroupName + suffix);
    do_assert_sasentry(entry, parameters.runNumber, parameters.workspaceTitle);

    // Check instrument
    auto instrument = entry.openGroup(sasInstrumentGroupName);
    do_assert_instrument(instrument, parameters.instrumentName, parameters.idf,
                         parameters.radiationSource, parameters.detectors,
                         parameters.invalidDetectors);

    // Check process
    auto process = entry.openGroup(sasProcessGroupName);
    do_assert_process(process, parameters.userFile);

    // Check data
    auto data = entry.openGroup(sasDataGroupName);
    if (parameters.is2dData) {
      do_assert_2D_data(data, parameters.size, parameters.value,
                        parameters.error, parameters.xmin, parameters.xmax,
                        parameters.ymin, parameters.ymax);
    } else {
      do_assert_data(data, parameters.size, parameters.value, parameters.error,
                     parameters.xmin, parameters.xmax, parameters.xerror,
                     parameters.hasDx);
    }

    // Check the transmission
    do_assert_transmission(entry, transmissionParameters);

    // Check the transmission for the can
    do_assert_transmission(entry, transmissionCanParameters);

    file.close();
  }
};

#endif /* MANTID_DATAHANDLING_SAVENXCANSASTEST_H_ */
