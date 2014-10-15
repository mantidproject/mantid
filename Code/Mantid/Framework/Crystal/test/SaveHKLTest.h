#ifndef MANTID_CRYSTAL_SAVEHKLTEST_H_
#define MANTID_CRYSTAL_SAVEHKLTEST_H_

#include "MantidCrystal/SaveHKL.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::PhysicalConstants;

class SaveHKLTest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    SaveHKL alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void do_test(int numRuns, size_t numBanks, size_t numPeaksPerBank)
  {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(4, 10, 1.0);
    PeaksWorkspace_sptr ws(new PeaksWorkspace());
    ws->setInstrument(inst);
    double smu = 0.357;
    double amu = 0.011;
    NeutronAtom neutron(static_cast<uint16_t>(EMPTY_DBL()), static_cast<uint16_t>(0),
                        0.0, 0.0, smu, 0.0, smu, amu);
    Object sampleShape;
    sampleShape.setMaterial(Material("SetInSaveHKLTest", neutron, 1.0));
    ws->mutableSample().setShape(sampleShape);
    
    API::Run & mrun = ws->mutableRun();
    mrun.addProperty<double>("Radius", 0.1, true);

    for (int run=1000; run<numRuns+1000; run++)
      for (size_t b=1; b<=numBanks; b++)
      for (size_t i=0; i<numPeaksPerBank; i++)
      {
        V3D hkl(static_cast<double>(i), static_cast<double>(i), static_cast<double>(i));
        DblMatrix gon(3,3, true);
        Peak p(inst, static_cast<detid_t>(b*100 + i+1+i*10), static_cast<double>(i)*1.0+0.5, hkl, gon);
        p.setRunNumber(run);
        p.setBankName("bank1");
        p.setIntensity( static_cast<double>(i)+0.1);
        p.setSigmaIntensity( sqrt(static_cast<double>(i)));
        p.setBinCount( static_cast<double>(i) );
        ws->addPeak(p);
      }

    std::string outfile = "./SaveHKLTest.hkl";
    SaveHKL alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", outfile) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Get the file
    outfile = alg.getPropertyValue("Filename");
    bool fileExists = false;
    TS_ASSERT( fileExists = Poco::File(outfile).exists() );
    if ( fileExists )
    {
      std::ifstream in(outfile.c_str());

      double d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11,d12,d13,d14;
      if(numPeaksPerBank > 0)
      {
        in >> d1 >> d2 >> d3 >> d4 >> d5 >>d6 >> d7 >>d8 >>d9 >>d10>> d11>> d12>>d13>>d14;
        TS_ASSERT_EQUALS(d1,-1 );
        TS_ASSERT_EQUALS(d2,-1 );
        TS_ASSERT_EQUALS(d3,-1 );
        TS_ASSERT_EQUALS(d4,1.1 );
        TS_ASSERT_EQUALS(d5,1 );
        TS_ASSERT_EQUALS(d6,1 );
        TS_ASSERT_EQUALS(d7,1.5 );
        TS_ASSERT_EQUALS(d8,0.1591 );
        TS_ASSERT_EQUALS(d9,1000. );
        TS_ASSERT_EQUALS(d10,9 );
        TS_ASSERT_EQUALS(d11,0.9434 );
        TS_ASSERT_EQUALS(d12,1 );
        TS_ASSERT_DELTA(d13,0.4205 , 1e-4);
        TS_ASSERT_EQUALS(d14,3.5933 );
      }
    }

    if (Poco::File(outfile).exists())
      Poco::File(outfile).remove();
  }

  /// Test with an empty PeaksWorkspace
  void test_empty()
  {
    do_test(0,0,0);
  }

  /// Test with a few peaks
  void test_exec()
  {
    do_test(2, 4,4);
  }


};


#endif /* MANTID_CRYSTAL_SAVEHKLTEST_H_ */

