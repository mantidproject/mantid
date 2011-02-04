#ifndef RANGESTRINGPARSERTEST_H_
#define RANGESTRINGPARSERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/UserStringParser.h"
#include <vector>

using namespace Mantid::Kernel;

class UserStringParserTest : public CxxTest::TestSuite
{
public:

  void testColonSeparatedStrings()
  {
    /*std::string s;
    std::getline(std::cin,s);*/

    UserStringParser parser;
    std::string str="2010:2020:2";
    unsigned int startNum=2010;


    std::vector<std::vector<unsigned int> > parsedVals=parser.parse(str);
    std::vector<std::vector<unsigned int> >::const_iterator parsedValcitr;
    for(parsedValcitr=parsedVals.begin();parsedValcitr!=parsedVals.end();++parsedValcitr)
    {
      std::vector<unsigned int>::const_iterator citr;
      for(citr=(*parsedValcitr).begin();citr!=(*parsedValcitr).end();++citr)
      {
       TS_ASSERT_EQUALS(*citr, startNum )
       startNum+=2;
      }
    }
   
  }

  void testMinusSeparatedStrings()
  {    
    UserStringParser parser;
    std::string str="60-61";
    unsigned int startNum=60;
    
    std::vector<std::vector<unsigned int> > parsedVals=parser.parse(str);
    std::vector<std::vector<unsigned int> >::const_iterator parsedValcitr;
    for(parsedValcitr=parsedVals.begin();parsedValcitr!=parsedVals.end();++parsedValcitr)
    {
      std::vector<unsigned int>::const_iterator citr;
      for(citr=(*parsedValcitr).begin();citr!=(*parsedValcitr).end();++citr)
      {
       TS_ASSERT_EQUALS(*citr, startNum )
       startNum+=1;
      }
    }

  }

  void testMinusSeparatedRangeofStrings()
  {
    /*std::string s;
    std::getline(std::cin,s);*/

    UserStringParser parser;
    std::string str="60-85";
    unsigned int startNum=60;
           
    std::vector<std::vector<unsigned int> > parsedVals=parser.parse(str);
    std::vector<std::vector<unsigned int> >::const_iterator parsedValcitr;
    for(parsedValcitr=parsedVals.begin();parsedValcitr!=parsedVals.end();++parsedValcitr)
    {
      std::vector<unsigned int>::const_iterator citr;
      for(citr=(*parsedValcitr).begin();citr!=(*parsedValcitr).end();++citr)
      {
        TS_ASSERT_EQUALS(*citr, startNum )
        startNum+=1;
      }
               
    }

  }

   void testPlusSeparatedStrings()
  {    
    UserStringParser parser;
    std::string str="62+63";
    unsigned int startNum=62;

    std::vector<std::vector<unsigned int> > parsedVals=parser.parse(str);
    std::vector<std::vector<unsigned int> >::const_iterator parsedValcitr;
    for(parsedValcitr=parsedVals.begin();parsedValcitr!=parsedVals.end();++parsedValcitr)
    {
      std::vector<unsigned int>::const_iterator citr;
      for(citr=(*parsedValcitr).begin();citr!=(*parsedValcitr).end();++citr)
      {
       TS_ASSERT_EQUALS(*citr, startNum )
       startNum+=1;
      }
    }
    
   
  }

  void testMinusandColonSeparatedStrings()
  {     
    UserStringParser parser;
    std::string str="4010-4020:2";
    unsigned int startNum=4010;

    std::vector<std::vector<unsigned int> > parsedVals=parser.parse(str);
    std::vector<std::vector<unsigned int> >::const_iterator parsedValcitr;
    for(parsedValcitr=parsedVals.begin();parsedValcitr!=parsedVals.end();++parsedValcitr)
    {
      std::vector<unsigned int>::const_iterator citr;
      for(citr=(*parsedValcitr).begin();citr!=(*parsedValcitr).end();++citr)
      {
        TS_ASSERT_EQUALS(*citr, startNum )
          startNum+=2;
      }
    }
    
  }

  void testCommaSeparatedComplexStrings1()
  {
        
    UserStringParser parser;
    std::string str="60-61,62+63,64-65";
   
    std::vector<std::vector<unsigned int> > parsedVals=parser.parse(str);
    TS_ASSERT_EQUALS(parsedVals.size(), 3 )

    TS_ASSERT_EQUALS((parsedVals[0][0]),60)
    TS_ASSERT_EQUALS((parsedVals[0][1]),61)
    TS_ASSERT_EQUALS((parsedVals[1][0]),62)
    TS_ASSERT_EQUALS((parsedVals[1][1]),63)
    TS_ASSERT_EQUALS((parsedVals[2][0]),64)
    TS_ASSERT_EQUALS((parsedVals[2][1]),65)
    
  }

  void testCommaSeparatedComplexStrings2()
  {
        
    UserStringParser parser;
    std::string str="60,61+62,64-66,68-74:2";
         
    std::vector<std::vector<unsigned int> > parsedVals=parser.parse(str);
    TS_ASSERT_EQUALS(parsedVals.size(), 4 )

    TS_ASSERT_EQUALS((parsedVals[0][0]),60)
    TS_ASSERT_EQUALS((parsedVals[1][0]),61)
    TS_ASSERT_EQUALS((parsedVals[1][1]),62)
    TS_ASSERT_EQUALS((parsedVals[2][0]),64)
    TS_ASSERT_EQUALS((parsedVals[2][1]),65)
    TS_ASSERT_EQUALS((parsedVals[2][2]),66)

    TS_ASSERT_EQUALS((parsedVals[3][0]),68)
    TS_ASSERT_EQUALS((parsedVals[3][1]),70)
    TS_ASSERT_EQUALS((parsedVals[3][2]),72)

   
  }

  void testCommaSeparatedComplexStrings3()
  {
        
    UserStringParser parser;
    std::string str="60,61,62:68";
         
    std::vector<std::vector<unsigned int> > parsedVals=parser.parse(str);
    TS_ASSERT_EQUALS(parsedVals.size(),9 )

    TS_ASSERT_EQUALS((parsedVals[0][0]),60)
    TS_ASSERT_EQUALS((parsedVals[1][0]),61)
    TS_ASSERT_EQUALS((parsedVals[2][0]),62)
    TS_ASSERT_EQUALS((parsedVals[3][0]),63)
    TS_ASSERT_EQUALS((parsedVals[4][0]),64)
    TS_ASSERT_EQUALS((parsedVals[5][0]),65)
    TS_ASSERT_EQUALS((parsedVals[6][0]),66)
    TS_ASSERT_EQUALS((parsedVals[7][0]),67)
    TS_ASSERT_EQUALS((parsedVals[8][0]),68)
       
  }


private:
 
};

#endif 
