// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "../../Nexus/inc/MantidNexus/H5Util.h"
#include "../inc/MantidDataHandling/NXcanSASDefinitions.h"
#include "../inc/MantidDataHandling/NXcanSASHelper.h"

#include "NXcanSASTestHelper.h"

#include <string>
#include <vector>

using namespace Mantid::DataHandling::NXcanSAS;
using namespace Mantid::NeXus;
using namespace NXcanSASTestHelper;
namespace {
constexpr double DELTA = 1e-6;
}

class NXcanSASFileTest {
protected:
  void do_assert_sasentry(const H5::Group &entry, const std::string &run, const std::string &title) {
    assertNumberOfAttributes(entry, 3);

    assertStrAttribute(entry, sasCanSASclass, sasEntryClassAttr);
    assertStrAttribute(entry, sasNxclass, nxEntryClassAttr);
    assertStrAttribute(entry, sasEntryVersionAttr, sasEntryVersionAttrValue);

    // Definition data set
    assertStrDataSet(entry, sasEntryDefinition, sasEntryDefinitionFormat, "File definition should be NXcanSAS");
    // Run data set
    assertStrDataSet(entry, sasEntryRun, run, "Run number should have been stored");
    // Title data set
    assertStrDataSet(entry, sasEntryTitle, title, "The title should have been stored as the workspace name");
  }

  void do_assert_source(const H5::Group &source, const std::string &radiationSource) {
    assertNumberOfAttributes(source, 2);

    assertStrAttribute(source, sasCanSASclass, sasInstrumentSourceClassAttr);
    assertStrAttribute(source, sasNxclass, nxInstrumentSourceClassAttr);

    // Radiation data set
    assertStrDataSet(source, sasInstrumentSourceRadiation, radiationSource, "Radiation sources should match.");
  }

  void do_assert_aperture(const H5::Group &aperture, const std::string &beamShape, const double &beamHeight,
                          const double &beamWidth) {
    assertNumberOfAttributes(aperture, 2);

    assertStrAttribute(aperture, sasCanSASclass, sasInstrumentApertureClassAttr);
    assertStrAttribute(aperture, sasNxclass, nxInstrumentApertureClassAttr);

    // beam shape
    assertStrDataSet(aperture, sasInstrumentApertureShape, beamShape, "Beam shapes should match");
    // beam height
    assertNumArrDataSet(aperture, sasInstrumentApertureGapHeight, beamHeight, "Beam height should match.");
    // beam width
    assertNumArrDataSet(aperture, sasInstrumentApertureGapWidth, beamWidth, "Beam witdh should match.");
  }

  void do_assert_detector(const H5::Group &instrument, const std::vector<std::string> &detectors) {
    for (auto &detector : detectors) {
      std::string detectorName = sasInstrumentDetectorGroupName + detector;
      auto detectorNameSanitized = makeCanSASRelaxedName(detectorName);
      auto detectorGroup = instrument.openGroup(detectorNameSanitized);

      assertNumberOfAttributes(detectorGroup, 2);
      assertStrAttribute(detectorGroup, sasCanSASclass, sasInstrumentDetectorClassAttr);
      assertStrAttribute(detectorGroup, sasNxclass, nxInstrumentDetectorClassAttr);

      assertStrDataSet(detectorGroup, sasInstrumentDetectorName, detector);
      // SDD  data set
      assertDataSetDoesNotThrow(detectorGroup, sasInstrumentDetectorSdd);
    }
  }

  void do_assert_no_detectors(H5::Group &instrument) {
    // iterate over all groups and confirm that no SASdetector is present
    auto numObjects = instrument.getNumObjs();
    for (hsize_t index = 0; index < numObjects; ++index) {
      auto objectType = instrument.getObjTypeByIdx(index);
      if (objectType == H5G_GROUP) {
        auto subGroupName = instrument.getObjnameByIdx(index);
        auto subGroup = instrument.openGroup(subGroupName);
        // canSAS_class and NX_class attribute
        std::string classAttribute;
        H5Util::readStringAttribute(subGroup, sasCanSASclass, classAttribute);
        TSM_ASSERT("Should not be a detector", classAttribute != sasInstrumentDetectorClassAttr);
      }
    }
  }

