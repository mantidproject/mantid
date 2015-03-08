#ifndef MANTID_MDEVENTS_BOXCONTROLLERSETTINGSALGORITHMTEST_H_
#define MANTID_MDEVENTS_BOXCONTROLLERSETTINGSALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FrameworkManager.h"
#include <iostream>
#include <iomanip>

#include "MantidMDAlgorithms/BoxControllerSettingsAlgorithm.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ScopedFileHelper.h"
#include <boost/format.hpp>

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;
using ScopedFileHelper::ScopedFile;

//------------------------------------------------------------------------------------------------
/** Concrete declaration of BoxControllerSettingsAlgorithm for testing */
class BoxControllerSettingsAlgorithmImpl : public BoxControllerSettingsAlgorithm
{
  // Make all the members public so I can test them.
  friend class BoxControllerSettingsAlgorithmTest;
public:
  virtual const std::string name() const { return "BoxControllerSettingsAlgorithmImpl";};
  virtual int version() const { return 1;};
  virtual const std::string category() const { return "Testing";}
  virtual const std::string summary() const { return "Summary of this test."; }
  void init() {}
  void exec() {}
};


class BoxControllerSettingsAlgorithmTest : public CxxTest::TestSuite
{

private:

  /**
  Helper function. Runs LoadParameterAlg, to get an instrument parameter definition from a file onto a workspace.
  */
  void apply_instrument_parameter_file_to_workspace(MatrixWorkspace_sptr ws, const ScopedFile& file)
  {
    // Load the Instrument Parameter file over the existing test workspace + instrument.
    using DataHandling::LoadParameterFile;
    LoadParameterFile loadParameterAlg;
    loadParameterAlg.setRethrows(true);
    loadParameterAlg.initialize();
    loadParameterAlg.setPropertyValue("Filename", file.getFileName());
    loadParameterAlg.setProperty("Workspace", ws);
    loadParameterAlg.execute();
  }

  MatrixWorkspace_sptr create_workspace_with_splitting_params(int splitThreshold, int splitInto, int maxRecursionDepth)
  {
    auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
    ws->initialize(1, 2, 1);
    ws->setInstrument(ComponentCreationHelper::createTestInstrumentRectangular(6, 1, 0));
    const std::string instrumentName = ws->getInstrument()->getName();

    // Create a parameter file, with a root equation that will apply to all detectors.
    const std::string parameterFileContents =  boost::str(boost::format(
      "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n\
       <parameter-file instrument = \"%1%\" date = \"2013-01-31T00:00:00\">\n\
          <component-link name=\"%1%\">\n\
           <parameter name=\"SplitThreshold\">\n\
               <value val=\"%2%\"/>\n\
           </parameter>\n\
           <parameter name=\"SplitInto\">\n\
               <value val=\"%3%\"/>\n\
           </parameter>\n\
           <parameter name=\"MaxRecursionDepth\">\n\
               <value val=\"%4%\"/>\n\
           </parameter>\n\
           </component-link>\n\
        </parameter-file>\n") % instrumentName % splitThreshold % splitInto % maxRecursionDepth);

    // Create a temporary Instrument Parameter file.
    ScopedFile file(parameterFileContents, instrumentName + "_Parameters.xml");

    // Apply parameter file to workspace.
    apply_instrument_parameter_file_to_workspace(ws, file);

    return ws;
  }

public:

  void test_defaultProps()
  {
    BoxControllerSettingsAlgorithmImpl alg;
    alg.initBoxControllerProps();
    BoxController_sptr bc(new BoxController(3));
    alg.setBoxController(bc);
    TS_ASSERT_EQUALS( bc->getSplitInto(0), 5 );
    TS_ASSERT_EQUALS( bc->getSplitThreshold(), 1000 );
    TS_ASSERT_EQUALS( bc->getMaxDepth(), 5 );
  }

  /** You can change the defaults given to the props */
  void test_initProps_otherDefaults()
  {
    BoxControllerSettingsAlgorithmImpl alg;
    alg.initBoxControllerProps("6", 1234, 34);
    BoxController_sptr bc(new BoxController(3));
    alg.setBoxController(bc);
    TS_ASSERT_EQUALS( bc->getSplitInto(0), 6 );
    TS_ASSERT_EQUALS( bc->getSplitThreshold(), 1234 );
    TS_ASSERT_EQUALS( bc->getMaxDepth(), 34 );
  }

  void doTest(BoxController_sptr bc,
      std::string SplitInto="", std::string SplitThreshold="", std::string MaxRecursionDepth="")
  {
    BoxControllerSettingsAlgorithmImpl alg;
    alg.initBoxControllerProps();
    if (!SplitInto.empty()) alg.setPropertyValue("SplitInto",SplitInto);
    if (!SplitThreshold.empty()) alg.setPropertyValue("SplitThreshold",SplitThreshold);
    if (!MaxRecursionDepth.empty()) alg.setPropertyValue("MaxRecursionDepth",MaxRecursionDepth);
    alg.setBoxController(bc);
  }

  void test_SplitInto()
  {
    BoxController_sptr bc(new BoxController(3));
    TSM_ASSERT_THROWS_ANYTHING("Too few parameters", doTest(bc, "5,5") );
    TSM_ASSERT_THROWS_ANYTHING("Too many parameters", doTest(bc, "1,2,3,4") );
    doTest(bc,"4");
    TS_ASSERT_EQUALS( bc->getSplitInto(2), 4 );
    doTest(bc,"7,6,5");
    TS_ASSERT_EQUALS( bc->getSplitInto(0), 7 );
    TS_ASSERT_EQUALS( bc->getSplitInto(1), 6 );
    TS_ASSERT_EQUALS( bc->getSplitInto(2), 5 );
  }

