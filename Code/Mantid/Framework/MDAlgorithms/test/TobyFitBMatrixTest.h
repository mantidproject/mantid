#ifndef MANTID_MDALGORITHMS_TOBYFITBMATRIXTEST_H_
#define MANTID_MDALGORITHMS_TOBYFITBMATRIXTEST_H_

#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitBMatrix.h"
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitYVector.h"
#include "MantidMDAlgorithms/Quantification/Resolution/TobyFitResolutionModel.h"
#include "MantidMDAlgorithms/Quantification/CachedExperimentInfo.h"

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FermiChopperModel.h"
#include "MantidAPI/IkedaCarpenterModerator.h"

#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <boost/shared_ptr.hpp>
#include <cxxtest/TestSuite.h>

class TobyFitBMatrixTest : public CxxTest::TestSuite
{
public:

  void test_Object_Construction_Does_Not_Throw()
  {
    using namespace Mantid::MDAlgorithms;
    TS_ASSERT_THROWS_NOTHING(TobyFitBMatrix());
  }

  void test_Number_Of_Cols_Equals_Number_Of_Rows_In_YVector()
  {
    using namespace Mantid::MDAlgorithms;
    TobyFitBMatrix bMatrix;

    TS_ASSERT_EQUALS(bMatrix.numCols(), TobyFitYVector::length());
  }

  void test_Number_Of_Rows_Equals_Six()
  {
    using namespace Mantid::MDAlgorithms;
    TobyFitBMatrix bMatrix;

    TS_ASSERT_EQUALS(bMatrix.numRows(), (int)NUM_ROWS);
  }

