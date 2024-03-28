// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Axis.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/SaveAscii2.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <iostream>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

// This test tests that SaveAscii produces a file of the expected form
// It does not test that the file can be loaded by loadAscii.
// The test LoadSaveAscii does that and should be run in addition to this test
// if you modify SaveAscii.

class SaveAscii2Test : public CxxTest::TestSuite {

public:
  static SaveAscii2Test *createSuite() { return new SaveAscii2Test(); }
  static void destroySuite(SaveAscii2Test *suite) { delete suite; }

  SaveAscii2Test() {
    m_filename = "SaveAscii2TestFile.dat";
    m_filename_nohead = "SaveAsciiTest2FileWithoutHeader.dat";
    m_name = "SaveAscii2WS";
  }
  ~SaveAscii2Test() override = default;

  void testExec() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    int specID;
    std::string header1, header2, header3, separator, comment, distributionFlag;
    // Test that the first few column headers, separator and first two bins are
    // as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> distributionFlag >> specID;
    TS_ASSERT_EQUALS(specID, 1);
    TS_ASSERT_EQUALS(comment, "#");
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_EQUALS(header1, "X");
    TS_ASSERT_EQUALS(header2, "Y");
    TS_ASSERT_EQUALS(header3, "E");

    std::string binlines;
    std::vector<std::string> binstr;
    std::vector<double> bins;
    std::getline(in, binlines);
    std::getline(in, binlines);

    boost::split(binstr, binlines, boost::is_any_of(","));
    for (const auto &i : binstr) {
      bins.emplace_back(boost::lexical_cast<double>(i));
    }
    TS_ASSERT_EQUALS(bins[0], 0);
    TS_ASSERT_EQUALS(bins[1], 2);
    TS_ASSERT_EQUALS(bins[2], 1);

    std::getline(in, binlines);
    bins.clear();
    boost::split(binstr, binlines, boost::is_any_of(","));
    for (const auto &i : binstr) {
      bins.emplace_back(boost::lexical_cast<double>(i));
    }
    TS_ASSERT_EQUALS(bins[0], 1.66667);
    TS_ASSERT_EQUALS(bins[1], 8.66667);
    TS_ASSERT_EQUALS(bins[2], 1);

    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void test_one_spectrum_per_file() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(3, 5);
    ws->getAxis(0)->setUnit("MomentumTransfer");
    AnalysisDataService::Instance().addOrReplace("test_ws_one_per_file", ws);
    std::string filename = "saveascii2test.txt";
    SaveAscii2 savealg;
    TS_ASSERT_THROWS_NOTHING(savealg.initialize());
    savealg.setPropertyValue("InputWorkspace", "test_ws_one_per_file");
    savealg.setPropertyValue("Filename", filename);
    savealg.setProperty("OneSpectrumPerFile", true);
    filename = savealg.getPropertyValue("Filename");
    const size_t extPos = filename.find(".txt");

    // spectrum axis
    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    for (int spec = 0; spec < 3; ++spec) {
      std::ostringstream ss;
      ss << std::string(filename, 0, extPos) << "_" << spec << std::string(filename, extPos);
      TS_ASSERT(Poco::File(ss.str()).exists());
      Poco::File(ss.str()).remove();
    }

    // numeric axis
    std::unique_ptr<Axis> numericAxis = std::make_unique<NumericAxis>(3);
    for (int i = 0; i < 3; ++i) {
      numericAxis->setValue(i, i * i);
    }
    ws->replaceAxis(1, std::move(numericAxis));
    savealg.setPropertyValue("InputWorkspace", "test_ws_one_per_file");
    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    for (int spec = 0; spec < 3; ++spec) {
      std::ostringstream ss;
      ss << std::string(filename, 0, extPos) << "_" << spec << "_" << spec * spec << std::string(filename, extPos);
      std::cout << ss.str() << std::endl;
      TS_ASSERT(Poco::File(ss.str()).exists());
      Poco::File(ss.str()).remove();
    }

    // bin edge axis
    std::unique_ptr<Axis> binEdgeAxis = std::make_unique<BinEdgeAxis>(4);
    for (int i = 0; i < 4; ++i) {
      binEdgeAxis->setValue(i, i * i);
    }
    ws->replaceAxis(1, std::move(binEdgeAxis));
    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    for (int spec = 0; spec < 3; ++spec) {
      std::ostringstream ss;
      ss << std::string(filename, 0, extPos) << "_" << spec << "_" << 0.5 * (spec * spec + (spec + 1) * (spec + 1))
         << std::string(filename, extPos);
      TS_ASSERT(Poco::File(ss.str()).exists());
      Poco::File(ss.str()).remove();
    }

