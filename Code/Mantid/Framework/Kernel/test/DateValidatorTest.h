#ifndef DATEVALIDATORTEST_H_
#define DATEVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/DateValidator.h"

using namespace Mantid::Kernel;

class DateValidatorTest : public CxxTest::TestSuite
{
public:
 
  void testInValidFormat()
  {    
	  DateValidator v;
	  TS_ASSERT_EQUALS( v.isValid("ddmmyyyy"),
		  "Invalid Date:date format must be DD/MM/YYYY")

	  TS_ASSERT_EQUALS( v.isValid("dd/mm:yyyy"),
	  "Invalid Date")
  }

  void testInvalidaDate()
  {
	  	/*std::string s;
		std::getline(std::cin,s);*/

		DateValidator v;
		TS_ASSERT_EQUALS( v.isValid("32/10/2009"),
		"Invalid Date:Day part of the Date parameter must be between 1 and 31")

		TS_ASSERT_EQUALS( v.isValid("12/101/2009"),
		"Invalid Date:Month part of the Date parameter must be between 1 and 12")

		TS_ASSERT_EQUALS( v.isValid("12/10/2012"),
		"Invalid Date:Year part of the Date parameter can not be greater than the current year")
   
  }
  void testValidDate()
  {
	  DateValidator v;
	  TS_ASSERT_EQUALS( v.isValid(""), "" ) //empty date is allowed
	  TS_ASSERT_EQUALS( v.isValid("12/11/2009"), "" ) 
	  TS_ASSERT_EQUALS( v.isValid("22/01/2006"), "" )

  }
   
};

#endif /*DATEVALIDATORTEST_H_*/
