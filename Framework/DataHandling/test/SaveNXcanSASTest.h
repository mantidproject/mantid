#ifndef MANTID_DATAHANDLING_SAVENXCANSASTEST_H_
#define MANTID_DATAHANDLING_SAVENXCANSASTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/H5Util.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidDataHandling/SaveNXcanSAS.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "NXcanSASTestHelper.h"

#include <H5Cpp.h>
#include <Poco/File.h>
#include <sstream>

namespace {
const std::string sasclass = "canSAS_class";
const std::string nxclass = "NX_class";
const std::string suffix = "01";
} // namespace

using Mantid::DataHandling::SaveNXcanSAS;
using namespace Mantid::DataHandling::NXcanSAS;
using namespace NXcanSASTestHelper;

class SaveNXcanSASTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created
  // statically
  // This means the constructor isn't called when running other tests
  static SaveNXcanSASTest *createSuite() { return new SaveNXcanSASTest(); }
  static void destroySuite(SaveNXcanSASTest *suite) { delete suite; }

  void test_that_workspace_without_momentum_transfer_units_is_invalid() {
    // Arrange
    auto ws = WorkspaceCreationHelper::create1DWorkspaceConstantWithXerror(
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
    removeFile(parameters.filename);
  }

  void test_that_2D_workspace_histogram_is_saved_correctly() {
    // Arrange
    NXcanSASTestParameters parameters;
    removeFile(parameters.filename);

    parameters.detectors.push_back("front-detector");
    parameters.detectors.push_back("rear-detector");
    parameters.invalidDetectors = false;

    parameters.is2dData = true;
    parameters.isHistogram = true; // The new bit

    auto ws = provide2DWorkspace(parameters);
    set2DValues(ws);

    parameters.idf = getIDFfromWorkspace(ws);

    // Act
    save_file_no_issues(ws, parameters);

    // Assert
    do_assert(parameters);

    // Clean up
    removeFile(parameters.filename);
  }

private:
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
    TSM_ASSERT_EQUALS("Should have three attributes", 3, numAttributes);

    // canSAS_class and NX_class attribute
    auto classAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(entry, sasclass);
    TSM_ASSERT_EQUALS("Should be SASentry class", classAttribute,
                      sasEntryClassAttr);
    classAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(entry, nxclass);
    TSM_ASSERT_EQUALS("Should be NXentr class", classAttribute,
                      nxEntryClassAttr);

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
    TSM_ASSERT_EQUALS("Should have 2 attribute", 2, numAttributes);

    // canSAS_class and NX_class attribute
    auto classAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(source, sasclass);
    TSM_ASSERT_EQUALS("Should be SASsource class", classAttribute,
                      sasInstrumentSourceClassAttr);
    classAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(source, nxclass);
    TSM_ASSERT_EQUALS("Should be NXsource class", classAttribute,
                      nxInstrumentSourceClassAttr);

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
      std::string detectorName = sasInstrumentDetectorGroupName + detector;
      auto detectorNameSanitized =
          Mantid::DataHandling::makeCanSASRelaxedName(detectorName);
      auto detectorGroup = instrument.openGroup(detectorNameSanitized);

      auto numAttributes = detectorGroup.getNumAttrs();
      TSM_ASSERT_EQUALS("Should have 2 attributes", 2, numAttributes);

      // canSAS_class and NX_class attribute
      auto classAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
          detectorGroup, sasclass);
      TSM_ASSERT_EQUALS("Should be SASdetector class", classAttribute,
                        sasInstrumentDetectorClassAttr);
      classAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
          detectorGroup, nxclass);
      TSM_ASSERT_EQUALS("Should be NXdetector class", classAttribute,
                        nxInstrumentDetectorClassAttr);

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
        // canSAS_class and NX_class attribute
        auto classAttribute =
            Mantid::DataHandling::H5Util::readAttributeAsString(subGroup,
                                                                sasclass);
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
    TSM_ASSERT_EQUALS("Should have 2 attribute", 2, numAttributes);

    // canSAS_class and NX_class attribute
    auto classAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
        instrument, sasclass);
    TSM_ASSERT_EQUALS("Should be SASentry class", classAttribute,
                      sasInstrumentClassAttr);
    classAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
        instrument, nxclass);
    TSM_ASSERT_EQUALS("Should be NXentry class", classAttribute,
                      nxInstrumentClassAttr);

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
    TSM_ASSERT_EQUALS("Should have 2 attribute", 2, numAttributes);

    // canSAS_class and NX_class attribute
    auto classAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(process, sasclass);
    TSM_ASSERT_EQUALS("Should be SASprocess class", classAttribute,
                      sasProcessClassAttr);
    classAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(process, nxclass);
    TSM_ASSERT_EQUALS("Should be NXprocess class", classAttribute,
                      nxProcessClassAttr);

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
      TSM_ASSERT_EQUALS("Should have 7 attribute", 7, numAttributes);
    } else {
      TSM_ASSERT_EQUALS(
          "Should have 6 attribute, since Q_uncertainty is not present", 6,
          numAttributes);
    }

    // canSAS_class and NX_class attribute
    auto classAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(data, sasclass);
    TSM_ASSERT_EQUALS("Should be SASdata class", classAttribute,
                      sasDataClassAttr);
    classAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(data, nxclass);
    TSM_ASSERT_EQUALS("Should be NXdata class", classAttribute,
                      nxDataClassAttr);

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
    auto qAttribute =
        Mantid::DataHandling::H5Util::readNumArrayAttributeCoerce<int>(
            data, sasDataQIndicesAttr);
    TSM_ASSERT_EQUALS("Should be just 0", qAttribute, std::vector<int>{0});

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

  void do_assert_2D_data(H5::Group &data) {
    auto numAttributes = data.getNumAttrs();
    TSM_ASSERT_EQUALS(
        "Should have 6 attributes, since Q_uncertainty is not present", 6,
        numAttributes);

    // canSAS_class and NX_class attribute
    auto classAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(data, sasclass);
    TSM_ASSERT_EQUALS("Should be SASdata class", classAttribute,
                      sasDataClassAttr);
    classAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(data, nxclass);
    TSM_ASSERT_EQUALS("Should be NXdata class", classAttribute,
                      nxDataClassAttr);

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
    auto qAttribute =
        Mantid::DataHandling::H5Util::readNumArrayAttributeCoerce<int>(
            data, sasDataQIndicesAttr);
    std::vector<int> expectedQIndices{0, 1};
    TSM_ASSERT_EQUALS("Should be just 0,1", qAttribute, expectedQIndices);

    // Signal attribute
    auto signalAttribute =
        Mantid::DataHandling::H5Util::readAttributeAsString(data, sasSignal);
    TSM_ASSERT_EQUALS("Should be just I", signalAttribute, sasDataI);

    // Note: Acutal Values are being testin in LoadNXcanSAS to avoid redundant
    // testing
  }

  void do_assert_transmission(H5::Group &entry,
                              NXcanSASTestTransmissionParameters &parameters) {
    if (!parameters.usesTransmission) {
      return;
    }

    auto transmission = entry.openGroup(sasTransmissionSpectrumGroupName + "_" +
                                        parameters.name);

    // canSAS_class and NX_class attribute
    auto classAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
        transmission, sasclass);
    TSM_ASSERT_EQUALS("Should be SAStransmission_spectrum class",
                      classAttribute, sasTransmissionSpectrumClassAttr);
    classAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
        transmission, nxclass);
    TSM_ASSERT_EQUALS("Should be NXdata class", classAttribute,
                      nxTransmissionSpectrumClassAttr);

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
      do_assert_2D_data(data);
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
