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

namespace
{
const std::string nxclass = "NX_class";
const std::string suffix = "01";

std::string concatenateStringVector(std::vector<std::string> stringVector)
{
    std::ostringstream os;
    for (auto &element : stringVector) {
        os << element;
        os << Mantid::DataHandling::NXcanSAS::sasSeparator;
    }

    return os.str();
}

std::string getIDFfromWorkspace(Mantid::API::MatrixWorkspace_sptr workspace)
{
    auto instrument = workspace->getInstrument();
    auto name = instrument->getFullName();
    auto date = workspace->getWorkspaceStartDate();
    return workspace->getInstrumentFilename(name, date);
}

void setXValuesOn1DWorkspaceWithPointData(
    Mantid::API::MatrixWorkspace_sptr workspace, double xmin, double xmax)
{
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

class SaveNXcanSASTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created
    // statically
    // This means the constructor isn't called when running other tests
    static SaveNXcanSASTest *createSuite() { return new SaveNXcanSASTest(); }
    static void destroySuite(SaveNXcanSASTest *suite) { delete suite; }

    void test_that_workspace_without_momentum_transfer_units_is_invalid()
    {
        // Arrange
        auto ws = WorkspaceCreationHelper::Create1DWorkspaceConstantWithXerror(
            10 /*size*/, 1.23 /*value&*/, 2.3 /*error*/, 23.4 /*xerror*/);
        const std::string filename = "SaveNXcanSASTestFile.h5";

        // Act + Assert
        auto saveAlg
            = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
                "SaveNXcanSAS");
        saveAlg->initialize();
        saveAlg->setProperty("Filename", filename);
        TSM_ASSERT_THROWS_ANYTHING(
            "Should not save file without momentum transfer units.",
            saveAlg->setProperty("InputWorkspace", ws));
    }

    void test_that_histogram_data_cannot_be_saved()
    {
        // Arrange
        auto ws = WorkspaceCreationHelper::Create2DWorkspaceBinned(
            1 /*nhist*/, 10 /*nbins*/, 1.0 /*xmin*/, 1.0 /*increment*/);
        ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create(
            "MomentumTransfer");
        const std::string filename = "SaveNXcanSASTestFile.h5";
        // Act + Assert
        auto saveAlg
            = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
                "SaveNXcanSAS");
        saveAlg->initialize();
        saveAlg->setProperty("Filename", filename);
        saveAlg->setProperty("InputWorkspace", ws);
        TSM_ASSERT_THROWS_ANYTHING(
            "Should not run since we are providing a histogram.",
            saveAlg->execute());
    }

    void test_that_1D_workspace_without_transmissions_is_saved_correctly()
    {
        // Arrange
        const std::string filename = "SaveNXcanSASTestFile.h5";
        removeFile(filename);
        const int size = 100;
        const double value = 10.23;
        const double error = 3.45;
        const double xerror = 2.3759 / 3.6;
        const double xmin = 1.0;
        const double xmax = 100.0;
        const std::string runNumber = "1234";
        const std::string userFile("sdkfsdlfkjsdlfksjdlfksdjf");
        const std::string workspaceTitle("sample_worksapce");
        const std::string instrumentName = "SANS2D";
        const std::string radiationSource("Spallation Neutron Source");
        const std::vector<std::string> detectors
            = {"front-detector", "rear-detector"};
        auto ws = provide1DWorkspaceWithXError(size, value, error, xerror,
                                               runNumber, userFile,
                                               workspaceTitle, instrumentName);

        setXValuesOn1DWorkspaceWithPointData(ws, xmin, xmax);

        auto idf = getIDFfromWorkspace(ws);

        // Act
        save_file_no_issues(ws, filename, radiationSource, detectors);

        // Assert
        do_assert_that_entries_exist(size, value, error, xmin, xmax, xerror,
                                     filename, runNumber, workspaceTitle,
                                     instrumentName, idf, userFile,
                                     radiationSource, detectors);

        // Clean up
        removeFile(filename);
    }

    void test_that_unknown_detector_names_are_not_saved() {
      // Arrange
      const std::string filename = "SaveNXcanSASTestFile.h5";
      removeFile(filename);
      const int size = 100;
      const double value = 10.23;
      const double error = 3.45;
      const double xerror = 2.3759 / 3.6;
      const std::string runNumber = "1234";
      const std::string userFile("sdkfsdlfkjsdlfksjdlfksdjf");
      const std::string workspaceTitle("sample_worksapce");
      const std::string instrumentName = "SANS2D";
      const std::string radiationSource("Spallation Neutron Source");
      const std::vector<std::string> detectors
          = {"wrong-detector1", "wrong-detector2"};
      auto ws = provide1DWorkspaceWithXError(size, value, error, xerror,
                                             runNumber, userFile,
                                             workspaceTitle, instrumentName);

      // Act
      save_file_no_issues(ws, filename, radiationSource, detectors);

      // Assert


    }

    void
    test_that_1D_workspace_without_transmissions_and_without_xerror_is_saved_correctly()
    {
    }

    void tset_that_1D_workspace_with_transmissions_is_saved_correctly() {}