  void do_assert_instrument(H5::Group &instrument, const std::string &instrumentName, const std::string &idf,
                            const std::string &radiationSource, const std::string &geometry, double beamHeight,
                            double beamWidth, const std::vector<std::string> &detectors, bool invalidDetectors) {
    assertNumberOfAttributes(instrument, 2);

    assertStrAttribute(instrument, sasCanSASclass, sasInstrumentClassAttr);
    assertStrAttribute(instrument, sasNxclass, nxInstrumentClassAttr);

    assertStrDataSet(instrument, sasInstrumentName, instrumentName, "Name of the instrument should have been stored");
    assertStrDataSet(instrument, sasInstrumentIDF, idf, "The idf should have been stored");

    // Check source
    auto source = instrument.openGroup(sasInstrumentSourceGroupName);
    do_assert_source(source, radiationSource);

    // Check aperture
    auto aperture = instrument.openGroup(sasInstrumentApertureGroupName);
    do_assert_aperture(aperture, geometry, beamHeight, beamWidth);

    // Check detectors
    if (!invalidDetectors) {
      do_assert_detector(instrument, detectors);
    } else {
      // Make sure that no SASdetector group exists
      do_assert_no_detectors(instrument);
    }
  }

  void do_assert_sample(const H5::Group &sample, double thickness) {
    assertNumberOfAttributes(sample, 2);
    assertNumArrDataSet(sample, sasInstrumentSampleThickness, thickness, "Sample thickness should match");
  }

  void do_assert_process(const H5::Group &process, bool hasSampleRuns, bool hasCanRuns, const std::string &userFile,
                         const std::string &sampleDirectRun, const std::string &canDirectRun, const bool &hasBgSub,
                         const std::string &scaledBgSubWorkspace, double scaledBgSubScaleFactor) {
    assertNumberOfAttributes(process, 2);

    assertStrAttribute(process, sasCanSASclass, sasProcessClassAttr);
    assertStrAttribute(process, sasNxclass, nxProcessClassAttr);

    // SVN data set
    assertDataSetDoesNotThrow(process, sasProcessTermSvn);

    assertStrDataSet(process, sasProcessName, sasProcessNameValue, "Should have the Mantid NXcanSAS process name");
    assertStrDataSet(process, sasProcessTermUserFile, userFile, "Should have correct user file name");

    if (hasSampleRuns) {
      assertStrDataSet(process, sasProcessTermSampleDirect, sampleDirectRun,
                       "Should have the corect sample direct run number");
    }

    if (hasCanRuns) {
      assertStrDataSet(process, sasProcessTermCanDirect, canDirectRun, "Should have the correct can direct run number");
    }

    if (hasBgSub) {
      assertStrDataSet(process, sasProcessTermScaledBgSubWorkspace, scaledBgSubWorkspace,
                       "Should have the correct scaled background subtraction workspace");
      assertNumArrDataSet(process, sasProcessTermScaledBgSubScaleFactor, scaledBgSubScaleFactor,
                          "Should have the correct scaled background subtraction scale factor");
    }
  }

  void do_assert_that_Q_dev_information_is_not_present(const H5::Group &data) {
    // Check that Q_uncertainty attribute is not saved
    assertMissingAttr(data, sasDataQUncertaintyAttr);
    assertMissingAttr(data, sasDataQUncertaintiesAttr);

    // Check that Qdev data set does not exist
    TS_ASSERT(!data.nameExists(sasDataQdev));

    auto qDataSet = data.openDataSet(sasDataQ);
    assertMissingAttr(qDataSet, sasUncertaintyAttr);
    assertMissingAttr(qDataSet, sasUncertaintiesAttr);
  }

