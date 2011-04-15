#ifndef FFTSMOOTH2TEST_H_
#define FFTSMOOTH2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/FFTSmooth2.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidNexus/LoadNexusProcessed.h"


using namespace Mantid::API;
using namespace Mantid::Kernel;

class FFTSmooth2Test : public CxxTest::TestSuite
{
public:
  void testName()
  {
    TS_ASSERT_EQUALS( fftsmooth2.name(), "FFTSmooth" )
  }
  
  void testVersion()
  {
    TS_ASSERT_EQUALS( fftsmooth2.version(), 2 )
  }
  
  void testCategory()
  {
    TS_ASSERT_EQUALS( fftsmooth2.category(), "General" )
  }
  
  void testInit()
  {
    Mantid::Algorithms::FFTSmooth2   fftsmooth2_b;
    TS_ASSERT_THROWS_NOTHING( fftsmooth2_b.initialize() );
    TS_ASSERT( fftsmooth2_b.isInitialized() );

    const std::vector<Property*> props = fftsmooth2_b.getProperties();
    TS_ASSERT_EQUALS( props.size(), 7 );

    TS_ASSERT_EQUALS( props[0]->name(), "InputWorkspace" );
    TS_ASSERT( props[0]->isDefault() );
    TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[0]) );

    TS_ASSERT_EQUALS( props[1]->name(), "OutputWorkspace" );
    TS_ASSERT( props[1]->isDefault() );
    TS_ASSERT( dynamic_cast<WorkspaceProperty<MatrixWorkspace>* >(props[1]) );

    TS_ASSERT_EQUALS( props[2]->name(), "WorkspaceIndex");
    TS_ASSERT( props[2]->isDefault() );
    TS_ASSERT( dynamic_cast<PropertyWithValue<int>* >(props[2]) );

    TS_ASSERT_EQUALS( props[3]->name(), "Filter");
    TS_ASSERT( props[3]->isDefault() );
    TS_ASSERT_EQUALS( props[3]->value(), "Zeroing" );
    TS_ASSERT( dynamic_cast<PropertyWithValue<std::string>* >(props[3]) );

    TS_ASSERT_EQUALS( props[4]->name(), "Params" );
    TS_ASSERT( props[4]->isDefault() );
    TS_ASSERT_EQUALS( props[4]->value(), "" );
    TS_ASSERT( dynamic_cast<PropertyWithValue<std::string>* >(props[4]) );
  }

  void testZeroing()
  {                                               // load input and "Gold" result workspaces
    Mantid::NeXus::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename","MultispectralTestData.nxs");
    loader.setProperty("OutputWorkspace","TestInputWS");
    loader.execute();

    loader.initialize();
    loader.setProperty("Filename","FFTSmooth2_Zeroing.nxs");
    loader.setProperty("OutputWorkspace","ZeroingGoldWS");
    loader.execute();
                                                 // create and execute the algorithm for "Zeroing"
    Mantid::Algorithms::FFTSmooth2   fftsmooth2_c;
    TS_ASSERT_THROWS_NOTHING( fftsmooth2_c.initialize() );
    TS_ASSERT( fftsmooth2_c.isInitialized() );

    TS_ASSERT_THROWS_NOTHING( fftsmooth2_c.setPropertyValue( "InputWorkspace","TestInputWS" ) );
    TS_ASSERT_THROWS_NOTHING( fftsmooth2_c.setPropertyValue( "OutputWorkspace","SmoothedWS" ) );
    TS_ASSERT_THROWS_NOTHING( fftsmooth2_c.setPropertyValue( "WorkspaceIndex", "0" ) );
    TS_ASSERT_THROWS_NOTHING( fftsmooth2_c.setPropertyValue( "Filter", "Zeroing" ) );
    TS_ASSERT_THROWS_NOTHING( fftsmooth2_c.setPropertyValue( "Params", "100" ) );
    TS_ASSERT_THROWS_NOTHING( fftsmooth2_c.execute() );
    TS_ASSERT( fftsmooth2_c.isExecuted() );

    MatrixWorkspace_sptr test_output_WS;
    MatrixWorkspace_sptr gold_output_WS;

    TS_ASSERT_THROWS_NOTHING( test_output_WS=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("SmoothedWS")) );
    TS_ASSERT( test_output_WS );

    TS_ASSERT_THROWS_NOTHING( gold_output_WS=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("ZeroingGoldWS")) );
    TS_ASSERT( gold_output_WS );

    TS_ASSERT_EQUALS( test_output_WS->size(), gold_output_WS->size() );

    for ( int i = 0; i < test_output_WS->size(); i++ )
    {
      TS_ASSERT_DELTA( test_output_WS->dataY(0)[i], gold_output_WS->dataY(0)[i], 0.00001 );
    }

    AnalysisDataService::Instance().remove("TestInputWS");
    AnalysisDataService::Instance().remove("SmoothedWS");
    AnalysisDataService::Instance().remove("ZeroingGoldWS");