    // text axis
    std::unique_ptr<TextAxis> textAxis = std::make_unique<TextAxis>(3);
    for (int i = 0; i < 3; ++i) {
      textAxis->setLabel(i, std::string("ax_") + std::to_string(i));
    }
    ws->replaceAxis(1, std::unique_ptr<Axis>(std::move(textAxis)));
    TS_ASSERT_THROWS_NOTHING(savealg.execute());
    for (int spec = 0; spec < 3; ++spec) {
      std::ostringstream ss;
      ss << std::string(filename, 0, extPos) << "_" << spec << "_ax_" << spec << std::string(filename, extPos);
      TS_ASSERT(Poco::File(ss.str()).exists());
      Poco::File(ss.str()).remove();
    }
  }

  void testExec_DXNoData() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave = std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
        WorkspaceFactory::Instance().create("Workspace2D", 2, 3, 3));
    for (int i = 0; i < 2; i++) {
      std::vector<double> &X = wsToSave->dataX(i);
      std::vector<double> &Y = wsToSave->dataY(i);
      std::vector<double> &E = wsToSave->dataE(i);
      for (int j = 0; j < 3; j++) {
        X[j] = 1.5 * j / 0.9;
        Y[j] = (i + 1) * (2. + 4. * X[j]);
        E[j] = 1.;
      }
    }
    AnalysisDataService::Instance().add(m_name, wsToSave);
    SaveAscii2 save;
    initSaveAscii2(save);
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("WriteXError", "1"));
    TS_ASSERT_THROWS_NOTHING(save.execute());
    AnalysisDataService::Instance().remove(m_name);
  }

  void testExec_DX() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave = std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
        WorkspaceFactory::Instance().create("Workspace2D", 2, 3, 3));
    for (int i = 0; i < 2; i++) {
      auto &X = wsToSave->mutableX(i);
      auto &Y = wsToSave->mutableY(i);
      auto &E = wsToSave->mutableE(i);
      wsToSave->setPointStandardDeviations(i, 3);
      auto &DX = wsToSave->mutableDx(i);
      for (int j = 0; j < 3; j++) {
        X[j] = 1.5 * j / 0.9;
        Y[j] = (i + 1) * (2. + 4. * X[j]);
        E[j] = 1.;
        DX[j] = i + 1;
      }
    }

    AnalysisDataService::Instance().add(m_name, wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("WriteXError", "1"));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    int specID;
    std::string header1, header2, header3, header4, separator, comment, distributionFlag;

    // Test that the first few column headers, separator and first two bins are
    // as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> separator >> header4 >>
        distributionFlag >> specID;
    TS_ASSERT_EQUALS(specID, 1);
    TS_ASSERT_EQUALS(comment, "#");
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_EQUALS(header1, "X");
    TS_ASSERT_EQUALS(header2, "Y");
    TS_ASSERT_EQUALS(header3, "E");
    TS_ASSERT_EQUALS(header4, "DX");

    std::string binlines;
    std::vector<std::string> binstr;
    std::vector<double> bins;
    std::getline(in, binlines);
    std::getline(in, binlines);

    boost::split(binstr, binlines, boost::is_any_of(","));
    for (const auto &i : binstr) {
      bins.emplace_back(boost::lexical_cast<double>(i));
    }
    TS_ASSERT_EQUALS(bins[0], 0);
    TS_ASSERT_EQUALS(bins[1], 2);
    TS_ASSERT_EQUALS(bins[2], 1);

    std::getline(in, binlines);
    bins.clear();
    boost::split(binstr, binlines, boost::is_any_of(","));
    for (const auto &i : binstr) {
      bins.emplace_back(boost::lexical_cast<double>(i));
    }
    TS_ASSERT_EQUALS(bins[0], 1.66667);
    TS_ASSERT_EQUALS(bins[1], 8.66667);
    TS_ASSERT_EQUALS(bins[2], 1);

    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void test_valid_SpectrumMetaData_values() {
    MatrixWorkspace_sptr wsToSave;
    writeInelasticWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setProperty("SpectrumMetaData", "SpectrumNumber,Q,Angle"));
    TS_ASSERT_THROWS_NOTHING(save.setProperty("WriteSpectrumID", false));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    int specID;
    double qVal, angle;
    std::string header1, header2, header3, separator, comment, distributionFlag;

    // Test that the first few column headers, separator and first two bins are
    // as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> distributionFlag >> specID >>
        separator >> qVal >> separator >> angle;
    TS_ASSERT_EQUALS(comment, "#");
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_EQUALS(header1, "X");
    TS_ASSERT_EQUALS(header2, "Y");
    TS_ASSERT_EQUALS(header3, "E");
    TS_ASSERT_EQUALS(specID, 1);
    TS_ASSERT_EQUALS(qVal, 2.2092230401788049);
    TS_ASSERT_EQUALS(angle, 57.295779513082316);

    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void test_spectrum_axis_values() {
    MatrixWorkspace_sptr wsToSave;
    writeInelasticWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setProperty("WriteSpectrumAxisValue", true));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    double axisVal;
    std::string header1, header2, header3, separator, comment, distributionFlag;

    // Test that the first few column headers, separator and first two bins are
    // as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> distributionFlag >> axisVal;

    TS_ASSERT_EQUALS(comment, "#");
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_EQUALS(header1, "X");
    TS_ASSERT_EQUALS(header2, "Y");
    TS_ASSERT_EQUALS(header3, "E");
    TS_ASSERT_EQUALS(axisVal, 1.);
    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void test_Spectrum_Number_and_spec_ID_does_not_print_spec_num_twice() {
    MatrixWorkspace_sptr wsToSave;
    writeInelasticWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setProperty("SpectrumMetaData", "SpectrumNumber"));
    TS_ASSERT_THROWS_NOTHING(save.setProperty("WriteSpectrumID", true));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    int specID;
    double firstData;
    std::string header1, header2, header3, separator, comment, distributionFlag;

    // Test that the first few column headers, separator and first two bins are
    // as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> distributionFlag >> specID >> firstData;
    TS_ASSERT_EQUALS(comment, "#");
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_EQUALS(header1, "X");
    TS_ASSERT_EQUALS(header2, "Y");
    TS_ASSERT_EQUALS(header3, "E");
    TS_ASSERT_EQUALS(specID, 1);
    TS_ASSERT_EQUALS(firstData, -6.66667);

    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void test_input_success_with_valid_SpectrumMetaData_list() {
    MatrixWorkspace_sptr wsToSave;
    writeInelasticWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("SpectrumMetaData", "SpectrumNumber,Q,Angle"));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // the algorithm will have used a defualt and written a file to disk
    TS_ASSERT(Poco::File(filename).exists());
    Poco::File(filename).remove();

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_non_spectrum_axisworkspace() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave, false);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setProperty("WriteSpectrumID", true));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    int specID;
    std::string header1, header2, header3, separator, comment, distributionFlag;

    // Test that the first few column headers, separator and first two bins are
    // as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> distributionFlag >> specID;
    TS_ASSERT_EQUALS(comment, "#");
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_EQUALS(header1, "X");
    TS_ASSERT_EQUALS(header2, "Y");
    TS_ASSERT_EQUALS(header3, "E");
    TS_ASSERT_EQUALS(specID, 1);

    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void testExec_no_header() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    save.setPropertyValue("Filename", m_filename_nohead);
    save.setPropertyValue("InputWorkspace", m_name);
    TS_ASSERT_THROWS_NOTHING(save.setProperty("ColumnHeader", false));
    std::string filename_nohead = save.getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename_nohead).exists());

    // Now we check that the first line of the file without header matches the
    // second line of the file with header
    std::ifstream in1(filename.c_str());
    std::string line2header;
    std::ifstream in2(filename_nohead.c_str());
    std::string line1noheader;
    getline(in1, line2header);
    getline(in1, line2header);
    getline(in1, line2header); // 3rd line of file with header
    getline(in2, line1noheader);
    getline(in2, line1noheader); // 2nd line of file without header
    TS_ASSERT_EQUALS(line1noheader, line2header);
    in1.close();
    in2.close();
    // Remove files
    Poco::File(filename).remove();
    Poco::File(filename_nohead).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void testExec_samplelogs_header() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);
    wsToSave->mutableRun().addProperty("wavelength", 6.0);
    wsToSave->mutableRun().addProperty("instrument_name", std::string{"CustomInstrument"});
    SaveAscii2 save;
    std::vector<std::string> logList = {"wavelength", "instrument_name", "does_not_exist"};
    std::string filename = initSaveAscii2(save, logList);

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now we check that the first line of the file without header matches the
    // second line of the file with header
    std::ifstream in1(filename.c_str());
    std::string line1, line2, line3;
    getline(in1, line1);
    getline(in1, line2);
    getline(in1, line3);
    TS_ASSERT_EQUALS(line1, "wavelength,6,");
    TS_ASSERT_EQUALS(line2, "instrument_name,CustomInstrument,");
    TS_ASSERT_EQUALS(line3, "does_not_exist,Not defined,");
    in1.close();
    // Remove files
    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void test_CustomSeparator_override() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    // Separator is left at default on purpouse to see if Customseparator
    // overrides it
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CustomSeparator", "/"));

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    int specID;
    std::string header1, header2, header3, separator, comment, distributionFlag;

    // Test that the first few column headers, separator and first two bins are
    // as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> distributionFlag >> specID;
    TS_ASSERT_EQUALS(specID, 1);
    TS_ASSERT_EQUALS(comment, "#");
    // the algorithm will use a custom one if supplied even if the type selected
    // is not "UserDefined"
    TS_ASSERT_EQUALS(separator, "/");
    TS_ASSERT_EQUALS(header1, "X");
    TS_ASSERT_EQUALS(header2, "Y");
    TS_ASSERT_EQUALS(header3, "E");

    in.close();
    Poco::File(filename).remove();

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_SpectrumList() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("SpectrumList", "2, 1"));

    TS_ASSERT_THROWS_ANYTHING(save.execute());

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(filename).exists());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_workspace() {
    SaveAscii2 save;
    save.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(save.initialize());
    TS_ASSERT(save.isInitialized());
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Filename", m_filename));
    std::string filename = save.getPropertyValue("Filename"); // Get absolute path
    TS_ASSERT_THROWS_ANYTHING(save.setPropertyValue("InputWorkspace", "NotARealWS"));
    TS_ASSERT_THROWS_ANYTHING(save.execute());

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(filename).exists());
  }

  void test_fail_invalid_IndexMax() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("WorkspaceIndexMin", "1"));
    // first check the validator
    TS_ASSERT_THROWS_ANYTHING(save.setProperty("WorkspaceIndexMax", -1));
    // then check the workspace bounds testing
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("WorkspaceIndexMax", "5"));

    TS_ASSERT_THROWS(save.execute(), const std::invalid_argument &);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(filename).exists());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_IndexMin() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);
    // first check the validator
    TS_ASSERT_THROWS_ANYTHING(save.setProperty("WorkspaceIndexMin", -1));
    // then check the workspace bounds testing
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("WorkspaceIndexMin", "5"));
    // the problem is that this will throw regardless as no numbers below zero can
    // get in so i have to go over the bounds
    // so i have to either force Max higher or overlap, and both are tested
    // separatly
    // the validator seems to replace "-1" with the same as EMPTY_INT() so the
    // bounds aren't checked
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("WorkspaceIndexMax", "7"));

    TS_ASSERT_THROWS(save.execute(), const std::invalid_argument &);

    // the algorithm didn't run so there should be no file
    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(filename).exists());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_spectrum_number_in_meta_data_for_non_spectrum_axis_ws() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave, false);

    SaveAscii2 save;
    initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setProperty("SpectrumMetaData", "SpectrumNumber"));
    TS_ASSERT_THROWS_ANYTHING(save.execute());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_IndexMin_Max_Overlap() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    if (Poco::File(filename).exists())
      Poco::File(filename).remove();

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("WorkspaceIndexMin", "3"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("WorkspaceIndexMax", "2"));

    TS_ASSERT_THROWS(save.execute(), const std::invalid_argument &);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(filename).exists());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_SpectrumList() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_ANYTHING(save.setPropertyValue("SpectrumList", "2 3 1"));

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // the algorithm will have used a defualt and written a file to disk
    TS_ASSERT(Poco::File(filename).exists());
    Poco::File(filename).remove();

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_SpectrumList_exceeds() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("SpectrumList", "2, 3, 1"));

    TS_ASSERT_THROWS(save.execute(), const std::invalid_argument &);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(filename).exists());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_valid_SpectrumList() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("SpectrumList", "1"));

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    int specID;
    std::string header1, header2, header3, separator, comment, distributionFlag;
    // Test that the first few column headers, separator and first two bins are
    // as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> distributionFlag >> specID;
    TS_ASSERT_EQUALS(specID, 2);
    TS_ASSERT_EQUALS(comment, "#");
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_EQUALS(header1, "X");
    TS_ASSERT_EQUALS(header2, "Y");
    TS_ASSERT_EQUALS(header3, "E");

    std::string binlines;
    std::vector<std::string> binstr;
    std::vector<double> bins;
    std::getline(in, binlines);
    std::getline(in, binlines);

    boost::split(binstr, binlines, boost::is_any_of(","));
    for (const auto &i : binstr) {
      bins.emplace_back(boost::lexical_cast<double>(i));
    }
    TS_ASSERT_EQUALS(bins[0], 0);
    TS_ASSERT_EQUALS(bins[1], 4);
    TS_ASSERT_EQUALS(bins[2], 1);

    std::getline(in, binlines);
    bins.clear();
    boost::split(binstr, binlines, boost::is_any_of(","));
    for (const auto &i : binstr) {
      bins.emplace_back(boost::lexical_cast<double>(i));
    }
    TS_ASSERT_EQUALS(bins[0], 1.66667);
    TS_ASSERT_EQUALS(bins[1], 17.3333);
    TS_ASSERT_EQUALS(bins[2], 1);

    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_Precision() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_ANYTHING(save.setPropertyValue("Precision", "-4"));

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // the algorithm will have used a defualt and written a file to disk
    TS_ASSERT(Poco::File(filename).exists());
    Poco::File(filename).remove();

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_CommentIndicator_number() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CommentIndicator", "3"));

    TS_ASSERT_THROWS(save.execute(), const std::invalid_argument &);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(filename).exists());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_CommentIndicator_e() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CommentIndicator", "e"));

    TS_ASSERT_THROWS(save.execute(), const std::invalid_argument &);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(filename).exists());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_CommentIndicator_hyphen() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CommentIndicator", "-"));

    TS_ASSERT_THROWS(save.execute(), const std::invalid_argument &);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(filename).exists());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_CommentIndicator_plus() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CommentIndicator", "+"));

    TS_ASSERT_THROWS(save.execute(), const std::invalid_argument &);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(filename).exists());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_Separator_e() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Separator", "UserDefined"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CustomSeparator", "e"));

    TS_ASSERT_THROWS(save.execute(), const std::invalid_argument &);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(filename).exists());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_Separator_number() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Separator", "UserDefined"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CustomSeparator", "3"));

    TS_ASSERT_THROWS(save.execute(), const std::invalid_argument &);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(filename).exists());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_Separator_plus() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Separator", "UserDefined"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CustomSeparator", "+"));

    TS_ASSERT_THROWS(save.execute(), const std::invalid_argument &);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(filename).exists());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_Separator_hyphen() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Separator", "UserDefined"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CustomSeparator", "-"));

    TS_ASSERT_THROWS(save.execute(), const std::invalid_argument &);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(filename).exists());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_Separator() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_ANYTHING(save.setPropertyValue("Separator", "NotAValidChoice"));

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // the algorithm will have used a defualt and written a file to disk
    TS_ASSERT(Poco::File(filename).exists());
    Poco::File(filename).remove();

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_with_invalid_SpectrumMetaData() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("SpectrumMetaData", "NotAValidChoice"));
    TS_ASSERT_THROWS_ANYTHING(save.execute());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_clash_CustomSeparator_CustomComment() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CommentIndicator", "@"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Separator", "UserDefined"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CustomSeparator", "@"));

    TS_ASSERT_THROWS_ANYTHING(save.execute());

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT(!Poco::File(filename).exists());

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_TableWorkspace() {
    Workspace_sptr wsToSave = writeTableWS(m_name);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    std::string header1, header2, header3, header4, header5, header6, header7, header8, header9, separator, comment,
        type1, type2;

    // Test that the first few column headers, separator and first two types are
    // as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> separator >> header4 >> separator >>
        header5 >> separator >> header6 >> separator >> header7 >> separator >> header8 >> separator >> header9 >>
        separator >> type1 >> separator >> type2;

    TS_ASSERT_EQUALS(comment, "#");
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_EQUALS(header1, "int");
    TS_ASSERT_EQUALS(header2, "uint");
    TS_ASSERT_EQUALS(header3, "int64");
    TS_ASSERT_EQUALS(header4, "size_t");
    TS_ASSERT_EQUALS(header5, "float");
    TS_ASSERT_EQUALS(header6, "double");
    TS_ASSERT_EQUALS(header7, "bool");
    TS_ASSERT_EQUALS(header8, "string");
    TS_ASSERT_EQUALS(header9, "V3D");
    TS_ASSERT_EQUALS(type1, "int");
    TS_ASSERT_EQUALS(type2, "uint");

    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void test_OnSpectrumPerFile() {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);
    save.setProperty("OneSpectrumPerFile", true);

    TS_ASSERT_THROWS_NOTHING(save.execute());
    size_t extPos = filename.find(".dat");
    std::ostringstream ss0, ss1;
    ss0 << std::string(filename, 0, extPos) << "_0" << std::string(filename, extPos);
    ss1 << std::string(filename, 0, extPos) << "_1" << std::string(filename, extPos);
    TS_ASSERT(Poco::File(ss0.str()).exists());
    TS_ASSERT(Poco::File(ss1.str()).exists());
    AnalysisDataService::Instance().remove(m_name);
  }

  void test_Distribution_true_Header_true() {
    // check the stream saved to file
    // contains the header flag Distribution=true
    // when the workspace is a Distribution

    MatrixWorkspace_sptr wsToSave;
    writeInelasticWS(wsToSave);
    wsToSave->setDistribution(true);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setProperty("SpectrumMetaData", "SpectrumNumber,Q,Angle"));
    TS_ASSERT_THROWS_NOTHING(save.setProperty("WriteSpectrumID", false));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    int specID;
    double qVal, angle;
    std::string header1, header2, header3, separator, comment, distributionFlag;

    // Test that the first few column headers, separator and first two bins are
    // as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> distributionFlag >> specID >>
        separator >> qVal >> separator >> angle;
    TS_ASSERT_EQUALS(comment, "#");
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_EQUALS(header1, "X");
    TS_ASSERT_EQUALS(header2, "Y");
    TS_ASSERT_EQUALS(header3, "E");
    TS_ASSERT_EQUALS(distributionFlag, "Distribution=true");
    TS_ASSERT_EQUALS(specID, 1);
    TS_ASSERT_EQUALS(qVal, 2.2092230401788049);
    TS_ASSERT_EQUALS(angle, 57.295779513082316);

    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void test_Distribution_false_Header_true() {
    // check the stream saved to file
    // contains the header flag Distribution=false
    // when the workspace is NOT a Distribution

    MatrixWorkspace_sptr wsToSave;
    writeInelasticWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setProperty("SpectrumMetaData", "SpectrumNumber,Q,Angle"));
    TS_ASSERT_THROWS_NOTHING(save.setProperty("WriteSpectrumID", false));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    int specID;
    double qVal, angle;
    std::string header1, header2, header3, separator, comment, distributionFlag;

    // Test that the first few column headers, separator and first two bins are
    // as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> distributionFlag >> specID >>
        separator >> qVal >> separator >> angle;
    TS_ASSERT_EQUALS(comment, "#");
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_EQUALS(header1, "X");
    TS_ASSERT_EQUALS(header2, "Y");
    TS_ASSERT_EQUALS(header3, "E");
    TS_ASSERT_EQUALS(distributionFlag, "Distribution=false");
    TS_ASSERT_EQUALS(specID, 1);
    TS_ASSERT_EQUALS(qVal, 2.2092230401788049);
    TS_ASSERT_EQUALS(angle, 57.295779513082316);

    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void test_Distribution_true_Header_false() {
    // check the stream saved to file
    // contains the header flag Distribution=true
    // when the workspace is a Distribution
    // even when ColumnHeader=false

    // ColumnHeader should be written, even when ColumnHeader=false, but Distribution=true
    MatrixWorkspace_sptr wsToSave;
    writeInelasticWS(wsToSave);
    wsToSave->setDistribution(true);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setProperty("SpectrumMetaData", "SpectrumNumber,Q,Angle"));
    TS_ASSERT_THROWS_NOTHING(save.setProperty("WriteSpectrumID", false));
    TS_ASSERT_THROWS_NOTHING(save.setProperty("ColumnHeader", false));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    int specID;
    double qVal, angle;
    std::string header1, header2, header3, separator, comment, distributionFlag;

    // Test that the first few column headers, separator and first two bins are
    // as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> distributionFlag >> specID >>
        separator >> qVal >> separator >> angle;
    TS_ASSERT_EQUALS(comment, "#");
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_EQUALS(header1, "X");
    TS_ASSERT_EQUALS(header2, "Y");
    TS_ASSERT_EQUALS(header3, "E");
    TS_ASSERT_EQUALS(distributionFlag, "Distribution=true");
    TS_ASSERT_EQUALS(specID, 1);
    TS_ASSERT_EQUALS(qVal, 2.2092230401788049);
    TS_ASSERT_EQUALS(angle, 57.295779513082316);

    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void test_Distribution_false_Header_false() {
    // check the stream saved to file
    // does NOT contain a header
    // when the workspace is NOT a Distribution
    // and ColumnHeader=false

    // ColumnHeader not written, when ColumnHeader=false and Distribution=false
    MatrixWorkspace_sptr wsToSave;
    writeInelasticWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setProperty("SpectrumMetaData", "SpectrumNumber,Q,Angle"));
    TS_ASSERT_THROWS_NOTHING(save.setProperty("WriteSpectrumID", false));
    TS_ASSERT_THROWS_NOTHING(save.setProperty("ColumnHeader", false));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());
    int specID;
    double qVal, angle;
    std::string header1, header2, header3, separator, comment, distributionFlag;

    // Test that the first few column headers, separator and first two bins are
    // as expected
    in >> specID >> separator >> qVal >> separator >> angle;
    TS_ASSERT_EQUALS(specID, 1);
    TS_ASSERT_EQUALS(qVal, 2.2092230401788049);
    TS_ASSERT_EQUALS(angle, 57.295779513082316);

    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void test_1DHistoCuts() {
    write1MDHistoWS(m_name);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT(Poco::File(filename).exists());

    // Now make some checks on the content of the file
    std::ifstream in(filename.c_str());

    // Check the first four lines
    std::vector<std::string> lines(4, "");
    for (int i = 0; i < 4; i++) {
      std::getline(in, lines[i]);
    }
    TS_ASSERT_EQUALS(
        lines[0],
        "# {InputWorkspace: SaveAscii2WS, AxisAligned: 0, AlignedDim0: , AlignedDim1: , AlignedDim2: , AlignedDim3: , "
        "AlignedDim4: , AlignedDim5: , BasisVector0: (0.0+1.0x 0.0 0.0), in 7.26 Ang^-1, 1.0,0.0,0.0, BasisVector1: "
        "u2, in 7.26 Ang^-1, 0.0,1.0,0.0, BasisVector2: u3, in 3.14 Ang^-1,0.0,0.0,1.0, BasisVector3: , BasisVector4: "
        ", BasisVector5: , Translation: , OutputExtents: -2.5,2.5,-0.16,0.16,-0.05,0.05, OutputBins: 1,50,1, "
        "NormalizeBasisVectors: 1, ForceOrthogonal: 0, ImplicitFunctionXML: , IterateEvents: 1, Parallel: 0, "
        "TemporaryDataWorkspace: , OutputWorkspace: " +
            m_name + " }");
    TS_ASSERT_EQUALS(lines[1], "# u2 in 7.26 Ang^-1, Signal, Error");
    TS_ASSERT_EQUALS(lines[2], "-0.1568,3,1.73205");
    TS_ASSERT_EQUALS(lines[3], "-0.1504,0,0");

    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  // public as it is used in LoadAsciiTest as well.
  static ITableWorkspace_sptr writeTableWS(const std::string &name) {
    auto table = WorkspaceFactory::Instance().createTable();
    // One column of each type
    table->addColumn("int", "int");
    table->addColumn("uint", "uint");
    table->addColumn("long64", "int64");
    table->addColumn("size_t", "size_t");
    table->addColumn("float", "float");
    table->addColumn("double", "double");
    table->addColumn("bool", "bool");
    table->addColumn("str", "string");
    table->addColumn("V3D", "V3D");

    // A few rows
    TableRow row1 = table->appendRow();
    row1 << -1 << static_cast<uint32_t>(0) << static_cast<int64_t>(1) << static_cast<size_t>(10) << 5.5f << -9.9 << true
         << "Hello" << Mantid::Kernel::V3D();
    TableRow row2 = table->appendRow();
    row2 << 1 << static_cast<uint32_t>(2) << static_cast<int64_t>(-2) << static_cast<size_t>(100) << 0.0f << 101.0
         << false << "World" << Mantid::Kernel::V3D(-1, 3, 4);
    TableRow row3 = table->appendRow();
    row3 << 6 << static_cast<uint32_t>(3) << static_cast<int64_t>(0) << static_cast<size_t>(0) << -99.0f << 0.0 << false
         << "!" << Mantid::Kernel::V3D(1, 6, 10);

    AnalysisDataService::Instance().add(name, table);
    return table;
  }

private:
  void writeSampleWS(Mantid::DataObjects::Workspace2D_sptr &wsToSave, const bool &isSpectra = true) {
    wsToSave = std::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(
        WorkspaceFactory::Instance().create("Workspace2D", 2, 3, 3));
    for (int i = 0; i < 2; i++) {
      auto &X = wsToSave->mutableX(i);
      auto &Y = wsToSave->mutableY(i);
      auto &E = wsToSave->mutableE(i);
      for (int j = 0; j < 3; j++) {
        X[j] = 1.5 * j / 0.9;
        Y[j] = (i + 1) * (2. + 4. * X[j]);
        E[j] = 1.;
      }
    }

    if (!isSpectra) {
      auto textAxis = std::make_unique<TextAxis>(2);
      textAxis->setLabel(0, "Test Axis 1");
      textAxis->setLabel(1, "Test Axis 2");
      wsToSave->replaceAxis(1, std::move(textAxis));
    }

    AnalysisDataService::Instance().add(m_name, wsToSave);
  }

  void writeInelasticWS(MatrixWorkspace_sptr &wsToSave) {
    const std::vector<double> l2{1, 2, 3, 4, 5};
    const std::vector<double> polar{1, 2, 3, 4, 5};
    const std::vector<double> azimutal{1, 2, 3, 4, 5};
    const int nBins = 3;

    wsToSave = WorkspaceCreationHelper::createProcessedInelasticWS(l2, polar, azimutal, nBins);
    AnalysisDataService::Instance().add(m_name, wsToSave);
  }

  void write1MDHistoWS(const std::string name) {
    FrameworkManager::Instance().exec("CreateMDWorkspace", 16, "Dimensions", "3", "Extents", "-5,5,-4,4,-3,3", "Names",
                                      "H,K,L", "Units", "r.l.u., r.l.u., r.l.u.", "Frames", "HKL,HKL,HKL", "SplitInto",
                                      "2", "SplitThreshold", "50", "OutputWorkspace", name.c_str());

    FrameworkManager::Instance().exec("FakeMDEventData", 8, "InputWorkspace", name.c_str(), "UniformParams", "100000",
                                      "PeakParams", "100000,0,0,1,0.3", "RandomSeed", "3873875");

    FrameworkManager::Instance().exec(
        "BinMD", 18, "InputWorkspace", name.c_str(), "AxisAligned", "0", "BasisVector0",
        "(0.0+1.0x 0.0 0.0), in 7.26 Ang^-1, 1.0,0.0,0.0", "BasisVector1", "u2, in 7.26 Ang^-1, 0.0,1.0,0.0",
        "BasisVector2", "u3, in 3.14 Ang^-1,0.0,0.0,1.0", "OutputExtents", "-2.5,2.5,-0.16,0.16,-0.05,0.05",
        "OutputBins", "1,50,1", "NormalizeBasisVectors", "1", "OutputWorkspace", name.c_str());
  }

  std::string initSaveAscii2(SaveAscii2 &save, std::vector<std::string> logList = {}) {
    save.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(save.initialize());
    TS_ASSERT(save.isInitialized());
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Filename", m_filename));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("InputWorkspace", m_name));
    TS_ASSERT_THROWS_NOTHING(save.setProperty("LogList", logList));
    return save.getPropertyValue("Filename"); // return absolute path
  }

  std::string m_filename;
  std::string m_filename_nohead;
  std::string m_name;
};

