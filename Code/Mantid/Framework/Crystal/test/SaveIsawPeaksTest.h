#ifndef MANTID_CRYSTAL_SAVEISAWPEAKSTEST_H_
#define MANTID_CRYSTAL_SAVEISAWPEAKSTEST_H_

#include "MantidCrystal/SaveIsawPeaks.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

class SaveIsawPeaksTest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    SaveIsawPeaks alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void do_test(int numRuns, size_t numBanks, size_t numPeaksPerBank)
  {
    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(4, 10, 1.0);
    PeaksWorkspace_sptr ws(new PeaksWorkspace());
    ws->setInstrument(inst);

    for (int run=1000; run<numRuns+1000; run++)
      for (size_t b=1; b<=numBanks; b++)
      for (size_t i=0; i<numPeaksPerBank; i++)
      {
        V3D hkl(static_cast<double>(i), static_cast<double>(i), static_cast<double>(i));
        MantidMat gon(3,3, true);
        Peak p(inst, static_cast<detid_t>(b*100 + i+1+i*10), static_cast<double>(i)*1.0+0.5, hkl, gon);
        p.setRunNumber(run);
        p.setIntensity( static_cast<double>(i)+0.1);
        p.setSigmaIntensity( sqrt(static_cast<double>(i)));
        p.setBinCount( static_cast<double>(i) );
        ws->addPeak(p);
      }

    std::string outfile = "./SaveIsawPeaksTest.peaks";
    SaveIsawPeaks alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Filename", outfile) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Get the file
    outfile = alg.getPropertyValue("Filename");
    TS_ASSERT( Poco::File(outfile).exists() );
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


#endif /* MANTID_CRYSTAL_SAVEISAWPEAKSTEST_H_ */

