#ifndef LOADINSTRUMENTHELPERTEST_H_
#define LOADINSTRUMENTHELPERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadInstrumentHelper.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument/Instrument.h"
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

#include "Poco/File.h"
#include "Poco/Path.h"
#include "Poco/DirectoryIterator.h"
#include "Poco/DateTimeParser.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/RegularExpression.h"

#include "Poco/SAX/SAXParser.h"
#include "Poco/SAX/ContentHandler.h"
#include "Poco/SAX/Attributes.h"


using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

using namespace Poco::XML;



struct fromToEntry
{
  //validEntry(std::string path) : from(DataAndTime(str)) {}
  std::string path; 
  DateAndTime from;
  DateAndTime to;
};

class LoadInstrumentHelperTest : public CxxTest::TestSuite
{
public:

  // Test that all the IDFs contain valid-to and valid-from dates and that
  // for a single instrument none of these overlap
  void testAllDatesInIDFs()
  {
    LoadInstrumentHelper helper;


    // Collect all IDF filenames and put them in a multimap where the instrument
    // identifier is the key
    std::multimap<std::string, fromToEntry> idfFiles;
    std::set<std::string> idfIdentifiers;

    std::string m_filename = "../../../../Test/Instrument/HET_Definition.xml";
    Poco::RegularExpression regex(".*_Definition.*\\.xml", Poco::RegularExpression::RE_CASELESS );
    Poco::DirectoryIterator end_iter;
    for ( Poco::DirectoryIterator dir_itr(Poco::Path(m_filename).parent()); dir_itr != end_iter; ++dir_itr )
    {
          if ( !Poco::File(dir_itr->path() ).isFile() ) continue;

          std::string l_filenamePart = Poco::Path(dir_itr->path()).getFileName();

          if ( regex.match(l_filenamePart) )
          {
            std::string validFrom, validTo;
            helper.getValidFromTo(dir_itr->path(), validFrom, validTo);

            size_t found;
            found = l_filenamePart.find("_Definition");
            fromToEntry ft;
            ft.path = dir_itr->path();
            ft.from.set_from_ISO8601_string(validFrom);
            ft.to.set_from_ISO8601_string(validTo);
            idfFiles.insert( std::pair<std::string,fromToEntry>(l_filenamePart.substr(0,found), 
              ft) );
            idfIdentifiers.insert(l_filenamePart.substr(0,found));
          }
    }

          // iterator to browse through the multimap: paramInfoFromIDF

    std::multimap<std::string,fromToEntry> :: const_iterator it1, it2;
    std::pair<std::multimap<std::string,fromToEntry>::iterator,
    std::multimap<std::string,fromToEntry>::iterator> ret;


    std::set<std::string>::iterator setIt;
    for (setIt=idfIdentifiers.begin(); setIt != idfIdentifiers.end(); setIt++)
    {
      ret = idfFiles.equal_range(*setIt);
      for (it1 = ret.first; it1 != ret.second; ++it1)
      {
        for (it2 = ret.first; it2 != ret.second; ++it2)
        {
          if (it1 != it2)
          {
            if ( it2->second.from >= it1->second.to || it2->second.to <= it1->second.from )
            {
              //std::cout << "\nOK\n";
            }
            else
            {
              // some more intelligent stuff here later
              TS_ASSERT("dates in IDF overlap"=="0");
            }
          }
        }

      }
    }
  }

  // 
  void testInstrumentHelperFunctions()
  {
    LoadInstrumentHelper helper;

    std::string boevs = helper.getIDF_identifier("BIOSANS", "2100-01-31 22:59:59");
    TS_ASSERT(boevs.empty());
  }

};

#endif /*LOADINSTRUMENTHELPERTEST_H_*/

