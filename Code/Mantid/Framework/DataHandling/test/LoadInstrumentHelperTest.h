#ifndef LOADINSTRUMENTHELPERTEST_H_
#define LOADINSTRUMENTHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadInstrumentHelper.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/IDetector.h"
#include <vector>
#include <iostream>
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Glob.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/RegularExpression.h>
#include <Poco/SAX/SAXParser.h>
#include <Poco/SAX/ContentHandler.h>
#include <Poco/SAX/Attributes.h>


using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

using namespace Poco::XML;



class LoadInstrumentHelperTest : public CxxTest::TestSuite
{
public:

  void test_nothing()
  {
    /** Tests moved to ExperimentInfoTest Sep 13, 2011 */
  }

};

#endif /*LOADINSTRUMENTHELPERTEST_H_*/

