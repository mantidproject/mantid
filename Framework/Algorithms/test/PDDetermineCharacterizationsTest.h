#ifndef MANTID_ALGORITHMS_PDDETERMINECHARACTERIZATIONSTEST_H_
#define MANTID_ALGORITHMS_PDDETERMINECHARACTERIZATIONSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/PDDetermineCharacterizations.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayProperty.h"
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
                     const std::string &wavelength) {
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
  }

  void addRow(ITableWorkspace_sptr wksp, double freq, double wl, int bank,
              std::string van, std::string can, std::string empty,
              std::string dmin, std::string dmax, double tofmin,
              double tofmax) {
    Mantid::API::TableRow row = wksp->appendRow();
    row << freq;
    row << wl;
    row << bank;
    row << van;
    row << can;
    row << empty;
    row << dmin;
    row << dmax;
    row << tofmin;
    row << tofmax;
  }

  ITableWorkspace_sptr createEmptyTableWksp() {
    ITableWorkspace_sptr wksp = WorkspaceFactory::Instance().createTable();
    wksp->addColumn("double", "frequency");
    wksp->addColumn("double", "wavelength");
    wksp->addColumn("int", "bank");
    wksp->addColumn("str", "vanadium");
    wksp->addColumn("str", "container");
    wksp->addColumn("str", "empty");
    wksp->addColumn("str", "d_min"); // b/c it is an array for NOMAD
    wksp->addColumn("str", "d_max"); // b/c it is an array for NOMAD
    wksp->addColumn("double", "tof_min");
    wksp->addColumn("double", "tof_max");

    return wksp;
  }

  ITableWorkspace_sptr createTableWkspPG3() {
    ITableWorkspace_sptr wksp = createEmptyTableWksp();

    addRow(wksp, 60., 0.533, 1, "17702", "17711", "0", "0.05", "2.20", 0000.00,
           16666.67);
    addRow(wksp, 60., 1.333, 3, "17703", "17712", "0", "0.43", "5.40", 12500.00,
           29166.67);
    addRow(wksp, 60., 2.665, 4, "17704", "17713", "0", "1.15", "9.20", 33333.33,
           50000.00);
    addRow(wksp, 60., 4.797, 5, "17705", "17714", "0", "2.00", "15.35",
           66666.67, 83333.67);

    return wksp;
  }

  ITableWorkspace_sptr createTableWkspNOM() {
    ITableWorkspace_sptr wksp = createEmptyTableWksp();

    addRow(wksp, 60., 1.4, 1, "0", "0", "0", ".31,.25,.13,.13,.13,.42",
           "13.66,5.83,3.93,2.09,1.57,31.42", 300.00, 16666.67);

    return wksp;
  }

  PropertyManager_sptr
  createExpectedInfo(const double freq, const double wl, const int bank,
                     const std::string &van, const std::string &can,
                     const std::string &empty, const std::string &dmin,
                     const std::string &dmax, const double tofmin,
                     const double tofmax) {

    PropertyManager_sptr expectedInfo = boost::make_shared<PropertyManager>();
    expectedInfo->declareProperty(
        make_unique<PropertyWithValue<double>>("frequency", freq));
    expectedInfo->declareProperty(
        make_unique<PropertyWithValue<double>>("wavelength", wl));
    expectedInfo->declareProperty(
        make_unique<PropertyWithValue<int>>("bank", bank));
    expectedInfo->declareProperty(
        Mantid::Kernel::make_unique<ArrayProperty<int32_t>>("vanadium", van));
    expectedInfo->declareProperty(
        Mantid::Kernel::make_unique<ArrayProperty<int32_t>>("container", can));
    expectedInfo->declareProperty(
        Mantid::Kernel::make_unique<ArrayProperty<int32_t>>("empty", empty));
    expectedInfo->declareProperty(
        Mantid::Kernel::make_unique<ArrayProperty<double>>("d_min", dmin));
    expectedInfo->declareProperty(
        Mantid::Kernel::make_unique<ArrayProperty<double>>("d_max", dmax));
    expectedInfo->declareProperty(
        make_unique<PropertyWithValue<double>>("tof_min", tofmin));
    expectedInfo->declareProperty(
        make_unique<PropertyWithValue<double>>("tof_max", tofmax));

    return expectedInfo;
  }

  void compareResult(PropertyManager_sptr expected,
                     PropertyManager_sptr observed) {
    TS_ASSERT_EQUALS(expected->propertyCount(), observed->propertyCount());

    const std::vector<Property *> &expectedProps = expected->getProperties();

    for (std::size_t i = 0; i < expectedProps.size(); ++i) {
      const std::string name = expectedProps[i]->name();
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

    auto expectedInfo =
        createExpectedInfo(0., 0., 1, "0", "0", "0", "", "", 0., 0.);

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

    auto expectedInfo =
        createExpectedInfo(0., 0., 1, "0", "0", "0", "", "", 0., 0.);

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

    auto expectedInfo = createExpectedInfo(60., 0.533, 1, "17702", "17711", "0",
                                           "0.05", "2.20", 0000.00, 16666.67);

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

    auto expectedInfo = createExpectedInfo(60., 0.533, 1, "0", "0", "0", "0.05",
                                           "2.20", 0000.00, 16666.67);

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
        60., 1.4, 1, "0", "0", "0", ".31,.25,.13,.13,.13,.42",
        "13.66,5.83,3.93,2.09,1.57,31.42", 300.00, 16666.67);

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
        60., 1.4, 1, "1,2", "3,4", "5,6", ".31,.25,.13,.13,.13,.42",
        "13.66,5.83,3.93,2.09,1.57,31.42", 300.00, 16666.67);

    compareResult(expectedInfo, PropertyManagerDataService::Instance().retrieve(
                                    PROPERTY_MANAGER_NAME));
  }
};

#endif /* MANTID_ALGORITHMS_PDDETERMINECHARACTERIZATIONS2TEST_H_ */
