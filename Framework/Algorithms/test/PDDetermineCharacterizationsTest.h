// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_PDDETERMINECHARACTERIZATIONSTEST_H_
#define MANTID_ALGORITHMS_PDDETERMINECHARACTERIZATIONSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/PDDetermineCharacterizations.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/PropertyWithValue.h"

using Mantid::Algorithms::PDDetermineCharacterizations;
using namespace Mantid::API;
using namespace Mantid::Kernel;

const std::string PROPERTY_MANAGER_NAME = "__pd_reduction_properties";

class PDDetermineCharacterizationsTest : public CxxTest::TestSuite {
private:
  std::string m_logWSName;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PDDetermineCharacterizationsTest *createSuite() {
    return new PDDetermineCharacterizationsTest();
  }
  static void destroySuite(PDDetermineCharacterizationsTest *suite) {
    delete suite;
  }

  void createLogWksp(const std::string &frequency,
                     const std::string &wavelength,
                     const std::string &canName = std::string("")) {
    m_logWSName = "_det_char_log";

    {
      auto alg =
          FrameworkManager::Instance().createAlgorithm("CreateWorkspace");
      alg->setPropertyValue("DataX",
                            "-1.0,-0.8,-0.6,-0.4,-0.2,0.0,0.2,0.4,0.6,0.8,1.0");
      alg->setPropertyValue("DataY",
                            "-1.0,-0.8,-0.6,-0.4,-0.2,0.0,0.2,0.4,0.6,0.8");
      alg->setPropertyValue("OutputWorkspace", m_logWSName);
      TS_ASSERT(alg->execute());
    }
    {
      auto alg = FrameworkManager::Instance().createAlgorithm("AddSampleLog");
      alg->setPropertyValue("LogName", "frequency");
      alg->setProperty("LogText", frequency);
      alg->setPropertyValue("LogUnit", "Hz");
      alg->setPropertyValue("LogType", "Number");
      alg->setPropertyValue("Workspace", m_logWSName);
      TS_ASSERT(alg->execute());
    }
    {
      auto alg = FrameworkManager::Instance().createAlgorithm("AddSampleLog");
      alg->setPropertyValue("LogName", "LambdaRequest");
      alg->setPropertyValue("LogText", wavelength);
      alg->setPropertyValue("LogUnit", "Angstrom");
      alg->setPropertyValue("LogType", "Number");
      alg->setPropertyValue("Workspace", m_logWSName);
      TS_ASSERT(alg->execute());
    }

    if (!canName.empty()) {
      auto alg = FrameworkManager::Instance().createAlgorithm("AddSampleLog");
      alg->setPropertyValue("LogName", "SampleContainer");
      alg->setPropertyValue("LogText", canName);
      alg->setPropertyValue("LogType", "String");
      alg->setPropertyValue("Workspace", m_logWSName);
      TS_ASSERT(alg->execute());
    }
  }

  void addRow(ITableWorkspace_sptr wksp, const double freq, const double wl,
              const int bank, const std::string &van,
              const std::string &van_back, const std::string &can,
              const std::string &empty_env, const std::string &empty_inst,
              const std::string &dmin, const std::string &dmax,
              const double tofmin, const double tofmax, const double wlmin,
              const double wlmax,
              const std::string &canExtra = std::string("")) {
    Mantid::API::TableRow row = wksp->appendRow();
    row << freq;
    row << wl;
    row << bank;
    row << van;
    row << van_back;
    row << can;
    row << empty_env;
    row << empty_inst;
    row << dmin;
    row << dmax;
    row << tofmin;
    row << tofmax;
    row << wlmin;
    row << wlmax;
    if (!canExtra.empty())
      row << canExtra;
  }

  ITableWorkspace_sptr
  createEmptyTableWksp(const std::string &canName = std::string("")) {
    ITableWorkspace_sptr wksp = WorkspaceFactory::Instance().createTable();
    wksp->addColumn("double", "frequency");
    wksp->addColumn("double", "wavelength");
    wksp->addColumn("int", "bank");
    wksp->addColumn("str", "vanadium");
    wksp->addColumn("str", "vanadium_background");
    wksp->addColumn("str", "container");
    wksp->addColumn("str", "empty_environment");
    wksp->addColumn("str", "empty_instrument");
    wksp->addColumn("str", "d_min"); // b/c it is an array for NOMAD
    wksp->addColumn("str", "d_max"); // b/c it is an array for NOMAD
    wksp->addColumn("double", "tof_min");
    wksp->addColumn("double", "tof_max");
    wksp->addColumn("double", "wavelength_min");
    wksp->addColumn("double", "wavelength_max");
    if (!canName.empty()) {
      wksp->addColumn("str", canName);
    }

    return wksp;
  }

