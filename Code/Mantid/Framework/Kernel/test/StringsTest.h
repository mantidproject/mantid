#ifndef MANTID_SUPPORTTEST_H_
#define MANTID_SUPPORTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Strings.h"
#include <string>

using namespace Mantid::Kernel::Strings;

/**
  \class StringsTest
  \brief test of Strings components
  \date September 2005
  \author S.Ansell
  
  Checks the basic string operations in Strings
*/

class StringsTest : public CxxTest::TestSuite
{
public: 

	void testExtractWord()
	  /**
		Applies a test to the extractWord
		The object is to find a suitable lenght
		of a string in a group of words
		@retval -1 :: failed find word in string
		when the pattern exists.
	  */
	{
	  std::string Ln="Name wav wavelength other stuff";
	  int retVal=extractWord(Ln,"wavelengt",4);
	  TS_ASSERT_EQUALS(Ln, "Name wav  other stuff"); 
	}

	void testConvert()
	{
	  int i;
	  //valid double convert
	  TS_ASSERT_EQUALS(convert("   568   ",i), 1);
	  TS_ASSERT_EQUALS(i,568);
	  double X;
	  //valid double convert
	  TS_ASSERT_EQUALS(convert("   3.4   ",X), 1);
	  TS_ASSERT_EQUALS(X,3.4);
	  X=9.0;
	  //invalid leading stuff
	  TS_ASSERT_EQUALS(convert("   e3.4   ",X),0);
	  TS_ASSERT_EQUALS(X,9.0);
	  //invalid trailing stuff
	  TS_ASSERT_EQUALS(convert("   3.4g   ",X),0);
	  TS_ASSERT_EQUALS(X,9.0);
	  std::string Y;
	  TS_ASSERT_EQUALS(convert("   3.4y   ",Y),1);
	  TS_ASSERT_EQUALS(Y,"3.4y");
	}

	void testSection()
	{
	  std::string Mline="V 1 tth ";
	  std::string Y;
	  TS_ASSERT_EQUALS(section(Mline,Y),1);
	  TS_ASSERT_EQUALS(Y,"V");
	  TS_ASSERT_EQUALS(Mline," 1 tth ");  // Note the non-remove spc
	}

	void testSectPartNum()
	{
	  double X;
	  std::string NTest="   3.4   ";
	  TS_ASSERT_EQUALS(sectPartNum(NTest,X),1);
	  TS_ASSERT_EQUALS(X,3.4);
	  X=9.0;
	  NTest="   3.4g   ";
	  TS_ASSERT_EQUALS(sectPartNum(NTest,X),1);
	  TS_ASSERT_EQUALS(X,3.4);
	  X=9.0;
	  NTest="   e3.4   ";
	  TS_ASSERT_DIFFERS(sectPartNum(NTest,X),1);
	  TS_ASSERT_EQUALS(X,9.0);
	}


	void test_join()
	{
	  std::vector<std::string> v;
    std::string out;

    out = join(v.begin(), v.end(), ",");
    TS_ASSERT_EQUALS( out, "");

    v.push_back("Help");
    v.push_back("Me");
    v.push_back("I'm");
    v.push_back("Stuck");
    v.push_back("Inside");
    v.push_back("A");
    v.push_back("Test");

    out = join(v.begin(), v.end(), ",");
    TS_ASSERT_EQUALS( out, "Help,Me,I'm,Stuck,Inside,A,Test");
	}


	void test_replace()
	{
	  std::string in = "hello\nI hate\nnewlines.\n";
	  std::string out = replace(in, "\n", " ");
    TS_ASSERT_EQUALS(out, "hello I hate newlines. ");

    TS_ASSERT_EQUALS(replace("bla", "bla", ""), "");
    TS_ASSERT_EQUALS(replace("FirstSecond", "First", ""), "Second");
    TS_ASSERT_EQUALS(replace("FirstSecond", "Second", ""), "First");
    TS_ASSERT_EQUALS(replace("Hello You", " ", " I am stupid, "), "Hello I am stupid, You");
	}


};

#endif //MANTID_SUPPORTTEST_H_