  void test_Values_Are_As_Expected_For_Test_Setup()
  {
    using namespace Mantid::MDAlgorithms;
    boost::shared_ptr<CachedExperimentInfo> observation = createTestCachedExperimentInfo();
    const double deltaE = 195.0;
    QOmegaPoint qOmega(0.0,0.0,0.0,deltaE);

    TobyFitBMatrix bMatrix;
    bMatrix.recalculate(*observation, qOmega);

    // std::cerr << "\n\n";
    // std::cerr << std::setiosflags(std::ios_base::fixed) << std::setprecision(8);
    // for(unsigned int i = 0; i < bMatrix.numRows(); ++i)
    // {
    //   for(unsigned int j = 0; j < bMatrix.numCols(); ++j)
    //   {
    //     std::cerr << bMatrix[i][j] << " ";
    //   }
    //   std::cerr << "\n";
    // }
    // std::cerr << "\n";

    double expected[NUM_ROWS][NUM_COLS] =
    {
      {13447.77443282, 0.73296352, 0.0, -13447.77443282, 0.17571130, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      {0.0,-1.46727577, 0.0, 0.0, 1.46727577, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      {0.0, 0.0, -1.46727577, 0.0, 0.0, 0.0, -1.46727577, 0.0, 0.0, 0.0, 0.0},
      {-2387.21606587, 0.20616089, 0.0, 15077.15410023, 0.21296949, 2.28537259, -1.47292732, 1.82762473, 0.0, 0.0, -12689.93803436},
      {0.0, 0.0, 0.0, 0.0, -0.33477735, 1.58316802, 0.84953829, 0.0, 0.0, 1.82762473, 0.0},
      {0.0, 0.0, 0.0, 0.0, -1.70036178, 0.0, -0.67006116, 0.0, 1.82762473, 0.0, 0.0},
    };

    for(unsigned int i = 0; i < bMatrix.numRows(); ++i)
    {
      for(unsigned int j = 0; j < bMatrix.numCols(); ++j)
      {
        std::ostringstream os;
        os << "Element " << "(" << i << "," << j << ") mismatch.";
        TSM_ASSERT_DELTA(os.str(), expected[i][j], bMatrix[i][j], 1e-8);
      }
    }

  }

private:

  boost::shared_ptr<Mantid::MDAlgorithms::CachedExperimentInfo>
  createTestCachedExperimentInfo()
  {
    using namespace Mantid::API;
    using namespace Mantid::MDAlgorithms;
    m_expt = createTestExperiment();

    return boost::make_shared<CachedExperimentInfo>(*m_expt, (Mantid::detid_t)TEST_DET_ID);
  }

  Mantid::API::ExperimentInfo_const_sptr createTestExperiment()
  {
    using namespace Mantid::API;
    using namespace Mantid::Geometry;

    ExperimentInfo_sptr expt = boost::make_shared<ExperimentInfo>();
    Instrument_sptr testInst = createTestInstrument();
    expt->setInstrument(testInst);

    expt->mutableRun().addProperty<std::string>("deltaE-mode", "direct");
    const double ei = 447.0;
    expt->mutableRun().addProperty<double>("Ei", ei);

    // Chopper
    FermiChopperModel *chopper = new FermiChopperModel;
    chopper->setAngularVelocityInHz(600.0);
    chopper->setChopperRadius(49.0/1000.);
    chopper->setSlitRadius(1300./1000.);
    chopper->setSlitThickness(2.28/1000.);
    chopper->setIncidentEnergy(ei);

    expt->setChopperModel(chopper, 0);

    // Moderator
    IkedaCarpenterModerator *sourceDescr = new IkedaCarpenterModerator;
    sourceDescr->setTiltAngleInDegrees(0.5585*180.0/M_PI);
    expt->setModeratorModel(sourceDescr);

    // OrientedLattice
    OrientedLattice * latticeRotation = new OrientedLattice;
    expt->mutableSample().setOrientedLattice(latticeRotation);
    delete latticeRotation;
    return expt;
  }

  Mantid::Geometry::Instrument_sptr createTestInstrument()
  {
    using namespace Mantid::Geometry;
    using Mantid::Kernel::V3D;

    Instrument_sptr instrument = boost::make_shared<Instrument>();
    const PointingAlong beamDir = Mantid::Geometry::Z;
    const PointingAlong upDir = Mantid::Geometry::Y;
    boost::shared_ptr<ReferenceFrame> reference =
        boost::make_shared<ReferenceFrame>(upDir, beamDir, Mantid::Geometry::Right, "frame");

    instrument->setReferenceFrame(reference);

    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0.0,0.0, -12.0));
    instrument->add(source);
    instrument->markAsSource(source);

    ObjComponent *aperture = new ObjComponent("aperture");
    aperture->setPos(V3D(0.0,0.0, -10.01));
    Object_sptr shape = ComponentCreationHelper::createCuboid(0.047,0.047,0.001);
    aperture->setShape(shape);
    instrument->add(aperture);

    ObjComponent *chopperPos = new ObjComponent("chopperPos");
    chopperPos->setPos(V3D(0.0,0.0,-1.9));
    instrument->add(chopperPos);
    instrument->markAsChopperPoint(chopperPos);

    ObjComponent *sample = new ObjComponent("samplePos");
    sample->setPos(V3D());
    instrument->add(sample);
    instrument->markAsSamplePos(sample);

    Detector *det1 = new Detector("det1", TEST_DET_ID, instrument.get());
    V3D detPos;
    detPos.spherical_rad(6.0340, 0.37538367018968838, 2.618430210304493);
    det1->setPos(detPos);
    shape = ComponentCreationHelper::createCappedCylinder(0.012, 0.01, detPos,V3D(0,1,0),"cyl");
    det1->setShape(shape);

    instrument->add(det1);
    instrument->markAsDetector(det1);

    return instrument;
  }
  /// Test ID value
  enum { TEST_DET_ID = 1 };
  /// Num of rows expected
  enum { NUM_ROWS = 6 };
  /// Num of columns expected
  enum { NUM_COLS = 11 };
  /// Test experiment
  Mantid::API::ExperimentInfo_const_sptr m_expt;
};

#endif //MANTID_MDALGORITHMS_TOBYFITBMATRIXTEST_H_
