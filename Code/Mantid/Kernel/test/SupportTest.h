#ifndef testSupport_h
#define testSupport_h 


/*!
  \class testSupport 
  \brief test of Support components
  \version 1.0
  \date September 2005
  \author S.Ansell
  
  Checks the basic string operations in
  support.cxx and regexSupport.cxx
*/

class testSupport
{
private:

  //Tests 
  int testConvert();     ///< test convert
  int testExtractWord(); ///< test extractWord
  int testSection();     ///< test section
  int testSectPartNum(); ///< test sectPartNum
  int testSetValues();   ///< test setValues
  int testStrComp();     ///< test StrComp
  int testStrFullCut();  ///< test StrCut
  int testStrParts();    ///< test StrParts
  int testStrRemove();   ///< test strRemove
  int testStrSplit();    ///< test StrSplit

public:

  testSupport();
  ~testSupport();

  int applyTest(const int);     

};

#endif
