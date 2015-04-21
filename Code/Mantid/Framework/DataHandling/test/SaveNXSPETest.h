#ifndef SAVENXSPETEST_H_
#define SAVENXSPETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SaveNXSPE.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataHandling/LoadInstrument.h"
#include <Poco/File.h>

#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/scoped_array.hpp>

#include <hdf5.h>
#include <hdf5_hl.h>

#include <limits>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using Mantid::Kernel::UnitFactory;
using Mantid::Geometry::ParameterMap;
using Mantid::Geometry::Instrument;
using Mantid::Geometry::IDetector_const_sptr;


static const double MASK_FLAG=-1e30;   // values here need to match what is in the SaveNXSPE.h file
static const double MASK_ERROR=0.0;

static const int NHIST = 3;
static const int THEMASKED = 2;
static const int DEFAU_Y = 2;

class SaveNXSPETest : public CxxTest::TestSuite
{
public:
  static SaveNXSPETest *createSuite() { return new SaveNXSPETest(); }
  static void destroySuite(SaveNXSPETest *suite) { delete suite; }

  void testName()
  {
    SaveNXSPE saver;
    TS_ASSERT_EQUALS( saver.name(), "SaveNXSPE" );
  }

  void testVersion()
  {
    SaveNXSPE saver;
    TS_ASSERT_EQUALS( saver.version(), 1 );
  }

  void testInit()
  {
    SaveNXSPE saver;
    TS_ASSERT_THROWS_NOTHING( saver.initialize() );
    TS_ASSERT( saver.isInitialized() );

    TS_ASSERT_EQUALS( static_cast<int>(saver.getProperties().size()), 6 );
  }

  void testExec()
  {
    // Create a small test workspace
    MatrixWorkspace_sptr input = makeWorkspace();

    SaveNXSPE saver;
    saver.initialize();
    saver.setChild(true);
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("InputWorkspace", input) );
    std::string outputFile("SaveNXSPETest_testEXEC.nxspe");
    TS_ASSERT_THROWS_NOTHING( saver.setPropertyValue("Filename",outputFile) );
    outputFile = saver.getPropertyValue("Filename");//get absolute path

    TS_ASSERT_THROWS_NOTHING( saver.setProperty("Efixed", 0.0));
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("Psi", 0.0));
    TS_ASSERT_THROWS_NOTHING( saver.setProperty("KiOverKfScaling", true));
    TS_ASSERT_THROWS_NOTHING( saver.execute() );
    TS_ASSERT( saver.isExecuted() );

    TS_ASSERT( Poco::File(outputFile).exists() );
    if( !Poco::File(outputFile).exists() ) return;

    auto h5file = H5Fopen(outputFile.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    char * dset = "/mantid_workspace/data/data";
    int rank(0);
    herr_t status = H5LTget_dataset_ndims(h5file, dset, &rank);
    TS_ASSERT_EQUALS(0, status);
    TS_ASSERT_EQUALS(2, rank);

    boost::scoped_array<hsize_t> dims(new hsize_t[rank]);
    H5T_class_t classId(H5T_NO_CLASS);
    size_t typeSize(0);
    status = H5LTget_dataset_info(h5file, dset, dims.get(), &classId, &typeSize);
    TS_ASSERT_EQUALS(0, status);
    TS_ASSERT_EQUALS(3, dims[0]);
    TS_ASSERT_EQUALS(H5T_FLOAT, classId);
    TS_ASSERT_EQUALS(8, typeSize);

    size_t bufferSize(dims[0]*dims[1]);
    boost::scoped_array<double> signal(new double[bufferSize]), error(new double[bufferSize]);
    status = H5LTread_dataset_double(h5file, dset, signal.get());
    TS_ASSERT_EQUALS(0, status);
    dset = "/mantid_workspace/data/error";
    status = H5LTread_dataset_double(h5file, dset, error.get());
    TS_ASSERT_EQUALS(0, status);
    H5Fclose(h5file);
    double tolerance(1e-08);
    // element 0,0
    TS_ASSERT_DELTA(0.0, signal[0], tolerance);
    TS_ASSERT_DELTA(0.0, error[0], tolerance);
    // element 0,9
    TS_ASSERT_DELTA(9.0, signal[9], tolerance);
    TS_ASSERT_DELTA(18.0, error[9], tolerance);
    // element 1,2 in 2D flat buffer
    TS_ASSERT(boost::math::isnan(signal[1*dims[1] + 2]));
    TS_ASSERT_DELTA(0.0, error[1*dims[1] + 2], tolerance);
    // final element
    TS_ASSERT_DELTA(29.0, signal[dims[0]*dims[1] - 1], tolerance);
    TS_ASSERT_DELTA(58.0, error[dims[0]*dims[1] - 1], tolerance);

    Poco::File(outputFile).remove();
  }
  void testExecWithParFile()
  {
     MatrixWorkspace_sptr input = makeWorkspace();

     SaveNXSPE saver;
     saver.initialize();
     saver.setChild(true);
     TS_ASSERT_THROWS_NOTHING( saver.setProperty("InputWorkspace", input) );
     TS_ASSERT_THROWS_NOTHING( saver.setProperty("ParFile","testParFile.par"));
     std::string outputFile("SaveNXSPETest_testExecWithParFile.nxspe");
     TS_ASSERT_THROWS_NOTHING( saver.setPropertyValue("Filename",outputFile) );
     outputFile = saver.getPropertyValue("Filename");//get absolute path

    // throws file not exist from ChildAlgorithm
      saver.setRethrows(true);
      TS_ASSERT_THROWS( saver.execute(),Mantid::Kernel::Exception::FileError);
      TS_ASSERT( Poco::File(outputFile).exists() );

      if( Poco::File(outputFile).exists() ) Poco::File(outputFile).remove();
  }

private:

  MatrixWorkspace_sptr makeWorkspace()
  {
    // all the Y values in this new workspace are set to DEFAU_Y, which currently = 2
    const int nx = 10;
    auto testWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(NHIST,nx,1.0);
    for(int i = 0; i < NHIST; ++i) {
      auto & outY = testWS->dataY(i);
      auto & outE = testWS->dataE(i);
      for(int j = 0; j < nx; ++j) {
        outY[j] = i*nx + j;
        outE[j] = outY[j]*2.0;
      }
    }
    return setUpWorkspace(testWS);
  }

  MatrixWorkspace_sptr setUpWorkspace(MatrixWorkspace_sptr inputWS)
  {
    inputWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("DeltaE");

    // the following is largely about associating detectors with the workspace
    for (int j = 0; j < NHIST; ++j)
    {
      // Just set the spectrum number to match the index
      inputWS->getSpectrum(j)->setSpectrumNo(j+1);
    }

    // Load the instrument data
    Mantid::DataHandling::LoadInstrument loader;
    loader.initialize();
    loader.setChild(true);
    // Path to test input file assumes Test directory checked out from SVN
    std::string inputFile = "INES_Definition.xml";
    loader.setPropertyValue("Filename", inputFile);
    loader.setProperty("Workspace", inputWS);
    loader.execute();

    // mask the detector
    ParameterMap* m_Pmap = &(inputWS->instrumentParameters());
    boost::shared_ptr<const Instrument> instru = inputWS->getInstrument();
    IDetector_const_sptr toMask = instru->getDetector(THEMASKED);
    TS_ASSERT(toMask);
    m_Pmap->addBool(toMask.get(), "masked", true);

    // required to get it passed the algorthms validator
    inputWS->isDistribution(true);

    return inputWS;
  }
};

#endif /*SAVENXSPETEST_H_*/