private:
    void removeFile(const std::string &filename)
    {
        if (Poco::File(filename).exists())
            Poco::File(filename).remove();
    }

    void add_sample_log(Mantid::API::MatrixWorkspace_sptr workspace,
                        const std::string &logName, const std::string &logValue)
    {
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
                  const std::string &runNumber, const std::string &userFile)
    {
        if (!runNumber.empty()) {
            add_sample_log(workspace, "run_number", runNumber);
        }

        if (!userFile.empty()) {
            add_sample_log(workspace, "UserFile", userFile);
        }
    }

    void set_instrument(Mantid::API::MatrixWorkspace_sptr workspace,
                        const std::string &instrumentName)
    {
        auto instAlg
            = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
                "LoadInstrument");
        instAlg->initialize();
        instAlg->setChild(true);
        instAlg->setProperty("Workspace", workspace);
        instAlg->setProperty("InstrumentName", instrumentName);
        instAlg->setProperty("RewriteSpectraMap", "False");
        instAlg->execute();
    }

    Mantid::API::MatrixWorkspace_sptr provide1DWorkspaceWithXError(
        int size, double value, double error, double xerror,
        const std::string &runNumber, const std::string &userFile,
        const std::string &workspaceTitle, const std::string &instrumentName)
    {
        // Create a sample 1D workspace
        auto ws = WorkspaceCreationHelper::Create1DWorkspaceConstantWithXerror(
            size, value, error, xerror);
        ws->setTitle(workspaceTitle);
        ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create(
            "MomentumTransfer");

        // Add sample logs
        set_logs(ws, runNumber, userFile);

        // Set instrument
        set_instrument(ws, instrumentName);

        return ws;
    }

    void save_file_no_issues(Mantid::API::MatrixWorkspace_sptr workspace,
                             const std::string &filename,
                             const std::string &radiationSource,
                             const std::vector<std::string> &detectors
                             = std::vector<std::string>())
    {
        auto saveAlg
            = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
                "SaveNXcanSAS");
        saveAlg->initialize();
        saveAlg->setProperty("Filename", filename);
        saveAlg->setProperty("InputWorkspace", workspace);
        saveAlg->setProperty("RadiationSource", radiationSource);
        if (!detectors.empty()) {
            std::string detectorsAsString = concatenateStringVector(detectors);
            saveAlg->setProperty("DetectorNames", detectorsAsString);
        }
        TSM_ASSERT_THROWS_NOTHING("Should not throw anything",
                                  saveAlg->execute());
        TSM_ASSERT("Should have executed", saveAlg->isExecuted());
    }

    void do_assert_sasentry(H5::Group &entry, const std::string &run,
                            const std::string &title)
    {
        using namespace Mantid::DataHandling::NXcanSAS;
        auto numAttributes = entry.getNumAttrs();
        TSM_ASSERT_EQUALS("Should have three attributes", 2, numAttributes);

        // NX_class attribute
        auto classAttribute
            = Mantid::DataHandling::H5Util::readAttributeAsString(entry,
                                                                  nxclass);
        TSM_ASSERT_EQUALS("Should be SASentry class", classAttribute,
                          sasEntryClassAttr);

        // Version attribute
        auto versionAttribute
            = Mantid::DataHandling::H5Util::readAttributeAsString(
                entry, sasEntryVersionAttr);
        TSM_ASSERT_EQUALS("Version should be 1.0", versionAttribute,
                          sasEntryVersionAttrValue);

        // Definition data set
        auto definitionDataSet = entry.openDataSet(sasEntryDefinition);
        auto definitionValue
            = Mantid::DataHandling::H5Util::readString(definitionDataSet);
        TSM_ASSERT_EQUALS("File definition should be NXcanSAS", definitionValue,
                          sasEntryDefinitionFormat);

        // Run data set
        auto runDataSet = entry.openDataSet(sasEntryRun);
        auto runValue = Mantid::DataHandling::H5Util::readString(runDataSet);
        TSM_ASSERT_EQUALS("Run number should have been stored.", runValue, run);

        // Title data set
        auto titleDataSet = entry.openDataSet(sasEntryTitle);
        auto titleValue
            = Mantid::DataHandling::H5Util::readString(titleDataSet);
        TSM_ASSERT_EQUALS(
            "The title should have been stored as the workspace name.",
            titleValue, title);
    }

    void do_assert_source(H5::Group &source, const std::string &radiationSource)
    {

        auto numAttributes = source.getNumAttrs();
        TSM_ASSERT_EQUALS("Should have 1 attribute", 1, numAttributes);

        // NX_class attribute
        auto classAttribute
            = Mantid::DataHandling::H5Util::readAttributeAsString(source,
                                                                  nxclass);
        TSM_ASSERT_EQUALS("Should be SASsource class", classAttribute,
                          sasInstrumentSourceClassAttr);

        // Radiation data set
        auto radiationDataSet
            = source.openDataSet(sasInstrumentSourceRadiation);
        auto radiationValue
            = Mantid::DataHandling::H5Util::readString(radiationDataSet);
        TSM_ASSERT_EQUALS("Radiation sources should match.", radiationValue,
                          radiationSource);
    }

    void do_assert_detector(H5::Group &instrument,
                            std::vector<std::string> detectors)
    {
        for (auto &detector : detectors) {
            auto detectorGroup = instrument.openGroup(
                sasInstrumentDetectorGroupName + suffix + detector);

            auto numAttributes = detectorGroup.getNumAttrs();
            TSM_ASSERT_EQUALS("Should have 1 attribute", 1, numAttributes);

            // NX_class attribute
            auto classAttribute
                = Mantid::DataHandling::H5Util::readAttributeAsString(
                    detectorGroup, nxclass);
            TSM_ASSERT_EQUALS("Should be SASdetector class", classAttribute,
                              sasInstrumentDetectorClassAttr);

            // Detector name data set
            auto name = detectorGroup.openDataSet(sasInstrumentDetectorName);
            auto nameValue = Mantid::DataHandling::H5Util::readString(name);
            TSM_ASSERT_EQUALS("Radiation sources should match.", nameValue,
                              detector);

            // SDD  data set
            auto sdd = detectorGroup.openDataSet(sasInstrumentDetectorSdd);
            TS_ASSERT_THROWS_NOTHING(
                Mantid::DataHandling::H5Util::readString(sdd));
        }
    }

    void do_assert_instrument(H5::Group &instrument,
                              const std::string &instrumentName,
                              const std::string &idf,
                              const std::string &radiationSource,
                              const std::vector<std::string> &detectors,
                              bool invalidDetectors)
    {
        auto numAttributes = instrument.getNumAttrs();
        TSM_ASSERT_EQUALS("Should have 1 attribute", 1, numAttributes);

        // NX_class attribute
        auto classAttribute
            = Mantid::DataHandling::H5Util::readAttributeAsString(instrument,
                                                                  nxclass);
        TSM_ASSERT_EQUALS("Should be SASentry class", classAttribute,
                          sasInstrumentClassAttr);

        // Name data set
        auto instrumentNameDataSet = instrument.openDataSet(sasInstrumentName);
        auto instrumentNameValue
            = Mantid::DataHandling::H5Util::readString(instrumentNameDataSet);
        TSM_ASSERT_EQUALS("Name of the instrument should have been stored",
                          instrumentNameValue, instrumentName);

        // IDF data set
        auto idfDataSet = instrument.openDataSet(sasInstrumentIDF);
        auto idfValue = Mantid::DataHandling::H5Util::readString(idfDataSet);
        TSM_ASSERT_EQUALS("The idf should have been stored", idfValue, idf);

        // Check source
        auto source
            = instrument.openGroup(sasInstrumentSourceGroupName + suffix);
        do_assert_source(source, radiationSource);

        // Check detectors
        if (!invalidDetectors) {
            do_assert_detector(instrument, detectors);
        } else {
          // Make sure that no SASdetector group exists
          do_assert_no_detectors(instrument);
        }
    }

    void do_assert_process(H5::Group &process, const std::string &userFile)
    {
        auto numAttributes = process.getNumAttrs();
        TSM_ASSERT_EQUALS("Should have 1 attribute", 1, numAttributes);

        // NX_class attribute
        auto classAttribute
            = Mantid::DataHandling::H5Util::readAttributeAsString(process,
                                                                  nxclass);
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
        TSM_ASSERT_EQUALS("Should have the Mantid NXcanSAS process name",
                          nameValue, sasProcessNameValue);

        // User file
        auto userFileDataSet = process.openDataSet(sasProcessTermUserFile);
        auto userFileValue
            = Mantid::DataHandling::H5Util::readString(userFileDataSet);
        TSM_ASSERT_EQUALS("Should have the Mantid NXcanSAS process name",
                          userFileValue, userFile);
    }

    void do_assert_1D_vector_with_same_entries(H5::DataSet &dataSet,
                                               double referenceValue, int size)
    {
        auto data
            = Mantid::DataHandling::H5Util::readArray1DCoerce<double>(dataSet);
        TS_ASSERT_EQUALS(data.size(), static_cast<size_t>(size));
        TS_ASSERT_EQUALS(data[0], referenceValue);
    }

    void do_assert_1D_vector_with_increasing_entries(H5::DataSet &dataSet,
                                                     double min,
                                                     double increment, int size)
    {
        auto data
            = Mantid::DataHandling::H5Util::readArray1DCoerce<double>(dataSet);
        TS_ASSERT_EQUALS(data.size(), static_cast<size_t>(size));
        for (size_t index = 0; index < data.size(); ++index) {
            TS_ASSERT_EQUALS(data[index], min);
            min += increment;
        }
    }

    void do_assert_data(H5::Group &data, int size, double value, double error,
                        double xmin, double xmax, double xerror)
    {

        auto numAttributes = data.getNumAttrs();
        TSM_ASSERT_EQUALS("Should have 6 attribute", 6, numAttributes);

        // NX_class attribute
        auto classAttribute
            = Mantid::DataHandling::H5Util::readAttributeAsString(data,
                                                                  nxclass);
        TSM_ASSERT_EQUALS("Should be SASdata class", classAttribute,
                          sasDataClassAttr);

        // I_axes attribute
        auto intensityAttribute
            = Mantid::DataHandling::H5Util::readAttributeAsString(
                data, sasDataIAxesAttr);
        TSM_ASSERT_EQUALS("Should be just Q", intensityAttribute, sasDataQ);

        // I_uncertainty attribute
        auto errorAttribute
            = Mantid::DataHandling::H5Util::readAttributeAsString(
                data, sasDataIUncertaintyAttr);
        TSM_ASSERT_EQUALS("Should be just Idev", errorAttribute, sasDataIdev);

        // Q_indices attribute
        auto qAttribute = Mantid::DataHandling::H5Util::readAttributeAsString(
            data, sasDataQIndicesAttr);
        TSM_ASSERT_EQUALS("Should be just 0", qAttribute, "0");

        // Q_uncertainty attribute

        // Signal attribute
        auto signalAttribute
            = Mantid::DataHandling::H5Util::readAttributeAsString(data,
                                                                  sasSignal);
        TSM_ASSERT_EQUALS("Should be just 0", signalAttribute, sasDataI);

        // I data set
        auto intensityDataSet = data.openDataSet(sasDataI);
        do_assert_1D_vector_with_same_entries(intensityDataSet, value, size);

        // I data set uncertainty attribute
        auto uncertaintyIAttribute
            = Mantid::DataHandling::H5Util::readAttributeAsString(
                intensityDataSet, sasUncertaintyAttr);
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

        // Q data set uncertainty attribute
        auto uncertaintyQAttribute
            = Mantid::DataHandling::H5Util::readAttributeAsString(
                qDataSet, sasUncertaintyAttr);
        TSM_ASSERT_EQUALS("Should be just Qdev", uncertaintyQAttribute,
                          sasDataQdev);

        // Q error data set
        auto xErrorDataSet = data.openDataSet(sasDataQdev);
        do_assert_1D_vector_with_same_entries(xErrorDataSet, xerror, size);
    }

    void do_assert_that_entries_exist(
        int size, double value, double error, double xmin, double xmax,
        double xerror, const std::string &filename, const std::string &run,
        const std::string &title, const std::string &instrumentName,
        const std::string &idf, const std::string &userFile,
        const std::string &radiationSource,
        const std::vector<std::string> &detectors,
        bool invalidDetectors = false)
    {
        H5::H5File file(filename, H5F_ACC_RDONLY);

        // Check sasentry
        auto entry = file.openGroup(sasEntryGroupName + suffix);
        do_assert_sasentry(entry, run, title);

        // Check instrument
        auto instrument = entry.openGroup(sasInstrumentGroupName + suffix);
        do_assert_instrument(instrument, instrumentName, idf, radiationSource,
                             detectors, invalidDetectors);

        // Check process
        auto process = entry.openGroup(sasProcessGroupName + suffix);
        do_assert_process(process, userFile);

        // Check data
        auto data = entry.openGroup(sasDataGroupName + suffix);
        do_assert_data(data, size, value, error, xmin, xmax, xerror);

        file.close();
    }
};

#endif /* MANTID_DATAHANDLING_SAVENXCANSASTEST_H_ */
