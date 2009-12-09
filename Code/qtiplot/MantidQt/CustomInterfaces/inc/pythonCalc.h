#ifndef MANTIDQTCUSTOMINTERFACES_PYTHONCALC_H_
#define MANTIDQTCUSTOMINTERFACES_PYTHONCALC_H_

#include <QString>
#include <string>

class pythonCalc
{
public:
  // stores the informtion returned by the python script
  struct TestSummary
  {
    QString test;                       //< Name of the test is displayed to users
    QString status;                     //< status is displayed to users
    QString outputWS;                   //< Name of the workspace that contains the bad detectors
    int numBad;                         //< The total number of bad detectors
    QString inputWS;                    //< If these results came from loading another workspace this contains the name of that workspace
    enum resultsStatus {NORESULTS = 15-INT_MAX};  //< a flag value to indicate that there are no results to show, could be that the test has not completed or there was an error
  }; 
};

class deltaECalc : public pythonCalc
{
public:
  explicit deltaECalc(std::string settings);
  
  void run();
private:
};

#endif //MANTIDQTCUSTOMINTERFACES_PYTHONCALC_H_