class SaveAscii2TestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    createInelasticWS();
    for (int i = 0; i < numberOfIterations; ++i) {
      saveAlgPtrs.emplace_back(setupAlg());
    }
  }

  void testSaveAscii2Performance() {
    for (auto alg : saveAlgPtrs) {
      TS_ASSERT_THROWS_NOTHING(alg->execute());
    }
  }

  void tearDown() override {
    for (int i = 0; i < numberOfIterations; i++) {
      delete saveAlgPtrs[i];
      saveAlgPtrs[i] = nullptr;
    }
    Mantid::API::AnalysisDataService::Instance().remove(m_name);
    Poco::File gsasfile(m_filename);
    if (gsasfile.exists())
      gsasfile.remove();
  }

private:
  std::vector<SaveAscii2 *> saveAlgPtrs;

  const int numberOfIterations = 5;

  const std::string m_filename = "performance_filename";
  const std::string m_name = "performance_ws";

  SaveAscii2 *setupAlg() {
    SaveAscii2 *saver = new SaveAscii2;
    saver->initialize();
    saver->isInitialized();
    saver->initialize();
    saver->isInitialized();
    saver->setPropertyValue("Filename", m_filename);
    saver->setPropertyValue("InputWorkspace", m_name);
    saver->setRethrows(true);
    return saver;
  }

  void createInelasticWS() {
    const std::vector<double> l2{1, 2, 3, 4, 5};
    const std::vector<double> polar{1, 2, 3, 4, 5};
    const std::vector<double> azimutal{1, 2, 3, 4, 5};
    const int nBins = 3;

    auto wsToSave = WorkspaceCreationHelper::createProcessedInelasticWS(l2, polar, azimutal, nBins);
    AnalysisDataService::Instance().add(m_name, wsToSave);
  }
};