  void do_assert_data(const H5::Group &data, int size, double value, double error, double xmin, double xmax,
                      double xerror, bool hasDx, int expectedDataEntries = 9) {
    auto expectedNumAttributes = hasDx ? expectedDataEntries : expectedDataEntries - 2;
    assertNumberOfAttributes(data, expectedNumAttributes);

    std::map<std::string, std::string> attrMap = {{sasCanSASclass, sasDataClassAttr},
                                                  {sasNxclass, nxDataClassAttr},
                                                  {sasDataIAxesAttr, sasDataQ},
                                                  {sasDataIUncertaintyAttr, sasDataIdev},
                                                  {sasDataIUncertaintiesAttr, sasDataIdev},
                                                  {sasSignal, sasDataI}};
    if (hasDx) {
      attrMap.insert({{sasDataQUncertaintyAttr, sasDataQdev}, {sasDataQUncertaintiesAttr, sasDataQdev}});
    }

    for (auto const &[attributeName, expectedValue] : attrMap) {
      assertStrAttribute(data, attributeName, expectedValue);
    }

    auto intensityDataSet = data.openDataSet(sasDataI);
    assertStrAttribute(intensityDataSet, sasUncertaintyAttr, sasDataIdev);
    assertStrAttribute(intensityDataSet, sasUncertaintiesAttr, sasDataIdev);

    // Q_indices attribute
    assertNumArrayAttributes(data, sasDataQIndicesAttr, std::vector<int>{0}, "Should be just 0");

    // I data set
    assertNumArrEntries(data.openDataSet(sasDataI), size, value);
    // I dev data set
    assertNumArrEntries(data.openDataSet(sasDataIdev), size, error);

    // Q data set
    auto const qDataSet = data.openDataSet(sasDataQ);
    auto increment = (xmax - xmin) / static_cast<double>(size - 1);
    assertNumArrEntries(qDataSet, size, xmin, increment);

    if (hasDx) {
      assertStrAttribute(qDataSet, sasUncertaintyAttr, sasDataQdev);
      assertStrAttribute(qDataSet, sasUncertaintiesAttr, sasDataQdev);
      // Q error data set
      assertNumArrEntries(data.openDataSet(sasDataQdev), size, xerror);
    } else {
      do_assert_that_Q_dev_information_is_not_present(data);
    }
  }

  void do_assert_polarized_data_1D(const H5::Group &data, int size, double value, double error, double xmin,
                                   double xmax, double xerror, bool hasDx) {
    do_assert_data(data, size, value, error, xmin, xmax, xerror, hasDx, 11);
  }

  void do_assert_2D_data(const H5::Group &data) {
    // Note: Testing attributes and data shape:
    // Actual Values are being tested in LoadNXcanSAS to avoid redundant testing
    assertNumberOfAttributes(data, 7);

    std::map<std::string, std::string> attrMap = {{sasCanSASclass, sasDataClassAttr},
                                                  {sasNxclass, nxDataClassAttr},
                                                  {sasDataIAxesAttr, sasDataQ + sasSeparator + sasDataQ},
                                                  {sasDataIUncertaintyAttr, sasDataIdev},
                                                  {sasDataIUncertaintiesAttr, sasDataIdev},
                                                  {sasSignal, sasDataI}};

    for (auto const &[attributeName, expectedValue] : attrMap) {
      assertStrAttribute(data, attributeName, expectedValue);
    }
    // Q_indices attribute
    assertNumArrayAttributes(data, sasDataQIndicesAttr, std::vector<int>{0, 1}, "Should be just 0,1");
  }

