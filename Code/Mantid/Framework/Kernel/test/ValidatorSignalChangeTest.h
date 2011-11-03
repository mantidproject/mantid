#ifndef MANTID_KERNEL_SIGNALCHANGETEST_H_
#define MANTID_KERNEL_SIGNALCHANGETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/ValidatorSignalChange.h"
#include "MantidKernel/PropertyWithValue.h"

using namespace Mantid;
using namespace Mantid::Kernel;

void actOnPropertyChange(const Property *pProp)
{
    int middle_val(1);
    int new_val = boost::lexical_cast<int>(pProp->value());
    if (new_val>middle_val){
        throw(" new prop is bigger");
    }else{
        throw(std::string(" new prop is smaller"));
    }

};

class SignalChangeTest : public CxxTest::TestSuite
{
public:

  void test_SendSignal()
  {

    PropertyWithValue<int> *ipProp     = new PropertyWithValue<int>("intProp", 1);
    ValidatorSignalChange<int> * pValC = new ValidatorSignalChange<int>(ipProp);
    pValC->connect(&actOnPropertyChange);

    ipProp->setValue("2");
    // value provided to validator function is irrelevant here, as check occurs on ipProp and the number is just to keep the function signaturel
    TSM_ASSERT_THROWS("should throw a pointer to ""new prop is bigger""",pValC->isValid(2),const char *);
    //
    ipProp->setValue("0");
    // value provided to validator function is irrelevant here, as check occurs on ipProp and the number is just to keep the function signaturel
    TSM_ASSERT_THROWS("should throw a pointer to ""new prop is smaller""",pValC->isValid(2),std::string);
    //
    delete ipProp;
    delete pValC;
  }
};
#endif /* MANTID_KERNEL_SIGNALCHANGETEST_H_ */
