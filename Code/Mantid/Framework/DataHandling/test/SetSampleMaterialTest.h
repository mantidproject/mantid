#ifndef SetSampleMaterialTEST_H_
#define SetSampleMaterialTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SetSampleMaterial.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;
using namespace Mantid::PhysicalConstants;

using Mantid::API::MatrixWorkspace_sptr;

class SetSampleMaterialTest : public CxxTest::TestSuite
{
public:
  void testName()
  {
    IAlgorithm* setmat = Mantid::API::FrameworkManager::Instance().createAlgorithm("SetSampleMaterial");
    TS_ASSERT_EQUALS( setmat->name(), "SetSampleMaterial" );
  }

  void testVersion()
  {
    IAlgorithm* setmat = Mantid::API::FrameworkManager::Instance().createAlgorithm("SetSampleMaterial");
    TS_ASSERT_EQUALS( setmat->version(), 1 );
  }

  void testInit()
  {
    IAlgorithm* setmat = Mantid::API::FrameworkManager::Instance().createAlgorithm("SetSampleMaterial");
    TS_ASSERT_THROWS_NOTHING( setmat->initialize() );
    TS_ASSERT( setmat->isInitialized() );
  }

  void testExec()
  {
	  std::string wsName = "SetSampleMaterialTestWS";
    IAlgorithm* setmat = Mantid::API::FrameworkManager::Instance().createAlgorithm("SetSampleMaterial");
    if ( !setmat->isInitialized() ) setmat->initialize();

    // Create a small test workspace
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
	
    // Register the workspace in the data service
    AnalysisDataService::Instance().add(wsName, testWS);

	  TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("InputWorkspace", wsName) );
    TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("ChemicalFormula","Al2-O3") );
    TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("SampleNumberDensity","0.0236649") );
    TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("ScatteringXSection","15.7048") );
    TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("AttenuationXSection","0.46257") );
    TS_ASSERT_THROWS_NOTHING( setmat->execute() );
    TS_ASSERT( setmat->isExecuted() );

    // check some of the output properties
    double value = setmat->getProperty("bAverage");
    TS_ASSERT_DELTA(value, 1.8503, 0.0001);
    value = setmat->getProperty("bSquaredAverage");
    TS_ASSERT_DELTA(value, 9.1140, 0.0001);
    value = setmat->getProperty("NormalizedLaue");
    TS_ASSERT_DELTA(value, 1.6618, 0.0001);

	//can get away with holding pointer as it is an inout ws property
    const Material *m_sampleMaterial = &(testWS->sample().getMaterial());
    TS_ASSERT_DELTA( m_sampleMaterial->numberDensity(), 0.0236649, 0.0001 );
    TS_ASSERT_DELTA( m_sampleMaterial->totalScatterXSection(NeutronAtom::ReferenceLambda), 15.7048, 0.0001);
    TS_ASSERT_DELTA( m_sampleMaterial->absorbXSection(NeutronAtom::ReferenceLambda), 0.46257, 0.0001);

    checkOutputProperties(setmat,m_sampleMaterial);

	  AnalysisDataService::Instance().remove(wsName);

  }
  void testExecMat_Formula()
  {
	  
	std::string wsName = "SetSampleMaterialTestWS_formula";
    IAlgorithm* setmat = Mantid::API::FrameworkManager::Instance().createAlgorithm("SetSampleMaterial");
    if ( !setmat->isInitialized() ) setmat->initialize();

    // Create a small test workspace
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
	
    // Register the workspace in the data service
    AnalysisDataService::Instance().add(wsName, testWS);
	
	TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("InputWorkspace", wsName) );
    TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("ChemicalFormula","Al2-O3") );
    TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("UnitCellVolume","253.54") );
    TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("ZParameter","6") );
    TS_ASSERT_THROWS_NOTHING( setmat->execute() );
    TS_ASSERT( setmat->isExecuted() );

    const Material *m_sampleMaterial = &(testWS->sample().getMaterial());
    TS_ASSERT_DELTA( m_sampleMaterial->numberDensity(), 0.0236649, 0.0001 );
    TS_ASSERT_DELTA( m_sampleMaterial->totalScatterXSection(NeutronAtom::ReferenceLambda), 3.1404, 0.0001);
    TS_ASSERT_DELTA( m_sampleMaterial->absorbXSection(NeutronAtom::ReferenceLambda), 0.0925, 0.0001);
	  
    checkOutputProperties(setmat,m_sampleMaterial);

	AnalysisDataService::Instance().remove(wsName);
  }
  void testExecMat_OneAtom()
  {

	std::string wsName = "SetSampleMaterialTestWS_oneatom";
    IAlgorithm* setmat = Mantid::API::FrameworkManager::Instance().createAlgorithm("SetSampleMaterial");
    if ( !setmat->isInitialized() ) setmat->initialize();

    // Create a small test workspace
    MatrixWorkspace_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 10);
    // Needs to have units of wavelength
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");

    // Register the workspace in the data service
    AnalysisDataService::Instance().add(wsName, testWS);

	TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("InputWorkspace", wsName) );
    TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("ChemicalFormula","Ni") );
    TS_ASSERT_THROWS_NOTHING( setmat->execute() );
    TS_ASSERT( setmat->isExecuted() );

    // check some of the output properties
    double value = setmat->getProperty("bAverage");
    TS_ASSERT_DELTA(value, 10.3, 0.0001);
    value = setmat->getProperty("bSquaredAverage");
    TS_ASSERT_DELTA(value, 106.09, 0.0001);
    value = setmat->getProperty("NormalizedLaue");
    TS_ASSERT_DELTA(value, 0., 0.0001);

    const Material *m_sampleMaterial = &(testWS->sample().getMaterial());
    TS_ASSERT_DELTA( m_sampleMaterial->numberDensity(), 0.0913375, 0.0001 );
    TS_ASSERT_DELTA( m_sampleMaterial->totalScatterXSection(NeutronAtom::ReferenceLambda), 18.5, 0.0001);
    TS_ASSERT_DELTA( m_sampleMaterial->absorbXSection(NeutronAtom::ReferenceLambda), 4.49, 0.0001);

    checkOutputProperties(setmat,m_sampleMaterial);

	AnalysisDataService::Instance().remove(wsName);
  }

  void checkOutputProperties(const IAlgorithm* alg,const Material *material)
  {
    //test output properties
    double testvalue = alg->getProperty("AbsorptionXSectionResult");
    TS_ASSERT_DELTA(material->absorbXSection(), testvalue, 0.0001);
    testvalue = alg->getProperty("CoherentXSectionResult");
    TS_ASSERT_DELTA(material->cohScatterXSection(), testvalue, 0.0001);
    testvalue = alg->getProperty("IncoherentXSectionResult");
    TS_ASSERT_DELTA(material->incohScatterXSection(),  testvalue, 0.0001);
    testvalue = alg->getProperty("TotalXSectionResult");
    TS_ASSERT_DELTA(material->totalScatterXSection(), testvalue, 0.0001);
    testvalue = alg->getProperty("SampleNumberDensityResult");
    TS_ASSERT_DELTA(material->numberDensity(), testvalue, 0.0001);
  }

};

#endif /*SetSampleMaterialTEST_H_*/