  ITableWorkspace_sptr createTableWkspPG3() {
    ITableWorkspace_sptr wksp = createEmptyTableWksp("PAC08");

    addRow(wksp, 60., 0.533, 1, "17702", "1234", "17711", "0", "0", "0.05",
           "2.20", 0000.00, 16666.67, 0., 0., "12345");
    addRow(wksp, 60., 1.333, 3, "17703", "1235", "17712", "0", "0", "0.43",
           "5.40", 12500.00, 29166.67, 0., 0., "12346");
    addRow(wksp, 60., 2.665, 4, "17704", "1236", "17713", "0", "0", "1.15",
           "9.20", 33333.33, 50000.00, 0., 0., "12347");
    addRow(wksp, 60., 4.797, 5, "17705", "1237", "17714", "0", "0", "2.00",
           "15.35", 66666.67, 83333.67, 0., 0., "12348");

    return wksp;
  }

  ITableWorkspace_sptr createTableWkspNOM() {
    ITableWorkspace_sptr wksp = createEmptyTableWksp();

    addRow(wksp, 60., 1.4, 1, "0", "0", "0", "0", "0",
           ".31,.25,.13,.13,.13,.42", "13.66,5.83,3.93,2.09,1.57,31.42", 300.00,
           16666.67, 0., 0.);

    return wksp;
  }

  ITableWorkspace_sptr createTableWkspNOM_withwl() {
    ITableWorkspace_sptr wksp = createEmptyTableWksp();

    addRow(wksp, 60., 1.4, 1, "0", "0", "0", "0", "0",
           ".31,.25,.13,.13,.13,.42", "13.66,5.83,3.93,2.09,1.57,31.42", 300.00,
           16666.67, .9, 2.1);

    return wksp;
  }

  PropertyManager_sptr
  createExpectedInfo(const double freq, const double wl, const int bank,
                     const std::string &van, const std::string &vanback,
                     const std::string &can, const std::string &empty,
                     const std::string &dmin, const std::string &dmax,
                     const double tofmin, const double tofmax,
                     const double wlmin, const double wlmax) {

    PropertyManager_sptr expectedInfo = boost::make_shared<PropertyManager>();
    expectedInfo->declareProperty(
        std::make_unique<PropertyWithValue<double>>("frequency", freq));
    expectedInfo->declareProperty(
        std::make_unique<PropertyWithValue<double>>("wavelength", wl));
    expectedInfo->declareProperty(
        std::make_unique<PropertyWithValue<int>>("bank", bank));
    expectedInfo->declareProperty(
        std::make_unique<ArrayProperty<int32_t>>("vanadium", van));
    expectedInfo->declareProperty(std::make_unique<ArrayProperty<int32_t>>(
        "vanadium_background", vanback));
    expectedInfo->declareProperty(
        std::make_unique<ArrayProperty<int32_t>>("container", can));
    expectedInfo->declareProperty(
        std::make_unique<ArrayProperty<int32_t>>("empty_environment", "0"));
    expectedInfo->declareProperty(
        std::make_unique<ArrayProperty<int32_t>>("empty_instrument", empty));
    expectedInfo->declareProperty(
        std::make_unique<ArrayProperty<double>>("d_min", dmin));
    expectedInfo->declareProperty(
        std::make_unique<ArrayProperty<double>>("d_max", dmax));
    expectedInfo->declareProperty(
        std::make_unique<PropertyWithValue<double>>("tof_min", tofmin));
    expectedInfo->declareProperty(
        std::make_unique<PropertyWithValue<double>>("tof_max", tofmax));
    expectedInfo->declareProperty(
        std::make_unique<PropertyWithValue<double>>("wavelength_min", wlmin));
    expectedInfo->declareProperty(
        std::make_unique<PropertyWithValue<double>>("wavelength_max", wlmax));

    return expectedInfo;
  }

  void compareResult(PropertyManager_sptr expected,
                     PropertyManager_sptr observed) {
    TS_ASSERT_EQUALS(expected->propertyCount(), observed->propertyCount());

    const std::vector<Property *> &expectedProps = expected->getProperties();

    for (auto expectedProp : expectedProps) {
      const std::string name = expectedProp->name();
      TS_ASSERT_EQUALS(expected->getPropertyValue(name),
                       observed->getPropertyValue(name));
    }
  }

  void test_Init() {
    PDDetermineCharacterizations alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void testNoChar() {
    createLogWksp("60.", "0.533");
    // don't create characterization table

    PDDetermineCharacterizations alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", m_logWSName));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ReductionProperties", PROPERTY_MANAGER_NAME));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    auto expectedInfo = createExpectedInfo(0., 0., 1, "0", "0", "0", "0", "",
                                           "", 0., 0., 0., 0.);