  void do_assert_transmission(const H5::Group &entry, NXcanSASTestTransmissionParameters &parameters) {
    if (!parameters.usesTransmission) {
      return;
    }
    // Map for attributes to assert with the expected values stored in NXcanSAS file.
    std::map<std::string, std::string> attrMap = {{sasCanSASclass, sasTransmissionSpectrumClassAttr},
                                                  {sasNxclass, nxTransmissionSpectrumClassAttr},
                                                  {sasTransmissionSpectrumNameAttr, parameters.name},
                                                  {sasTransmissionSpectrumTIndices, sasTransmissionSpectrumT},
                                                  {sasTransmissionSpectrumTUncertainty, sasTransmissionSpectrumTdev},
                                                  {sasTransmissionSpectrumTUncertainties, sasTransmissionSpectrumTdev},
                                                  {sasSignal, sasTransmissionSpectrumT}};

    auto transmission = entry.openGroup(sasTransmissionSpectrumGroupName + "_" + parameters.name);

    for (auto const &[attributeName, expectedValue] : attrMap) {
      assertStrAttribute(transmission, attributeName, expectedValue);
    }

    // Timestamp attribute
    std::string transmissionAttribute;
    TS_ASSERT_THROWS_NOTHING(
        H5Util::readStringAttribute(transmission, sasTransmissionSpectrumTimeStampAttr, transmissionAttribute));
    // T data set
    auto tDataSet = transmission.openDataSet(sasTransmissionSpectrumT);
    assertNumArrEntries(tDataSet, parameters.size, parameters.value);

    // Tdev data set
    auto tErrorDataSet = transmission.openDataSet(sasTransmissionSpectrumTdev);
    assertNumArrEntries(tErrorDataSet, parameters.size, parameters.error);

    // Lambda data set
    auto lambdaDataSet = transmission.openDataSet(sasTransmissionSpectrumLambda);
    double increment = (parameters.xmax - parameters.xmin) / static_cast<double>(parameters.size - 1);
    assertNumArrEntries(lambdaDataSet, parameters.size, parameters.xmin, increment);

    // Size check for matching T/Tdev/lambda
    do_assert_data_array_sizes_match(tDataSet, tErrorDataSet, lambdaDataSet);
  }

  void do_assert_data_array_sizes_match(const H5::DataSet &tDataSet, const H5::DataSet &tErrorDataSet,
                                        const H5::DataSet &lambdaDataSet) {
    auto arraySize = [](const H5::DataSet &dataSet) {
      const auto dataSpace = dataSet.getSpace();
      return dataSpace.getSelectNpoints();
    };

    TSM_ASSERT_EQUALS("Expected T and Tdev array lengths to match", arraySize(tDataSet), arraySize(tErrorDataSet));
    TSM_ASSERT_EQUALS("Expected T and Lambda array lengths to match", arraySize(lambdaDataSet), arraySize(tDataSet));
  }

  void do_assert(NXcanSASTestParameters &parameters,
                 NXcanSASTestTransmissionParameters transmissionParameters = NXcanSASTestTransmissionParameters(),
                 NXcanSASTestTransmissionParameters transmissionCanParameters = NXcanSASTestTransmissionParameters()) {
    auto filename = parameters.filename;
    H5::H5File file(filename, H5F_ACC_RDONLY, H5Util::defaultFileAcc());

    // Check sasentry
    auto entry = file.openGroup(sasEntryGroupName + sasEntryDefaultSuffix);
    do_assert_sasentry(entry, parameters.runNumber, parameters.workspaceTitle);

    // Check instrument
    auto instrument = entry.openGroup(sasInstrumentGroupName);
    do_assert_instrument(instrument, parameters.instrumentName, parameters.idf, parameters.radiationSource,
                         parameters.geometry, parameters.beamHeight, parameters.beamWidth, parameters.detectors,
                         parameters.invalidDetectors);

    // Check Sample
    auto sample = entry.openGroup(sasInstrumentSampleGroupAttr);
    do_assert_sample(sample, parameters.sampleThickness);

    // Check process
    auto process = entry.openGroup(sasProcessGroupName);
    do_assert_process(process, parameters.hasSampleRuns, parameters.hasCanRuns, parameters.userFile,
                      parameters.sampleDirectRun, parameters.canDirectRun, parameters.hasBgSub,
                      parameters.scaledBgSubWorkspace, parameters.scaledBgSubScaleFactor);

    // Check data
    auto data = entry.openGroup(sasDataGroupName);
    if (!parameters.isPolarized) {
      if (parameters.is2dData) {
        do_assert_2D_data(data);
      } else {
        do_assert_data(data, parameters.size, parameters.value, parameters.error, parameters.xmin, parameters.xmax,
                       parameters.xerror, parameters.hasDx, 9);
      }

      // Check the transmission
      do_assert_transmission(entry, transmissionParameters);
      // Check the transmission for the can
      do_assert_transmission(entry, transmissionCanParameters);

    } else {
      do_assert_polarized_data_1D(data, parameters.size, parameters.value, parameters.error, parameters.xmin,
                                  parameters.xmax, parameters.xerror, parameters.hasDx);
    }

    file.close();
  }

private:
  void assertNumberOfAttributes(const H5::H5Object &object, int expectedNumberAttributes) {
    auto numAttributes = object.getNumAttrs();
    TSM_ASSERT_EQUALS("Should have " + std::to_string(expectedNumberAttributes) + " attributes",
                      expectedNumberAttributes, numAttributes);
  }