  void test_SplitThreshold()
  {
    BoxController_sptr bc(new BoxController(3));
    TSM_ASSERT_THROWS_ANYTHING("Negative threshold", doTest(bc, "", "-3") );
    doTest(bc,"", "1234");
    TS_ASSERT_EQUALS( bc->getSplitThreshold(), 1234 );
  }

  void test_MaxRecursionDepth()
  {
    BoxController_sptr bc(new BoxController(3));
    TSM_ASSERT_THROWS_ANYTHING("Negative MaxRecursionDepth", doTest(bc, "", "", "-1") );
    doTest(bc,"", "", "34");
    TS_ASSERT_EQUALS( bc->getMaxDepth(), 34 );
  }


  void test_take_instrument_parameters()
  {
    const int splitInto = 4;
    const int splitThreshold = 16;
    const int maxRecursionDepth = 5;

    // Workspace has instrument has parameters for all box splitting parameters.
    auto ws = create_workspace_with_splitting_params(splitThreshold, splitInto, maxRecursionDepth);

    BoxController_sptr bc(new BoxController(1));

    BoxControllerSettingsAlgorithmImpl alg;
    alg.initBoxControllerProps();
    /* Note, not properties are set, so the algorithm will have defaults set. and should therefore look to
    pick-up any available in the instrument parameters.*/
    alg.setBoxController(bc, ws->getInstrument());

    int actualSplitThreshold = alg.getProperty("SplitThreshold");
    TS_ASSERT_EQUALS(splitThreshold, actualSplitThreshold);

    std::vector<int> actualSplitInto = alg.getProperty("SplitInto");
    TS_ASSERT_EQUALS(bc->getNDims(), actualSplitInto.size());
    std::vector<int> expectedSplitInto(bc->getNDims(), splitInto);
    TS_ASSERT_EQUALS(expectedSplitInto, actualSplitInto);

    int actualMaxRecursionDepth = alg.getProperty("MaxRecursionDepth");
    TS_ASSERT_EQUALS(maxRecursionDepth, actualMaxRecursionDepth);
  }

  // Test that the user providied values for spliting have precedence.
  void test_ignore_instrument_parameters()
  {
    const int splitInto = 8;
    const int splitThreshold = 16;
    const int maxRecursionDepth = 5;

    // Workspace has instrument has parameters for all box splitting parameters.
    auto ws = create_workspace_with_splitting_params(splitThreshold, splitInto, maxRecursionDepth);

    BoxController_sptr bc(new BoxController(1));

    // Create splitting parameters that are not default and not the same as those on the instrument parameters.
    const std::vector<int> nonDefaultSplitInto = std::vector<int>(bc->getNDims(), splitInto+1);
    const int nonDefaultSplitThreshold = splitThreshold + 1;
    const int nonDefaultMaxRecursionDepth = maxRecursionDepth + 1;

    BoxControllerSettingsAlgorithmImpl alg;
    alg.initBoxControllerProps();
    alg.setProperty("SplitInto", nonDefaultSplitInto);
    alg.setProperty("SplitThreshold", nonDefaultSplitThreshold);
    alg.setProperty("MaxRecursionDepth", nonDefaultMaxRecursionDepth);
    alg.setBoxController(bc, ws->getInstrument());

    int actualSplitThreshold = alg.getProperty("SplitThreshold");
    TS_ASSERT_EQUALS(nonDefaultSplitThreshold, actualSplitThreshold);

    std::vector<int> actualSplitInto = alg.getProperty("SplitInto");
    TS_ASSERT_EQUALS(bc->getNDims(), actualSplitInto.size());
    TS_ASSERT_EQUALS(nonDefaultSplitInto, actualSplitInto);

    int actualMaxRecursionDepth = alg.getProperty("MaxRecursionDepth");
    TS_ASSERT_EQUALS(nonDefaultMaxRecursionDepth, actualMaxRecursionDepth);
  }

  void test_with_no_instrument_parameters()
  {
    // Create a workspace with an instrument, but no instrument parameters for box splitting.
    auto ws = boost::make_shared<Mantid::DataObjects::Workspace2D>();
    ws->initialize(1, 2, 1);
    ws->setInstrument(ComponentCreationHelper::createTestInstrumentRectangular(6, 1, 0));

    BoxController_sptr bc(new BoxController(1));

    BoxControllerSettingsAlgorithmImpl alg;
    alg.initBoxControllerProps();
    // Note that no properties are actually set. All properties should fall-back to their default values.
    alg.setRethrows(true);
    TSM_ASSERT_THROWS_NOTHING("Lack of specific instrument parameters should not cause algorithm to fail.", alg.setBoxController(bc, ws->getInstrument()));

    // Check that the properties are unaffected. Should just reflect the defaults.
    Mantid::Kernel::Property* p = alg.getProperty("SplitThreshold");
    TS_ASSERT(p->isDefault());
    p = alg.getProperty("SplitInto");
    TS_ASSERT(p->isDefault());
    p = alg.getProperty("MaxRecursionDepth");
    TS_ASSERT(p->isDefault());
  }

};


#endif /* MANTID_MDEVENTS_BOXCONTROLLERSETTINGSALGORITHMTEST_H_ */