    compareResult(expectedInfo, PropertyManagerDataService::Instance().retrieve(
                                    PROPERTY_MANAGER_NAME));
  }

  void testEmptyChar() {
    createLogWksp("60.", "0.533");
    auto tableWS = createEmptyTableWksp();

    PDDetermineCharacterizations alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", m_logWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Characterizations", tableWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ReductionProperties", PROPERTY_MANAGER_NAME));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    auto expectedInfo = createExpectedInfo(0., 0., 1, "0", "0", "0", "0", "",
                                           "", 0., 0., 0., 0.);

    compareResult(expectedInfo, PropertyManagerDataService::Instance().retrieve(
                                    PROPERTY_MANAGER_NAME));
  }

  void testFullChar() {
    createLogWksp("60.", "0.533");
    auto tableWS = createTableWkspPG3();

    PDDetermineCharacterizations alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", m_logWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Characterizations", tableWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ReductionProperties", PROPERTY_MANAGER_NAME));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    auto expectedInfo =
        createExpectedInfo(60., 0.533, 1, "17702", "1234", "17711", "0", "0.05",
                           "2.20", 0000.00, 16666.67, 0., 0.);

    compareResult(expectedInfo, PropertyManagerDataService::Instance().retrieve(
                                    PROPERTY_MANAGER_NAME));
  }

  void testFullCharDisableChar() {
    createLogWksp("60.", "0.533");
    auto tableWS = createTableWkspPG3();

    PDDetermineCharacterizations alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", m_logWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Characterizations", tableWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BackRun", "-1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormRun", "-1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormBackRun", "-1"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ReductionProperties", PROPERTY_MANAGER_NAME));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    auto expectedInfo =
        createExpectedInfo(60., 0.533, 1, "0", "0", "0", "0", "0.05", "2.20",
                           0000.00, 16666.67, 0., 0.);

    compareResult(expectedInfo, PropertyManagerDataService::Instance().retrieve(
                                    PROPERTY_MANAGER_NAME));
  }

  void testFullCharWithCan() {
    createLogWksp("60.", "0.533", "PAC 08");
    auto tableWS = createTableWkspPG3();

    PDDetermineCharacterizations alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", m_logWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Characterizations", tableWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ReductionProperties", PROPERTY_MANAGER_NAME));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    auto expectedInfo =
        createExpectedInfo(60., 0.533, 1, "17702", "1234", "12345", "0", "0.05",
                           "2.20", 0000.00, 16666.67, 0., 0.);

    compareResult(expectedInfo, PropertyManagerDataService::Instance().retrieve(
                                    PROPERTY_MANAGER_NAME));
  }

  void testFullCharNom() {
    createLogWksp("60.", "1.4");
    auto tableWS = createTableWkspNOM();

    PDDetermineCharacterizations alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", m_logWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Characterizations", tableWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ReductionProperties", PROPERTY_MANAGER_NAME));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    auto expectedInfo = createExpectedInfo(
        60., 1.4, 1, "0", "0", "0", "0", ".31,.25,.13,.13,.13,.42",
        "13.66,5.83,3.93,2.09,1.57,31.42", 300.00, 16666.67, 0., 0.);

    compareResult(expectedInfo, PropertyManagerDataService::Instance().retrieve(
                                    PROPERTY_MANAGER_NAME));
  }

  void testFullCharNomMultiChar() {
    createLogWksp("60.", "1.4");
    auto tableWS = createTableWkspNOM();

    PDDetermineCharacterizations alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", m_logWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Characterizations", tableWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormRun", "1,  2"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BackRun", "3,4"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("NormBackRun", "5,6"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ReductionProperties", PROPERTY_MANAGER_NAME));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    auto expectedInfo = createExpectedInfo(
        60., 1.4, 1, "1,2", "5,6", "3,4", "0", ".31,.25,.13,.13,.13,.42",
        "13.66,5.83,3.93,2.09,1.57,31.42", 300.00, 16666.67, 0., 0.);

    compareResult(expectedInfo, PropertyManagerDataService::Instance().retrieve(
                                    PROPERTY_MANAGER_NAME));
  }

  void testNomWithWL() {
    createLogWksp("60.", "1.4");
    auto tableWS = createTableWkspNOM_withwl();

    PDDetermineCharacterizations alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", m_logWSName));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Characterizations", tableWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("ReductionProperties", PROPERTY_MANAGER_NAME));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    auto expectedInfo = createExpectedInfo(
        60., 1.4, 1, "0", "0", "0", "0", ".31,.25,.13,.13,.13,.42",
        "13.66,5.83,3.93,2.09,1.57,31.42", 300.00, 16666.67, .9, 2.1);

    compareResult(expectedInfo, PropertyManagerDataService::Instance().retrieve(
                                    PROPERTY_MANAGER_NAME));
  }
};

#endif /* MANTID_ALGORITHMS_PDDETERMINECHARACTERIZATIONS2TEST_H_ */
