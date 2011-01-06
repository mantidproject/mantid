#ifndef MANTID_TESTREGEXSUPPORT__
#define MANTID_TESTREGEXSUPPORT__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <sstream>

#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"

#include "MantidGeometry/V3D.h"
#include <boost/regex.hpp>
#include "MantidGeometry/Math/RegexSupport.h"


using namespace Mantid;
using namespace StrFunc;

class RegexSupportTest: public CxxTest::TestSuite
{
private:
  std::stringstream testStream;

public:
  RegexSupportTest()
  {
    testStream << "2007-11-16T13:25:48 END\n"
               << "2007-11-16T13:29:36 CHANGE RUNTABLE\n"
               << "2007-11-16T13:29:49 CHANGE RUNTABLE\n"
               << "2007-11-16T13:30:21 CHANGE RUNTABLE\n"
               << "2007-11-16T13:32:38 BEGIN\n"
               << "2007-11-16T13:43:40 ABORT\n";
  }

	void testStrComp(){

		double result;
		//By Default perl regex because using boost directly
		TS_ASSERT_EQUALS(StrComp(std::string("100.01 101.02 103.04 105.06 "),boost::regex("(([0-9]*.[0-9]*) )?"),result,0),1);
		TS_ASSERT_EQUALS(result,100.01);
		TS_ASSERT_EQUALS(StrComp(std::string("100.01 101.02 103.04 105.06 "),boost::regex("(([0-9]*.[0-9]*) )?"),result,1),1);		
		TS_ASSERT_EQUALS(result,101.02);
		TS_ASSERT_EQUALS(StrComp(std::string("100.01 101.02 103.04 105.06 "),boost::regex("(([0-9]*.[0-9]*) )?"),result,2),1);		
		TS_ASSERT_EQUALS(result,103.04);
		TS_ASSERT_EQUALS(StrComp("100.01 101.02 103.04 105.06 ",boost::regex("(([0-9]*.[0-9]*) )?"),result,3),1);		
		TS_ASSERT_EQUALS(result,105.06);
	}

	void testStrLook(){
		//By Default perl regex because using boost directly
		TS_ASSERT_EQUALS(StrLook("Mantid Geometry Regular Expression",boost::regex("xp")),1);
		TS_ASSERT_EQUALS(StrLook("Mantid Geometry Regular Expression",boost::regex("met")),1);
		TS_ASSERT_EQUALS(StrLook("Mantid Geometry Regular Expression",boost::regex(" ")),1);
		TS_ASSERT_EQUALS(StrLook("Mantid Geometry Regular Expression",boost::regex("rE")),0);

		TS_ASSERT_EQUALS(StrLook("1234-5678-1234-456",boost::regex("([[:digit:]]{4}[- ]){3}[[:digit:]]{3,4}")),1);
		TS_ASSERT_EQUALS(StrLook("OX11 0QX",boost::regex("^[a-zA-Z]{1,2}[0-9][0-9A-Za-z]{0,1} {0,1}[0-9][A-Za-z]{2}$")),1);
	}

	void testStrParts(){
		std::vector<std::string> tokens=StrParts("Mantid Geometry Regular Expression",boost::regex(" "));
		TS_ASSERT_EQUALS(tokens[0],"Mantid");
		TS_ASSERT_EQUALS(tokens[1],"Geometry");
		TS_ASSERT_EQUALS(tokens[2],"Regular");
		TS_ASSERT_EQUALS(tokens[3],"Expression");
	}

	void testStrFullSplit(){
		std::vector<double> dblresult;
		TS_ASSERT_EQUALS(StrFullSplit(std::string("100.01 101.02 103.04 105.06 "),boost::regex("([0-9]*.[0-9]* )?"),dblresult),4);
		TS_ASSERT_EQUALS(dblresult[0],100.01);
		TS_ASSERT_EQUALS(dblresult[1],101.02);
		TS_ASSERT_EQUALS(dblresult[2],103.04);
		TS_ASSERT_EQUALS(dblresult[3],105.06);
		std::vector<int> intresult;
		TS_ASSERT_EQUALS(StrFullSplit(std::string("100 101 103 105 "),boost::regex("([0-9]* )?"),intresult),4);
		TS_ASSERT_EQUALS(intresult[0],100);
		TS_ASSERT_EQUALS(intresult[1],101);
		TS_ASSERT_EQUALS(intresult[2],103);
		TS_ASSERT_EQUALS(intresult[3],105);
		std::vector<std::string> strresult;
		TS_ASSERT_EQUALS(StrFullSplit(std::string("100.01 101.02 103.04 105.06 "),boost::regex("([0-9]*.[0-9]* )?"),strresult),4);
		TS_ASSERT_EQUALS(strresult[0],"100.01");
		TS_ASSERT_EQUALS(strresult[1],"101.02");
		TS_ASSERT_EQUALS(strresult[2],"103.04");
		TS_ASSERT_EQUALS(strresult[3],"105.06");
	}

	void testStrSingleSplit(){
		std::vector<double> dblresult;
		TS_ASSERT_EQUALS(StrSingleSplit(std::string("100.01 101.02 103.04 105.06 "),boost::regex("([0-9]*.[0-9]* )?"),dblresult),1);
		TS_ASSERT_EQUALS(dblresult[0],100.01);
		dblresult.clear();
		TS_ASSERT_EQUALS(StrSingleSplit(std::string("101.02 103.04 105.06 "),boost::regex("([0-9]*.[0-9]* )?"),dblresult),1);
		TS_ASSERT_EQUALS(dblresult[0],101.02);
		std::vector<int> intresult;
		TS_ASSERT_EQUALS(StrSingleSplit(std::string("100 101 103 105 "),boost::regex("([0-9]* )?"),intresult),1);
		TS_ASSERT_EQUALS(intresult[0],100);
	}

	void testStrFullCut(){
		std::vector<double> dblresult;
		double sgldblResult;
		std::string input("100.01 101.02 103.04 105.06 Remainder of string");
		TS_ASSERT_EQUALS(StrFullCut(input,boost::regex("([0-9]*.[0-9]* )?"),sgldblResult,0),1);
		TS_ASSERT_EQUALS(sgldblResult,100.01);
		TS_ASSERT_EQUALS(input,"101.02 103.04 105.06 Remainder of string");
		TS_ASSERT_EQUALS(StrFullCut(input,boost::regex("([0-9]*.[0-9]* )?"),sgldblResult,-1),1);
		TS_ASSERT_EQUALS(sgldblResult,101.02);
		TS_ASSERT_EQUALS(input,"103.04 105.06 Remainder of string");
	}

	void testStrRemove(){
		std::string input("100.01 101.02 103.04 105.06 Remainder of string");
		std::string output;
		TS_ASSERT_EQUALS(StrRemove(input,output,boost::regex("([0-9]*.[0-9]* )*")),1);
		TS_ASSERT_EQUALS(input,"Remainder of string");
		TS_ASSERT_EQUALS(output,"100.01 101.02 103.04 105.06 ");
	}

	void testFindComp(){
		std::string output;
		TS_ASSERT_EQUALS(findComp(testStream,boost::regex("BEGIN"),output),5);
		TS_ASSERT_EQUALS(output,"");
                // Reset the stream pointer
		testStream.seekg(0);
	}

	void testFindPattern(){
		 std::string output;
		 TS_ASSERT_EQUALS(findPattern(testStream,boost::regex("BEGIN"),output),5);
		 TS_ASSERT_EQUALS(output,"2007-11-16T13:32:38 BEGIN");
	}
};

#endif
