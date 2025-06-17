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
#include "../inc/MantidDataHandling/SaveNXcanSASHelper.h"

#include "MantidKernel/VectorHelper.h"
#include "NXcanSASTestHelper.h"

#include <string>
#include <vector>

using namespace Mantid::DataHandling::NXcanSAS;
using namespace Mantid::NeXus;
using namespace NXcanSASTestHelper;
namespace {
constexpr double DELTA = 1e-6;
} // namespace

class NXcanSASFileTest {
protected:
  void do_assert_sasentry(const H5::Group &entry, const NXcanSASTestParameters &parameters) {
    assertNumberOfAttributes(entry, 3);

    assertStrAttribute(entry, sasCanSASclass, sasEntryClassAttr);
    assertStrAttribute(entry, sasNxclass, nxEntryClassAttr);
    assertStrAttribute(entry, sasEntryVersionAttr, sasEntryVersionAttrValue);

    // Definition data set
    assertStrDataSet(entry, sasEntryDefinition, sasEntryDefinitionFormat, "File definition should be NXcanSAS");
    // Run data set
    assertStrDataSet(entry, sasEntryRun, parameters.runNumber, "Run number should have been stored");
    // Title data set
    assertStrDataSet(entry, sasEntryTitle, parameters.workspaceTitle,
                     "The title should have been stored as the workspace name");
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
    for (const auto &detector : detectors) {
      const std::string detectorName = sasInstrumentDetectorGroupName + detector;
      const auto detectorNameSanitized = makeCanSASRelaxedName(detectorName);
      const auto detectorGroup = instrument.openGroup(detectorNameSanitized);

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
    const auto numObjects = instrument.getNumObjs();
    for (hsize_t index = 0; index < numObjects; ++index) {
      const auto objectType = instrument.getObjTypeByIdx(index);
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

  void do_assert_instrument(H5::Group &instrument, const NXcanSASTestParameters &parameters) {
    assertNumberOfAttributes(instrument, 2);

    assertStrAttribute(instrument, sasCanSASclass, sasInstrumentClassAttr);
    assertStrAttribute(instrument, sasNxclass, nxInstrumentClassAttr);

    assertStrDataSet(instrument, sasInstrumentName, parameters.instrumentName,
                     "Name of the instrument should have been stored");
    const auto idfName = parameters.idf == "POLSANSTEST" ? "unknown" : parameters.idf;
    assertStrDataSet(instrument, sasInstrumentIDF, idfName, "The idf should have been stored");

    // Check source
    const auto source = instrument.openGroup(sasInstrumentSourceGroupName);
    do_assert_source(source, parameters.radiationSource);

    // Check aperture
    auto aperture = instrument.openGroup(sasInstrumentApertureGroupName);
    do_assert_aperture(aperture, parameters.geometry, parameters.beamHeight, parameters.beamWidth);

    // Check detectors
    if (!parameters.invalidDetectors) {
      do_assert_detector(instrument, parameters.detectors);
    } else {
      // Make sure that no SASdetector group exists
      do_assert_no_detectors(instrument);
    }
  }

  void do_assert_sample(const H5::Group &sample, const double thickness) {
    assertNumberOfAttributes(sample, 2);
    assertNumArrDataSet(sample, sasInstrumentSampleThickness, thickness, "Sample thickness should match");
  }

  void do_assert_polarized_sample_metadata(const H5::Group &sample, const NXcanSASTestParameters &parameters) {
    if (!parameters.magneticFieldStrengthLogName.empty()) {
      assertNumArrDataSet(sample, sasSampleMagneticField, parameters.magneticFieldStrength,
                          "Magnetic Field log should have a value");
      assertStrAttribute(sample.openDataSet(sasSampleMagneticField), sasUnitAttr, parameters.magneticFieldUnit);
    }

    if (!parameters.magneticFieldDirection.empty()) {
      const auto dirVec =
          Mantid::Kernel::VectorHelper::splitStringIntoVector<double>(parameters.magneticFieldDirection);
      const std::vector<std::string> angles = {sasSampleEMFieldDirectionPolar, sasSampleEMFieldDirectionAzimuthal,
                                               sasSampleEMFieldDirectionRotation};

      for (auto i = 0; i < static_cast<int>(dirVec.size()); i++) {
        assertNumArrDataSet(sample, angles.at(i), dirVec.at(i));
        assertStrAttribute(sample.openDataSet(angles.at(i)), sasUnitAttr, sasSampleEMFieldDirectionUnitsAttr);
      }
    }
  }

  void do_assert_process(const H5::Group &process, const NXcanSASTestParameters &parameters) {
    assertNumberOfAttributes(process, 2);

    assertStrAttribute(process, sasCanSASclass, sasProcessClassAttr);
    assertStrAttribute(process, sasNxclass, nxProcessClassAttr);

    // SVN data set
    assertDataSetDoesNotThrow(process, sasProcessTermSvn);

    assertStrDataSet(process, sasProcessName, sasProcessNameValue, "Should have the Mantid NXcanSAS process name");
    assertStrDataSet(process, sasProcessTermUserFile, parameters.userFile, "Should have correct user file name");

    if (parameters.hasSampleRuns) {
      assertStrDataSet(process, sasProcessTermSampleDirect, parameters.sampleDirectRun,
                       "Should have the correct sample direct run number");
    }

    if (parameters.hasCanRuns) {
      assertStrDataSet(process, sasProcessTermCanDirect, parameters.canDirectRun,
                       "Should have the correct can direct run number");
    }

    if (parameters.hasBgSub) {
      assertStrDataSet(process, sasProcessTermScaledBgSubWorkspace, parameters.scaledBgSubWorkspace,
                       "Should have the correct scaled background subtraction workspace");
      assertNumArrDataSet(process, sasProcessTermScaledBgSubScaleFactor, parameters.scaledBgSubScaleFactor,
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

  void do_assert_data(const H5::Group &data, const NXcanSASTestParameters &parameters) {

    const auto expectedNumAttributes = parameters.hasDx ? 9 : 7;
    assertNumberOfAttributes(data, expectedNumAttributes);

    std::map<std::string, std::string> attrMap = {{sasCanSASclass, sasDataClassAttr},
                                                  {sasNxclass, nxDataClassAttr},
                                                  {sasDataIAxesAttr, sasDataQ},
                                                  {sasDataIUncertaintyAttr, sasDataIdev},
                                                  {sasDataIUncertaintiesAttr, sasDataIdev},
                                                  {sasSignal, sasDataI}};
    if (parameters.hasDx) {
      attrMap.insert({{sasDataQUncertaintyAttr, sasDataQdev}, {sasDataQUncertaintiesAttr, sasDataQdev}});
    }

    for (auto const &[attributeName, expectedValue] : attrMap) {
      assertStrAttribute(data, attributeName, expectedValue);
    }

    const auto intensityDataSet = data.openDataSet(sasDataI);
    assertStrAttribute(intensityDataSet, sasUncertaintyAttr, sasDataIdev);
    assertStrAttribute(intensityDataSet, sasUncertaintiesAttr, sasDataIdev);

    // Q_indices attribute
    assertNumArrayAttributes(data, sasDataQIndicesAttr, std::vector<int>{0}, "Should be just 0");

    // I data set
    assertNumArrEntries(data.openDataSet(sasDataI), parameters.size, parameters.value);
    // I dev data set
    assertNumArrEntries(data.openDataSet(sasDataIdev), parameters.size, parameters.error);

    // Q data set
    const auto qDataSet = data.openDataSet(sasDataQ);
    const auto increment = (parameters.xmax - parameters.xmin) / static_cast<double>(parameters.size - 1);
    assertNumArrEntries(qDataSet, parameters.size, parameters.xmin, increment);

    if (parameters.hasDx) {
      assertStrAttribute(qDataSet, sasUncertaintyAttr, sasDataQdev);
      assertStrAttribute(qDataSet, sasUncertaintiesAttr, sasDataQdev);
      // Q error data set
      assertNumArrEntries(data.openDataSet(sasDataQdev), parameters.size, parameters.xerror);
    } else {
      do_assert_that_Q_dev_information_is_not_present(data);
    }
  }
  void do_assert_polarized_data_2D(const H5::Group &data, const NXcanSASTestParameters &parameters) {
    assertNumberOfAttributes(data, 9);
    auto IaxesAttr = sasDataPin + sasSeparator + sasDataPout + sasSeparator + sasDataQ + sasSeparator + sasDataQ;
    const std::map<std::string, std::string> attrMap = {{sasCanSASclass, sasDataClassAttr},
                                                        {sasNxclass, nxDataClassAttr},
                                                        {sasDataIAxesAttr, IaxesAttr},
                                                        {sasDataIUncertaintyAttr, sasDataIdev},
                                                        {sasDataIUncertaintiesAttr, sasDataIdev},
                                                        {sasSignal, sasDataI}};

    for (auto const &[attributeName, expectedValue] : attrMap) {
      assertStrAttribute(data, attributeName, expectedValue);
    }

    const auto intensityDataSet = data.openDataSet(sasDataI);

    assertStrAttribute(intensityDataSet, sasUncertaintyAttr, sasDataIdev);
    assertStrAttribute(intensityDataSet, sasUncertaintiesAttr, sasDataIdev);

    // Q_indices attribute
    assertNumArrayAttributes(data, sasDataQIndicesAttr, std::vector<int>{0, 1, 2, 3}, "Should be just 0, 1, 2, 3");

    auto offset = static_cast<int>((parameters.ymax - parameters.ymin) * (parameters.xmax - parameters.xmin));
    auto size = offset * parameters.polWorkspaceNumber;
    auto reference = parameters.referenceValues.empty() ? std::vector<double>(parameters.polWorkspaceNumber, 0)
                                                        : parameters.referenceValues;

    // I data set
    assertMDNumArrEntries(intensityDataSet, size, reference, offset);
    // I dev data set
    // error is saved as sqrt of test value
    std::transform(reference.begin(), reference.end(), reference.begin(),
                   [](auto const &value) { return std::sqrt(value); });
    assertMDNumArrEntries(data.openDataSet(sasDataIdev), size, reference, offset);

    // Q data set

    // qx
    auto qDataSet = data.openDataSet(sasDataQ + "x");
    size = offset;
    offset = static_cast<int>(parameters.xmax - parameters.xmin);

    reference = std::vector<double>(offset, 1.5);
    assertMDNumArrEntries(qDataSet, size, reference, offset);

    // qy
    // indices are swaped storing qy
    qDataSet = data.openDataSet(sasDataQ + "y");
    std::generate(reference.begin(), reference.end(), [i = 1]() mutable { return i++; });
    assertMDNumArrEntries(qDataSet, size, reference, offset);
  }

  void do_assert_polarized_data_1D(const H5::Group &data, const NXcanSASTestParameters &parameters) {

    const auto expectedNumAttributes = parameters.hasDx ? 11 : 9;
    assertNumberOfAttributes(data, expectedNumAttributes);

    std::map<std::string, std::string> attrMap = {
        {sasCanSASclass, sasDataClassAttr},
        {sasNxclass, nxDataClassAttr},
        {sasDataIAxesAttr, sasDataPin + sasSeparator + sasDataPout + sasSeparator + sasDataQ},
        {sasDataIUncertaintyAttr, sasDataIdev},
        {sasDataIUncertaintiesAttr, sasDataIdev},
        {sasSignal, sasDataI}};
    if (parameters.hasDx) {
      attrMap.insert({{sasDataQUncertaintyAttr, sasDataQdev}, {sasDataQUncertaintiesAttr, sasDataQdev}});
    }

    for (auto const &[attributeName, expectedValue] : attrMap) {
      assertStrAttribute(data, attributeName, expectedValue);
    }

    const auto intensityDataSet = data.openDataSet(sasDataI);

    assertStrAttribute(intensityDataSet, sasUncertaintyAttr, sasDataIdev);
    assertStrAttribute(intensityDataSet, sasUncertaintiesAttr, sasDataIdev);

    // Q_indices attribute
    assertNumArrayAttributes(data, sasDataQIndicesAttr, std::vector<int>{0, 1, 2}, "Should be just 0, 1, 2");

    // I data set
    assertNumArrEntries(data.openDataSet(sasDataI), parameters.polWorkspaceNumber * parameters.size, parameters.value);
    // I dev data set
    assertNumArrEntries(data.openDataSet(sasDataIdev), parameters.polWorkspaceNumber * parameters.size,
                        parameters.error);

    // Q data set
    auto const qDataSet = data.openDataSet(sasDataQ);
    auto increment = (parameters.xmax - parameters.xmin) / static_cast<double>(parameters.size - 1);
    assertNumArrEntries(qDataSet, parameters.size, parameters.xmin, increment);

    if (parameters.hasDx) {
      assertStrAttribute(qDataSet, sasUncertaintyAttr, sasDataQdev);
      assertStrAttribute(qDataSet, sasUncertaintiesAttr, sasDataQdev);
      // Q error data set
      assertNumArrEntries(data.openDataSet(sasDataQdev), parameters.size, parameters.xerror);
    } else {
      do_assert_that_Q_dev_information_is_not_present(data);
    }
  }

  void do_assert_2D_data(const H5::Group &data) {
    // Note: Testing attributes and data shape:
    // Actual Values are being tested in LoadNXcanSAS to avoid redundant testing
    assertNumberOfAttributes(data, 7);

    const std::map<std::string, std::string> attrMap = {{sasCanSASclass, sasDataClassAttr},
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

  void do_assert_polarized_components(const H5::Group &group, const NXcanSASTestParameters &parameters) {
    auto components =
        std::vector({parameters.polarizerComponent, parameters.flipperComponent, parameters.analyzerComponent});

    for (auto const &component : components) {
      auto compVec = Mantid::Kernel::VectorHelper::splitStringIntoVector<std::string>(component.compName);
      assertPolarizedComponent(group, compVec, component.compType, component.distanceToSample, component.valueType);
    }
  }

  void do_assert_transmission(const H5::Group &entry, const TransmissionTestParameters &parameters) {
    // Map for attributes to assert with the expected values stored in NXcanSAS file.
    const std::map<std::string, std::string> attrMap = {
        {sasCanSASclass, sasTransmissionSpectrumClassAttr},
        {sasNxclass, nxTransmissionSpectrumClassAttr},
        {sasTransmissionSpectrumNameAttr, parameters.name},
        {sasTransmissionSpectrumTIndices, sasTransmissionSpectrumT},
        {sasTransmissionSpectrumTUncertainty, sasTransmissionSpectrumTdev},
        {sasTransmissionSpectrumTUncertainties, sasTransmissionSpectrumTdev},
        {sasSignal, sasTransmissionSpectrumT}};

    const auto transmission = entry.openGroup(sasTransmissionSpectrumGroupName + "_" + parameters.name);

    for (const auto &[attributeName, expectedValue] : attrMap) {
      assertStrAttribute(transmission, attributeName, expectedValue);
    }

    // Timestamp attribute
    std::string transmissionAttribute;
    TS_ASSERT_THROWS_NOTHING(
        H5Util::readStringAttribute(transmission, sasTransmissionSpectrumTimeStampAttr, transmissionAttribute));
    // T data set
    const auto tDataSet = transmission.openDataSet(sasTransmissionSpectrumT);
    assertNumArrEntries(tDataSet, parameters.size, parameters.value);

    // Tdev data set
    const auto tErrorDataSet = transmission.openDataSet(sasTransmissionSpectrumTdev);
    assertNumArrEntries(tErrorDataSet, parameters.size, parameters.error);

    // Lambda data set
    const auto lambdaDataSet = transmission.openDataSet(sasTransmissionSpectrumLambda);
    const double increment = (parameters.xmax - parameters.xmin) / static_cast<double>(parameters.size - 1);
    assertNumArrEntries(lambdaDataSet, parameters.size, parameters.xmin, increment);

    // Size check for matching T/Tdev/lambda
    assertDataSpacesMatch(std::make_pair(tDataSet, tErrorDataSet),
                          std::make_pair("Transmission signal", "Transmission error"));
    assertDataSpacesMatch(std::make_pair(tDataSet, lambdaDataSet), std::make_pair("Transmission signal", "Wavelength"));
  }

  void do_assert(const NXcanSASTestParameters &parameters) {
    const auto filename = parameters.filename;
    H5::H5File file(filename, H5F_ACC_RDONLY, H5Util::defaultFileAcc());

    // Check sasentry
    const auto entry = file.openGroup(sasEntryGroupName + sasEntryDefaultSuffix);
    do_assert_sasentry(entry, parameters);

    // Check instrument
    auto instrument = entry.openGroup(sasInstrumentGroupName);
    do_assert_instrument(instrument, parameters);

    // Check Sample
    const auto sample = entry.openGroup(sasInstrumentSampleGroupAttr);
    do_assert_sample(sample, parameters.sampleThickness);

    // Check process
    const auto process = entry.openGroup(sasProcessGroupName);
    do_assert_process(process, parameters);

    // Check the transmission for sample and can if necessary
    if (parameters.transmissionParameters.usesTransmission) {
      do_assert_transmission(entry, parameters.transmissionParameters);
    }
    if (parameters.transmissionCanParameters.usesTransmission) {
      do_assert_transmission(entry, parameters.transmissionCanParameters);
    }

    // Check data
    const auto data = entry.openGroup(sasDataGroupName);
    if (!parameters.isPolarized) {
      parameters.is2dData ? do_assert_2D_data(data) : do_assert_data(data, parameters);

    } else {
      parameters.is2dData ? do_assert_polarized_data_2D(data, parameters)
                          : do_assert_polarized_data_1D(data, parameters);
      // Check polarized metadata
      do_assert_polarized_components(instrument, parameters);
      do_assert_polarized_sample_metadata(sample, parameters);
    }
    file.close();
  }

private:
  void assertNumberOfAttributes(const H5::H5Object &object, const int expectedNumberAttributes) {
    const auto numAttributes = object.getNumAttrs();
    TSM_ASSERT_EQUALS("Should have " + std::to_string(expectedNumberAttributes) + " attributes, but " +
                          std::to_string(numAttributes) + " were found",
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
    const auto dataSet = group.openDataSet(dataSetName);
    const auto value = H5Util::readString(dataSet);
    TSM_ASSERT_EQUALS(message.value_or("Should be " + expectedValue), value, expectedValue);
  }

  template <typename numT>
  void assertNumArrDataSet(const H5::Group &group, const std::string &dataSetName, numT expectedValue,
                           const std::optional<std::string> &message = std::nullopt) {

    std::vector<numT> values;
    const auto dataSet = group.openDataSet(dataSetName);
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
  void assertNumArrEntries(const H5::DataSet &dataSet, const int size, numT referenceValue, numT increment = 0) {
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

  void assertMDNumArrEntries(const H5::DataSet &dataSet, const int size, const std::vector<double> &reference,
                             const int offset) {
    // Dump the MD dimensional array into a 1D array. The assertion tries to match a reference value every offset
    // to a point of the data vector. Used for testing multidimensional arrays, for example, with group workspaces.
    std::vector<double> data;
    H5Util::readArray1DCoerce<double>(dataSet, data);
    TS_ASSERT_EQUALS(data.size(), static_cast<size_t>(size));
    for (auto i = 0; i < static_cast<int>(reference.size()); i++) {
      const auto index = i * offset;
      TS_ASSERT_DELTA(data[index], reference[i], DELTA);
    }
  }

  void assertMissingAttr(const H5::H5Object &data, const std::string &attrName) {
    TSM_ASSERT("Should not have a " + attrName + " attribute", !data.attrExists(attrName));
  }

  void assertDataSetDoesNotThrow(const H5::Group &group, const std::string &dataSetName) {
    const auto dataSet = group.openDataSet(dataSetName);
    TS_ASSERT_THROWS_NOTHING(H5Util::readString(dataSet));
  }

  template <typename numT>
  void assertNumArrayAttributes(const H5::H5Object &data, const std::string &attrName,
                                const std::vector<numT> &expectedValues,
                                const std::optional<std::string> &message = std::nullopt) {

    auto indexes = H5Util::readNumArrayAttributeCoerce<numT>(data, attrName);
    TSM_ASSERT_EQUALS(message.value_or("Attribute " + attrName + " has wrong indexes"), indexes, expectedValues);
  }

  void assertDataSpacesMatch(const std::pair<H5::DataSet, H5::DataSet> &dataSets,
                             const std::pair<std::string, std::string> &names) {
    auto arraySize = [](const H5::DataSet &dataSet) {
      const auto dataSpace = dataSet.getSpace();
      return dataSpace.getSelectNpoints();
    };
    TSM_ASSERT_EQUALS("Expected " + names.first + " and " + names.second + " array lengths to match",
                      arraySize(dataSets.first), arraySize(dataSets.second));
  }

  void assertPolarizedComponent(const H5::Group &group, const std::vector<std::string> &components,
                                const std::string &componentType, const double expectedCompDistance,
                                const std::string &expectedValueType) {
    for (size_t i = 0; i < components.size(); i++) {
      const auto &compName = components.at(i);
      auto groupName = "sas" + componentType;
      groupName.append(components.size() > 1 ? addDigit(i + 1) : "");

      // Assert group exists or not if we are testing wrong group
      TS_ASSERT(compName == "wrong" ? !group.nameExists(groupName) : group.nameExists(groupName));
      if (group.nameExists(groupName)) {
        auto componentGroup = group.openGroup(groupName);
        auto NXClassValue = (componentType == "analyzer") ? "NXpolarizer" : "NX" + componentType;

        assertStrAttribute(componentGroup, sasNxclass, NXClassValue);
        assertStrAttribute(componentGroup, sasCanSASclass, "SAS" + componentType);
        assertStrAttribute(componentGroup.openDataSet(InstrumentPolarizer::sasPolarizerDistance()), sasUnitAttr,
                           InstrumentPolarizer::sasPolarizerDistanceUnitAttr());

        assertNumArrDataSet(componentGroup, InstrumentPolarizer::sasPolarizerDistance(), expectedCompDistance);
        assertStrDataSet(componentGroup, InstrumentPolarizer::sasPolarizerName(), compName);
        assertStrDataSet(componentGroup, InstrumentPolarizer::sasPolarizerDeviceType(), expectedValueType);
      }
    }
  }
};