  void assertStrAttribute(const H5::H5Object &object, const std::string &attributeName,
                          const std::string &expectedValue, const std::optional<std::string> &message = std::nullopt) {
    std::string nameAttribute;
    H5Util::readStringAttribute(object, attributeName, nameAttribute);
    TSM_ASSERT_EQUALS(message.value_or("Should be " + expectedValue), nameAttribute, expectedValue);
  }

  void assertStrDataSet(const H5::Group &group, const std::string &dataSetName, const std::string &expectedValue,
                        const std::optional<std::string> &message = std::nullopt) {
    auto dataSet = group.openDataSet(dataSetName);
    auto value = H5Util::readString(dataSet);
    TSM_ASSERT_EQUALS(message.value_or("Should be " + expectedValue), value, expectedValue);
  }

  template <typename numT>
  void assertNumArrDataSet(const H5::Group &group, const std::string &dataSetName, numT expectedValue,
                           const std::optional<std::string> &message = std::nullopt) {

    std::vector<numT> values;
    auto dataSet = group.openDataSet(dataSetName);
    H5Util::readArray1DCoerce<numT>(dataSet, values);
    TSM_ASSERT_EQUALS(message.value_or("Should be " + std::to_string(expectedValue)), values[0], expectedValue);
  }

  template <typename numT>
  void assertNumAttribute(const H5::H5Object &object, const std::string &attributeName, const numT &expectedValue,
                          const std::optional<std::string> &message = std::nullopt) {

    numT numAttribute;
    H5Util::readNumAttributeCoerce<numT>(object, attributeName, numAttribute);
    TSM_ASSERT_EQUALS(message.value_or("Should be " + std::to_string(expectedValue)), numAttribute, expectedValue);
  }

  template <typename numT>
  void assertNumArrEntries(const H5::DataSet &dataSet, int size, numT referenceValue, numT increment = 0) {
    std::vector<numT> data;
    H5Util::readArray1DCoerce<numT>(dataSet, data);
    TS_ASSERT_EQUALS(data.size(), static_cast<size_t>(size));
    if (!static_cast<bool>(increment)) {
      TS_ASSERT_DELTA(data[0], referenceValue, DELTA);
    } else {
      for (size_t index = 0; index < data.size(); ++index) {
        TS_ASSERT_DELTA(data[index], referenceValue, DELTA);
        referenceValue += increment;
      }
    }
  }

  void assertMissingAttr(const H5::H5Object &data, const std::string &attrName) {
    TSM_ASSERT("Should not have a " + attrName + " attribute", !data.attrExists(attrName));
  }

  void assertDataSetDoesNotThrow(const H5::Group &group, const std::string &dataSetName) {
    auto dataSet = group.openDataSet(dataSetName);
    TS_ASSERT_THROWS_NOTHING(H5Util::readString(dataSet));
  }

  template <typename numT>
  void assertNumArrayAttributes(const H5::H5Object &data, const std::string &attrName,
                                const std::vector<numT> &expectedValues,
                                const std::optional<std::string> &message = std::nullopt) {

    auto indexes = H5Util::readNumArrayAttributeCoerce<numT>(data, attrName);
    TSM_ASSERT_EQUALS(message.value_or("Attribute " + attrName + " has wrong indexes"), indexes, expectedValues);
  }
};
