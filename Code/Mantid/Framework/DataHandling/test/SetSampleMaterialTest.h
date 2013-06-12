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

    TS_ASSERT_THROWS_NOTHING( setmat->setProperty<MatrixWorkspace_sptr>("InputWorkspace", wsName) );
    TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("ChemicalFormula","Al2-O3") );
    TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("SampleNumberDensity","0.0236649") );
    TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("ScatteringXSection","15.7048") );
    TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("AttenuationXSection","0.46257") );
    TS_ASSERT_THROWS_NOTHING( setmat->execute() );
    TS_ASSERT( setmat->isExecuted() );
    
	//can get away with holding pointer as it is an inout ws property
    const Material *m_sampleMaterial = &(testWS->sample().getMaterial());
    TS_ASSERT_DELTA( m_sampleMaterial->numberDensity(), 0.0236649, 0.0001 );
    TS_ASSERT_DELTA( m_sampleMaterial->totalScatterXSection(NeutronAtom::ReferenceLambda), 15.7048, 0.0001);
    TS_ASSERT_DELTA( m_sampleMaterial->absorbXSection(NeutronAtom::ReferenceLambda), 0.46257, 0.0001);

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

    TS_ASSERT_THROWS_NOTHING( setmat->setProperty<MatrixWorkspace_sptr>("InputWorkspace", wsName) );
    TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("ChemicalFormula","Al2-O3") );
    TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("UnitCellVolume","253.54") );
    TS_ASSERT_THROWS_NOTHING( setmat->setPropertyValue("ZParameter","6") );
    TS_ASSERT_THROWS_NOTHING( setmat->execute() );
    TS_ASSERT( setmat->isExecuted() );

    const Material *m_sampleMaterial = &(testWS->sample().getMaterial());
    TS_ASSERT_DELTA( m_sampleMaterial->numberDensity(), 0.0236649, 0.0001 );
    TS_ASSERT_DELTA( m_sampleMaterial->totalScatterXSection(NeutronAtom::ReferenceLambda), 15.7048, 0.0001);
    TS_ASSERT_DELTA( m_sampleMaterial->absorbXSection(NeutronAtom::ReferenceLambda), 0.46257, 0.0001);
	
	AnalysisDataService::Instance().remove(wsName);
  }

};

#endif /*SetSampleMaterialTEST_H_*/
