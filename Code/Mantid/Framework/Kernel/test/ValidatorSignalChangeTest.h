#ifndef MANTID_KERNEL_SIGNALCHANGETEST_H_
#define MANTID_KERNEL_SIGNALCHANGETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/ValidatorSignalChange.h"
#include "MantidKernel/PropertyWithValue.h"
#include "boost/bind.hpp"

using namespace Mantid;
using namespace Mantid::Kernel;

std::string actOnPropertyChange(Property const *const pProp)
{
    std::string result;
    int middle_val(1);
    int new_val = boost::lexical_cast<int>(pProp->value());
    if (new_val>middle_val){
        return result.assign(" new property is bigger");
    }else{
       return result.assign(" new property is smaller");
    }


};

class propChanger{
public: 
    propChanger(Property & toChange):
      toModify(toChange){}
    std::string changes_analyser(Property const *const pProp){
       std::string rez = actOnPropertyChange(pProp);
       toModify.setValue(rez);
       return "";
    }
    Property &Accessor(){return toModify;}
private:
    Property &toModify;
};


class ValidatorSignalChangeTest : public CxxTest::TestSuite
{
public:

  void test_SendSignal()
  {

    PropertyWithValue<int> *ipProp     = new PropertyWithValue<int>("intProp", 1);
    ValidatorSignalChange<int> * pValC = new ValidatorSignalChange<int>(ipProp);
    pValC->connect(&actOnPropertyChange);

    ipProp->setValue("2");
    // value provided to validator function is irrelevant here, as check occurs on ipProp and the number is just to keep the function signaturel
    TSM_ASSERT_EQUALS("should return correct message "," new property is bigger",pValC->isValid(2));
    //
    ipProp->setValue("0");
    // value provided to validator function is irrelevant here, as check occurs on ipProp and the number is just to keep the function signaturel
    TSM_ASSERT_EQUALS("should return correct message"," new property is smaller",pValC->isValid(2));
    //
    delete ipProp;
    delete pValC;
  }
  void test_changeProp()
  {

    PropertyWithValue<int> *ipProp     = new PropertyWithValue<int>("intProp", 1);
    ValidatorSignalChange<int> * pValC = new ValidatorSignalChange<int>(ipProp);

    PropertyWithValue<std::string > *spProp     = new PropertyWithValue<std::string>("stringProp", "");
    propChanger propCh(*spProp);

    boost::function<std::string (Property const * const)> target;

    target = std::bind1st(std::mem_fun(&propChanger::changes_analyser),&propCh);

    pValC->connect(target);

    ipProp->setValue("2");
    // runs validator as it usually happens within an algorithm property
    pValC->isValid(2);
    TSM_ASSERT_EQUALS("should return correct message "," new property is bigger",propCh.Accessor().value());
    //
    ipProp->setValue("0");
    // runs validator as it usually happens within an algorithm property
    pValC->isValid(2);
    TSM_ASSERT_EQUALS("should return correct message"," new property is smaller",propCh.Accessor().value());
    //
    delete ipProp;
    delete pValC;
    delete spProp;
  }
};
#endif /* MANTID_KERNEL_SIGNALCHANGETEST_H_ */
