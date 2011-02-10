#ifndef IQTRANSFORMTEST_H_
#define IQTRANSFORMTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAlgorithms/IQTransform.h"
#include "MantidKernel/UnitFactory.h"

class IQTransformTest : public CxxTest::TestSuite
{
public:
  static IQTransformTest *createSuite() { return new IQTransformTest(); }
  static void destroySuite(IQTransformTest *suite) { delete suite; }

  IQTransformTest()
  {
    iq.setChild(true); // This means the ADS is not involved anywhere in this test

    inWS_hist = WorkspaceCreationHelper::Create2DWorkspaceBinned(1,2);
    inWS_hist->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("MomentumTransfer");
    inWS_hist->isDistribution(true);

    inWS_point = WorkspaceCreationHelper::Create2DWorkspace154(1,1);
    inWS_point->dataX(0)[0] = 3.0;  // 1 is not a good number to test with
    inWS_point->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("MomentumTransfer");
    inWS_point->isDistribution(true);
  }

  ~IQTransformTest()
  {
    // Tidy up
    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void testBasics()
  {
    TS_ASSERT_EQUALS( iq.name(), "IQTransform" );
    TS_ASSERT_EQUALS( iq.version(), 1 );
    TS_ASSERT_EQUALS( iq.category(), "SANS" );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( iq.initialize() );
    TS_ASSERT( iq.isInitialized() );

    // Set this just once to keep the validation happy
    TS_ASSERT_THROWS_NOTHING( iq.setPropertyValue("OutputWorkspace","dummy") );
  }

  void testGuinierSpheres()
  {
    TS_ASSERT_THROWS_NOTHING( iq.setProperty("InputWorkspace",inWS_hist) );
    TS_ASSERT_THROWS_NOTHING( iq.setPropertyValue("TransformType","Guinier (spheres)") );
    TS_ASSERT( iq.execute() );

    Mantid::API::MatrixWorkspace_const_sptr outWS = iq.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA( outWS->readX(0)[0], 0.25, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readY(0)[0], 0.693147, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readE(0)[0], 0.707107, 1.0e-6 );

    TS_ASSERT_EQUALS( outWS->YUnitLabel(), "Ln(I)" );
    TS_ASSERT_EQUALS( outWS->getAxis(0)->unit()->caption(), "Q^2");
  }

  void testGuinierRods()
  {
    TS_ASSERT_THROWS_NOTHING( iq.setProperty("InputWorkspace",inWS_point) );
    TS_ASSERT_THROWS_NOTHING( iq.setPropertyValue("TransformType","Guinier (rods)") );
    TS_ASSERT( iq.execute() );

    Mantid::API::MatrixWorkspace_const_sptr outWS = iq.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA( outWS->readX(0)[0], 9.0, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readY(0)[0], 2.708050, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readE(0)[0], 0.8, 1.0e-6 );

    TS_ASSERT_EQUALS( outWS->YUnitLabel(), "Ln(I x Q)" );
    TS_ASSERT_EQUALS( outWS->getAxis(0)->unit()->caption(), "Q^2");
}

  void testGuinierSheets()
  {
    TS_ASSERT_THROWS_NOTHING( iq.setProperty("InputWorkspace",inWS_hist) );
    TS_ASSERT_THROWS_NOTHING( iq.setPropertyValue("TransformType","Guinier (sheets)") );
    TS_ASSERT( iq.execute() );

    Mantid::API::MatrixWorkspace_const_sptr outWS = iq.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA( outWS->readX(0)[0], 0.25, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readY(0)[0], -0.693147, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readE(0)[0], 0.707107, 1.0e-6 );

    TS_ASSERT_EQUALS( outWS->YUnitLabel(), "Ln(I x Q^2)" );
    TS_ASSERT_EQUALS( outWS->getAxis(0)->unit()->caption(), "Q^2");
  }

  void testZimm()
  {
    TS_ASSERT_THROWS_NOTHING( iq.setProperty("InputWorkspace",inWS_point) );
    TS_ASSERT_THROWS_NOTHING( iq.setPropertyValue("TransformType","Zimm") );
    TS_ASSERT( iq.execute() );

    Mantid::API::MatrixWorkspace_const_sptr outWS = iq.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA( outWS->readX(0)[0], 9.0, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readY(0)[0], 0.2, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readE(0)[0], 0.16, 1.0e-6 );

    TS_ASSERT_EQUALS( outWS->YUnitLabel(), "1/I" );
    TS_ASSERT_EQUALS( outWS->getAxis(0)->unit()->caption(), "Q^2");
  }

  void testDebyeBueche()
  {
    TS_ASSERT_THROWS_NOTHING( iq.setProperty("InputWorkspace",inWS_hist) );
    TS_ASSERT_THROWS_NOTHING( iq.setPropertyValue("TransformType","Debye-Bueche") );
    TS_ASSERT( iq.execute() );

    Mantid::API::MatrixWorkspace_const_sptr outWS = iq.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA( outWS->readX(0)[0], 0.25, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readY(0)[0], 0.707107, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readE(0)[0], 0.5, 1.0e-6 );

    TS_ASSERT_EQUALS( outWS->YUnitLabel(), "1/sqrt(I)" );
    TS_ASSERT_EQUALS( outWS->getAxis(0)->unit()->caption(), "Q^2");
  }

  void testHoltzer()
  {
    TS_ASSERT_THROWS_NOTHING( iq.setProperty("InputWorkspace",inWS_point) );
    TS_ASSERT_THROWS_NOTHING( iq.setPropertyValue("TransformType","Holtzer") );
    TS_ASSERT( iq.execute() );

    Mantid::API::MatrixWorkspace_const_sptr outWS = iq.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA( outWS->readX(0)[0], 3.0, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readY(0)[0], 15.0, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readE(0)[0], 12.0, 1.0e-6 );

    TS_ASSERT_EQUALS( outWS->YUnitLabel(), "I x Q" );
    TS_ASSERT_EQUALS( outWS->getAxis(0)->unit()->caption(), "q");
  }

  void testKratky()
  {
    TS_ASSERT_THROWS_NOTHING( iq.setProperty("InputWorkspace",inWS_hist) );
    TS_ASSERT_THROWS_NOTHING( iq.setPropertyValue("TransformType","Kratky") );
    TS_ASSERT( iq.execute() );

    Mantid::API::MatrixWorkspace_const_sptr outWS = iq.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA( outWS->readX(0)[0], 0.5, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readY(0)[0], 0.5, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readE(0)[0], 0.353553, 1.0e-6 );

    TS_ASSERT_EQUALS( outWS->YUnitLabel(), "I x Q^2" );
    TS_ASSERT_EQUALS( outWS->getAxis(0)->unit()->caption(), "q");
  }

  void testPorod()
  {
    TS_ASSERT_THROWS_NOTHING( iq.setProperty("InputWorkspace",inWS_point) );
    TS_ASSERT_THROWS_NOTHING( iq.setPropertyValue("TransformType","Porod") );
    TS_ASSERT( iq.execute() );

    Mantid::API::MatrixWorkspace_const_sptr outWS = iq.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA( outWS->readX(0)[0], 3.0, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readY(0)[0], 405.0, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readE(0)[0], 324.0, 1.0e-6 );

    TS_ASSERT_EQUALS( outWS->YUnitLabel(), "I x Q^4" );
    TS_ASSERT_EQUALS( outWS->getAxis(0)->unit()->caption(), "q");
  }

  void testLogLog()
  {
    TS_ASSERT_THROWS_NOTHING( iq.setProperty("InputWorkspace",inWS_hist) );
    TS_ASSERT_THROWS_NOTHING( iq.setPropertyValue("TransformType","Log-Log") );
    TS_ASSERT( iq.execute() );

    Mantid::API::MatrixWorkspace_const_sptr outWS = iq.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA( outWS->readX(0)[0], -0.693147, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readY(0)[0], 0.693147, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readE(0)[0], 0.707107, 1.0e-6 );

    TS_ASSERT_EQUALS( outWS->YUnitLabel(), "Ln(I)" );
    TS_ASSERT_EQUALS( outWS->getAxis(0)->unit()->caption(), "Ln(Q)");
  }

  void testGeneral()
  {
    TS_ASSERT_THROWS_NOTHING( iq.setProperty("InputWorkspace",inWS_point) );
    TS_ASSERT_THROWS_NOTHING( iq.setPropertyValue("TransformType","General") );

    // Check it fails if constants not set
    TS_ASSERT_THROWS( iq.execute(), std::invalid_argument );

    std::vector<double> constants(10,2.0);
    TS_ASSERT_THROWS_NOTHING( iq.setProperty("GeneralFunctionConstants",constants) );
    TS_ASSERT( iq.execute() );

    Mantid::API::MatrixWorkspace_const_sptr outWS = iq.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA( outWS->readX(0)[0], 1374.580706, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readY(0)[0], 1374.580706, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readE(0)[0], 2559.329130, 1.0e-6 );

    TS_ASSERT_EQUALS( outWS->YUnitLabel(), "Q^2 x I^2 x Ln( Q^2 x I^2 x 2)" );
    TS_ASSERT_EQUALS( outWS->getAxis(0)->unit()->caption(), "Q^2 x I^2 x Ln( Q^2 x I^2 x 2)");
  }

  void testConstantBackground()
  {
    TS_ASSERT_THROWS_NOTHING( iq.setProperty("InputWorkspace",inWS_hist) );
    TS_ASSERT_THROWS_NOTHING( iq.setPropertyValue("TransformType","Holtzer") );
    TS_ASSERT_THROWS_NOTHING( iq.setProperty("BackgroundValue", 1.5) );
    TS_ASSERT( iq.execute() );

    Mantid::API::MatrixWorkspace_const_sptr outWS = iq.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA( outWS->readX(0)[0], 0.5, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readY(0)[0], 0.25, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readE(0)[0], 0.707107, 1.0e-6 );
  }

  void testWorkspaceBackground()
  {
    TS_ASSERT_THROWS_NOTHING( iq.setProperty("InputWorkspace",inWS_point) );
    TS_ASSERT_THROWS_NOTHING( iq.setPropertyValue("TransformType","Holtzer") );
    TS_ASSERT_THROWS_NOTHING( iq.setProperty<Mantid::API::MatrixWorkspace_sptr>("BackgroundWorkspace",WorkspaceCreationHelper::Create2DWorkspace123(1,1)) );
    TS_ASSERT( iq.execute() );

    // Remember that a constant value of 1.5 will also be subtracted because this property
    // was set in the previous test
    Mantid::API::MatrixWorkspace_const_sptr outWS = iq.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA( outWS->readX(0)[0], 3.0, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readY(0)[0], 4.5, 1.0e-6 );
    TS_ASSERT_DELTA( outWS->readE(0)[0], 15.0, 1.0e-6 );
  }

private:
  Mantid::Algorithms::IQTransform iq;
  Mantid::API::MatrixWorkspace_sptr inWS_hist;
  Mantid::API::MatrixWorkspace_sptr inWS_point;
};

#endif /*IQTRANSFORMTEST_H_*/
