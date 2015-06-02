/*
 * ShowPeakHKLOffsetsTest.h
 *
 *  Created on: May 14, 2013
 *      Author: ruth
 */

#ifndef SHOWPEAKHKLOFFSETSTEST_H_
#define SHOWPEAKHKLOFFSETSTEST_H_
#include <cxxtest/TestSuite.h>
#include "MantidAPI/ITableWorkspace.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidCrystal/ShowPeakHKLOffsets.h"

using Mantid::Crystal::LoadIsawPeaks;
using Mantid::Crystal::LoadIsawUB;
using Mantid::Crystal::ShowPeakHKLOffsets;
using Mantid::DataObjects::TableWorkspace;


class ShowPeakHKLOffsetsTest: public CxxTest::TestSuite
{
  public:

  void test_show()
  {
     LoadIsawPeaks load;
     load.initialize();
     load.setProperty("Filename" , "TOPAZ_3007.peaks");
     load.setProperty("OutputWorkspace", "aaa");
     load.execute();

     LoadIsawUB addUB;
     addUB.initialize();
     addUB.setProperty("InputWorkspace", "aaa");

     addUB.setProperty("Filename","TOPAZ_3007.mat");
     addUB.execute();

     ShowPeakHKLOffsets show;

     TS_ASSERT_THROWS_NOTHING( show.initialize() )
     TS_ASSERT( show.isInitialized() )

     TS_ASSERT_THROWS_NOTHING(show.setProperty("PeaksWorkspace","aaa"))
     TS_ASSERT_THROWS_NOTHING(show.setProperty("HKLIntegerOffsets","offsets"))
     TS_ASSERT(show.execute())
     TS_ASSERT(show.isExecuted())
     show.setProperty("HKLIntegerOffsets","offsets");
     boost::shared_ptr<Mantid::API::ITableWorkspace> Offsets= show.getProperty("HKLIntegerOffsets"); 


     TS_ASSERT_DELTA( Offsets->Double(3,1), 0.0186555,.1)
     TS_ASSERT_DELTA( Offsets->Double(5,3), -0.0214665,.1)

     TS_ASSERT_EQUALS( Offsets->Int(8,4), 27)

     TS_ASSERT_EQUALS( Offsets->Int(13,5),3007)

     TS_ASSERT_DELTA( Offsets->Double(23,0) , -0.00976605,.1)


  }
};


#endif /* SHOWPEAKHKLOFFSETSTEST_H_ */