//  std::cout<< "RUNNING FFTSmooth2 testZeroing() DONE!" << std::endl;
  }

  void testButterworth()
  {                                               // load input and "Gold" result workspaces
    Mantid::NeXus::LoadNexusProcessed loader;
    loader.initialize();
    loader.setProperty("Filename","MultispectralTestData.nxs");
    loader.setProperty("OutputWorkspace","TestInputWS");
    loader.execute();

    loader.initialize();
    loader.setProperty("Filename","FFTSmooth2_Butterworth.nxs");
    loader.setProperty("OutputWorkspace","ButterworthGoldWS");
    loader.execute();
                                                 // create and execute the algorithm for "Butterworth"
    Mantid::Algorithms::FFTSmooth2   fftsmooth2_c;
    TS_ASSERT_THROWS_NOTHING( fftsmooth2_c.initialize() );
    TS_ASSERT( fftsmooth2_c.isInitialized() );

    TS_ASSERT_THROWS_NOTHING( fftsmooth2_c.setPropertyValue( "InputWorkspace","TestInputWS" ) );
    TS_ASSERT_THROWS_NOTHING( fftsmooth2_c.setPropertyValue( "OutputWorkspace","SmoothedWS" ) );
    TS_ASSERT_THROWS_NOTHING( fftsmooth2_c.setPropertyValue( "WorkspaceIndex", "0" ) );
    TS_ASSERT_THROWS_NOTHING( fftsmooth2_c.setPropertyValue( "Filter", "Butterworth" ) );
    TS_ASSERT_THROWS_NOTHING( fftsmooth2_c.setPropertyValue( "Params", "100,2" ) );
    TS_ASSERT_THROWS_NOTHING( fftsmooth2_c.execute() );
    TS_ASSERT( fftsmooth2_c.isExecuted() );

    MatrixWorkspace_sptr test_output_WS;
    MatrixWorkspace_sptr gold_output_WS;

    TS_ASSERT_THROWS_NOTHING( test_output_WS=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("SmoothedWS")) );
    TS_ASSERT( test_output_WS );

    TS_ASSERT_THROWS_NOTHING( gold_output_WS=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("ButterworthGoldWS")) );
    TS_ASSERT( gold_output_WS );

    TS_ASSERT_EQUALS( test_output_WS->size(), gold_output_WS->size() );

    for ( int i = 0; i < test_output_WS->size(); i++ )
    {
      TS_ASSERT_DELTA( test_output_WS->dataY(0)[i], gold_output_WS->dataY(0)[i], 0.00001 );
    }

    AnalysisDataService::Instance().remove("TestInputWS");
    AnalysisDataService::Instance().remove("SmoothedWS");
    AnalysisDataService::Instance().remove("ZeroingGoldWS");

