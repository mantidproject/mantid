/*
 * LoadEventPreNeXusTest.h
 *
 *  Created on: Jun 23, 2010
 *      Author: janik zikovsky
 */

#ifndef LOADEVENTPRENEXUSTEST_H_
#define LOADEVENTPRENEXUSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include <boost/timer.hpp>

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::Kernel::Exception;
using namespace Mantid::API;
using namespace Mantid::Geometry;

using std::runtime_error;
using std::size_t;
using std::vector;
using std::cout;
using std::endl;
using namespace boost::posix_time;



//==========================================================================================
class LoadEventPreNeXusTest : public CxxTest::TestSuite
{
public:
  LoadEventPreNeXusTest()
  {    }

//  void setUp()
//  {
//    //Pass
//  }

  void xtest_LoadPreNeXus()
  {
    // start of LoadEventPreNeXus test
    const clock_t start = clock();
    LoadEventPreNeXus eventLoader;
    eventLoader.setPropertyValue("EventFilename",
         "/SNS/REF_L/IPTS-2574/45/32035/preNeXus/REF_L_32035_neutron_event.dat");
    eventLoader.setPropertyValue("MappingFilename",
          "/SNS/REF_L/2009_3_4B_CAL/calibrations/REF_L_TS_2010_02_19.dat");
    eventLoader.setPropertyValue("OutputWorkspace", "bobby");
    std::cout << "**********" << std::endl;
    eventLoader.execute();

    EventWorkspace_sptr eventWksp = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("bobby"));
    std::cout << "name:" << eventWksp->id() << std::endl;
    std::cout << "num histo:" << eventWksp->getNumberHistograms() << std::endl;
    std::cout << "num events:" << eventWksp->getNumberEvents() << std::endl;
    std::cout << "**********" << std::endl;
    // end of LoadEventPreNeXus test
  }

};

#endif /* LOADEVENTPRENEXUSTEST_H_ */
