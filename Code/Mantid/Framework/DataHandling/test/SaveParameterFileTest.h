#ifndef SAVEPARAMETERFILETEST_H_
#define SAVEPARAMETERFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidDataHandling/SaveParameterFile.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidKernel/Exception.h"
#include "MantidTestHelpers/ScopedFileHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class SaveParameterFileTest : public CxxTest::TestSuite
{
public:

  void testSavingParameters()
  {
    //First we want to load a workspace to work with.
    prepareWorkspace();

    //Now let's set some parameters
    setParam("nickel-holder", "testDouble1", 1.23);
    setParam("nickel-holder", "testDouble2", 1.00);
    setParam("nickel-holder", "testString1", "hello world");
    setParam("nickel-holder", "testString2", "unchanged");
    setParamByDetID(1301, "testDouble", 2.17);
		setFitParam("nickel-holder", "A", ", BackToBackExponential , S ,  ,  ,  ,  , sqrt(188.149*centre^4+6520.945*centre^2) , dSpacing , TOF , linear ; TOF ; TOF");

    //Create a temporary blank file for us to test with
    ScopedFileHelper::ScopedFile paramFile("", "__params.xml");

    //Save the parameters out to disk
    saveParams(paramFile.getFileName());

    //Change some parameters - these changes should not have an effect
		setParam("nickel-holder", "testDouble1", 3.14);
		setParam("nickel-holder", "testString1", "broken");
		setParamByDetID(1301, "testDouble", 7.89);
		setFitParam("nickel-holder", "B", "someString");

    //Load the saved parameters back in
    loadParams(paramFile.getFileName());

    //Confirm all the parameters are as they should be
    checkParam("nickel-holder", "testDouble1", 1.23);
    checkParam("nickel-holder", "testDouble2", 1.00);
    checkParam("nickel-holder", "testString1", "hello world");
    checkParam("nickel-holder", "testString2", "unchanged");
    checkParamByDetID(1301, "testDouble", 2.17);
		checkFitParam("nickel-holder", "A", ", BackToBackExponential , S ,  ,  ,  ,  , sqrt(188.149*centre^4+6520.945*centre^2) , dSpacing , TOF , linear ; TOF ; TOF");
  }

  void setParam(std::string cName, std::string pName, std::string value)
  {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap& paramMap = m_ws->instrumentParameters();
    boost::shared_ptr<const IComponent> comp = inst->getComponentByName(cName);
    paramMap.addString(comp->getComponentID(), pName, value);
  }

  void setParam(std::string cName, std::string pName, double value)
  {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap& paramMap = m_ws->instrumentParameters();
    boost::shared_ptr<const IComponent> comp = inst->getComponentByName(cName);
    paramMap.addDouble(comp->getComponentID(), pName, value);
  }

  void setParamByDetID(int id, std::string pName, double value)
  {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap& paramMap = m_ws->instrumentParameters();
    IDetector_const_sptr det = inst->getDetector(id);
    IComponent_const_sptr comp = boost::dynamic_pointer_cast<const IComponent>(det);
    paramMap.addDouble(comp->getComponentID(), pName, value);
  }

	void setFitParam(std::string cName, std::string pName, std::string value)
	{
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap& paramMap = m_ws->instrumentParameters();
    boost::shared_ptr<const IComponent> comp = inst->getComponentByName(cName);
		auto param = ParameterFactory::create("fitting",pName);
		param->fromString(value);
		paramMap.add(comp.get(),param);
	}

  void checkParam(std::string cName, std::string pName, std::string value)
  {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap& paramMap = m_ws->instrumentParameters();
    boost::shared_ptr<const IComponent> comp = inst->getComponentByName(cName);
    std::string param = paramMap.getString(comp.get(), pName);
    TS_ASSERT_EQUALS(value, param);
  }

  void checkParam(std::string cName, std::string pName, double value)
  {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap& paramMap = m_ws->instrumentParameters();
    boost::shared_ptr<const IComponent> comp = inst->getComponentByName(cName);
    std::vector<double> values = paramMap.getDouble(cName, pName);
    TS_ASSERT_DELTA(value, values.front(), 0.0001);
  }

  void checkParamByDetID(int id, std::string pName, double value)
  {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap& paramMap = m_ws->instrumentParameters();
    IDetector_const_sptr det = inst->getDetector(id);
    IComponent_const_sptr comp = boost::dynamic_pointer_cast<const IComponent>(det);
    Parameter_sptr param = paramMap.get(comp.get(), pName);
    double pValue = param->value<double>();
    TS_ASSERT_DELTA(value, pValue, 0.0001);
  }

  void checkFitParam(std::string cName, std::string pName, std::string value)
  {
    Instrument_const_sptr inst = m_ws->getInstrument();
    ParameterMap& paramMap = m_ws->instrumentParameters();
    boost::shared_ptr<const IComponent> comp = inst->getComponentByName(cName);
		auto param = paramMap.get(comp.get(),pName,"fitting");
		const Mantid::Geometry::FitParameter& fitParam = param->value<FitParameter>();

		// Info about fitting parameter is in string value, see FitParameter class
		typedef Poco::StringTokenizer tokenizer;
		tokenizer values(value, ",", tokenizer::TOK_TRIM);
		TS_ASSERT_EQUALS(fitParam.getFormula(), values[7]);
		TS_ASSERT_EQUALS(fitParam.getFunction(), values[1]);
		TS_ASSERT_EQUALS(fitParam.getResultUnit(), values[9]);
		TS_ASSERT_EQUALS(fitParam.getFormulaUnit(), values[8]);
  }

  void loadParams(std::string filename)
  {
    LoadParameterFile loaderPF;
    TS_ASSERT_THROWS_NOTHING(loaderPF.initialize());
    loaderPF.setPropertyValue("Filename", filename);
    loaderPF.setPropertyValue("Workspace", m_ws->name());
    TS_ASSERT_THROWS_NOTHING(loaderPF.execute());
    TS_ASSERT(loaderPF.isExecuted());
  }

  void saveParams(std::string filename)
  {
    SaveParameterFile saverPF;
    TS_ASSERT_THROWS_NOTHING(saverPF.initialize());
    saverPF.setPropertyValue("Filename", filename);
    saverPF.setPropertyValue("Workspace", m_ws->name());
    TS_ASSERT_THROWS_NOTHING(saverPF.execute());
    TS_ASSERT(saverPF.isExecuted());
  }

  void prepareWorkspace()
  {
    LoadInstrument loaderIDF2;

    TS_ASSERT_THROWS_NOTHING(loaderIDF2.initialize());

    std::string wsName = "SaveParameterFileTestIDF2";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    loaderIDF2.setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING2.xml");
    loaderIDF2.setPropertyValue("Workspace", wsName);
    TS_ASSERT_THROWS_NOTHING(loaderIDF2.execute());
    TS_ASSERT( loaderIDF2.isExecuted() );

    m_ws = boost::dynamic_pointer_cast<MatrixWorkspace>(ws2D);
  }

private:
  MatrixWorkspace_sptr m_ws;

};

#endif /*SAVEPARAMETERFILETEST_H_*/