//  std::cout<< "RUNNING FFTSmooth2 testZeroing() DONE!" << std::endl;
  }
  

  //-------------------------------------------------------------------------------------------------
  void performTest(bool event, std::string filter, std::string params, bool AllSpectra, int WorkspaceIndex, bool inPlace=false)
  {
    MatrixWorkspace_sptr ws1, out;
    int numPixels = 10;
    int numBins = 20;

    // Make workspaces where Y value == workspace index
    if (event)
      ws1 = WorkspaceCreationHelper::CreateEventWorkspace(10, numBins, numBins, 0, 1.0, 4);
    else
      ws1 = WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(numPixels, numBins);

    std::string outName = "SmoothedWS";

    Mantid::Algorithms::FFTSmooth2 alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    if (inPlace)
    {
      AnalysisDataService::Instance().addOrReplace("FFTSmooth2WsInput", ws1);
      TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue( "InputWorkspace", "FFTSmooth2WsInput" ) );
      outName = "FFTSmooth2WsInput";
    }
    else
    {
      TS_ASSERT_THROWS_NOTHING( alg.setProperty( "InputWorkspace", ws1 ) );
    }

    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue( "OutputWorkspace", outName) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty( "WorkspaceIndex", WorkspaceIndex ) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue( "Filter", filter ) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue( "Params", params) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty( "AllSpectra", AllSpectra) );
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    TS_ASSERT_THROWS_NOTHING( out=boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outName)) );
    TS_ASSERT( out );
    if (!out) return;

    TS_ASSERT_EQUALS( out->blocksize(), numBins);
    if (AllSpectra)
    {  TS_ASSERT_EQUALS( out->getNumberHistograms(), numPixels); }
    else
    {  TS_ASSERT_EQUALS( out->getNumberHistograms(), 1); }

    for (int wi=0; wi < out->getNumberHistograms(); wi++)
      for (int x=0; x<out->blocksize(); x++)
      {
        if (AllSpectra)
        {
          TS_ASSERT_DELTA( out->readY(wi)[x], wi*1.0, 0.02); // Because the spectra are flat, the smoothing won't do much
        }
        else
        {
          TS_ASSERT_DELTA( out->readY(wi)[x], WorkspaceIndex*1.0, 0.02); // Because the spectra are flat, the smoothing won't do much
        }
      }
  }

  //-------------------------------------------------------------------------------------------------
  // Some specific tests
  void test_Event_Butterworth_AllSpectra()
  { performTest(true,  "Butterworth", "100,2",  true, 1);  }

  void test_2D_Butterworth_AllSpectra()
  { performTest(false, "Butterworth", "100,2",  true, 2);  }

  void test_Event_Zeroing_AllSpectra()
  { performTest(true,  "Zeroing",     "100",    true, 3);  }

  void test_2D_Zeroing_AllSpectra()
  { performTest(false, "Zeroing",     "100",    true, 4);  }

  void test_Event_Butterworth_SingleSpectrum()
  { performTest(true,  "Butterworth", "100,2",  false, 4);  }

  void test_Event_Zeroing_SingleSpectrum()
  { performTest(true,  "Zeroing",     "100",    false, 6);  }


  //-------------------------------------------------------------------------------------------------
  /** Complete test of all possible inputs!
   * A total of 88 combinations are tested...
   * */
  void test_Everything()
  {
    for(int event=0; event<2; event++)
      for(int filterNum=0; filterNum<2; filterNum++)
        for(int AllSpectra=0; AllSpectra<2; AllSpectra++)
          for(int inPlace=0; inPlace<2; inPlace++)
          {
            std::string filter = "Zeroing";
            std::string params = "100";
            if (filterNum==1)
            { filter = "Butterworth"; params = "100,2"; }

            if (!AllSpectra)
            {
              for (int WorkspaceIndex=0; WorkspaceIndex<10; WorkspaceIndex++)
              {
                performTest(event, filter, params, AllSpectra, WorkspaceIndex, inPlace);
              }
            }
            else
            {
              performTest(event, filter, params, AllSpectra, 0, inPlace);
            }
          }
  }


private:
  Mantid::Algorithms::FFTSmooth2 fftsmooth2;

};

#endif /*FFTSMOOTH2TEST_H_*/
