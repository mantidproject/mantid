#ifndef MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILETEST_H_
#define MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILETEST_H_

#include <cxxtest/TestSuite.h>

//#include "MantidAlgorithms/SaveGSASInstrumentFile.h"
#include "SaveGSASInstrumentFile.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataHandling/LoadNexusProcessed.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

using namespace std;

using Mantid::Algorithms::SaveGSASInstrumentFile;

class SaveGSASInstrumentFileTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SaveGSASInstrumentFileTest *createSuite() { return new SaveGSASInstrumentFileTest(); }
  static void destroySuite( SaveGSASInstrumentFileTest *suite ) { delete suite; }


  void test_SaveGSSInstrumentFile_1Bank()
  {
    // Load a (local) table workspace
    loadProfileTable("PG3ProfileTable");
    TableWorkspace_sptr profiletablews = boost::dynamic_pointer_cast<TableWorkspace>(
          AnalysisDataService::Instance().retrieve("PG3ProfileTable"));
    TS_ASSERT(profiletablews);

    // Set up the algorithm
    SaveGSASInstrumentFile saver;
    saver.initialize();
    TS_ASSERT(saver.isInitialized());

    saver.setProperty("InputWorkspace", "PG3ProfileTable");
    saver.setProperty("OutputFilename", "test.iparm");
    saver.setPropertyValue("BankIDs", "4");
    saver.setProperty("Instrument", "PG3");
    saver.setPropertyValue("ChopperFrequency", "60");
    saver.setProperty("IDLine", "Blablabla Blablabla");
    saver.setProperty("Sample", "whatever");
    saver.setProperty("L1", 60.0);
    saver.setProperty("L2", 0.321);
    saver.setProperty("TwoTheta", 90.1);

    // Execute the algorithm
    saver.execute();
    TS_ASSERT(saver.isExecuted());

    // Check the output file against ... ....
    // Load generated file

    AnalysisDataService::Instance().remove("PG3ProfileTable");
    TS_ASSERT_EQUALS(1, 9876);

  }

  void test_SaveGSSInstrumentFile_MultiBank()
  {
    // Load a (local) table workspace
    loadProfileTable("PG3ProfileTable");
    TableWorkspace_sptr profiletablews = boost::dynamic_pointer_cast<TableWorkspace>(
          AnalysisDataService::Instance().retrieve("PG3ProfileTable"));
    TS_ASSERT(profiletablews);

    // Set up the algorithm
    SaveGSASInstrumentFile saver;
    saver.initialize();
    TS_ASSERT(saver.isInitialized());

    saver.setProperty("InputFullprofResolutonFile", "pg3_60hz.irf");
    saver.setProperty("OutputFilename", "test.iparm");
    saver.setPropertyValue("BankIDs", "4");
    saver.setProperty("Instrument", "PG3");
    saver.setPropertyValue("ChopperFrequency", "60");
    saver.setProperty("IDLine", "Blablabla Blablabla");
    saver.setProperty("Sample", "whatever");
    saver.setProperty("L1", 60.0);
    saver.setProperty("L2", 0.321);
    saver.setProperty("TwoTheta", 90.1);

    // Execute the algorithm
    saver.execute();
    TS_ASSERT(saver.isExecuted());

    // Check the output file against ... ....
    // Load generated file

    AnalysisDataService::Instance().remove("PG3ProfileTable");
    TS_ASSERT_EQUALS(1, 9876);

  }


  // Load table workspace containing instrument parameters
  void loadProfileTable(string wsname)
  {
    string tablewsname("pg3_bank1_params.nxs");

    LoadNexusProcessed loader;
    loader.initialize();

    loader.setProperty("Filename", tablewsname);
    loader.setProperty("OutputWorkspace", wsname);

    loader.execute();

    return;
  }

  // Compare 2 files
  bool compare2Files(std::string filename1, std::string filename2)
  {
    ifstream file1, file2;

    file1.open( filename1.c_str(), ios::binary ); //c_str() returns C-style string pointer
    file2.open( filename2.c_str(), ios::binary );

    if (!file1)
    {
      cout << "Couldn't open the file  " << filename1<<endl;
      return false;
    }

    if (!file2)
    {
      cout << "Couldn't open the file " << filename2 << endl;
      return false;
    }

    //---------- compare number of lines in both files ------------------//
    int c1,c2;
    c1 = 0;
    c2 = 0;
    string str;
    while(!file1.eof())
    {
      getline(file1,str);
      c1++;
    }

    if(c1 != c2)
    {
      cout << "Different number of lines in files!" << "\n";
      cout << filename1 << " has " << c1 << " lines and "<<  filename2 <<" has" << c2 << " lines" << "\n";
      return false;
    }

    while(!file2.eof())
    {
      getline(file2,str);
      c2++;
    }

    // Reset file stream pointer
     file1.clear();  //add
     file1.seekg(0,ios::beg);  //add

     file2.clear();
     file2.seekg(0,ios::beg);


 //---------- compare two files line by line ------------------//
     char string1[256], string2[256];
     int j = 0, error_count =0;
     while(!file1.eof())
     {
         file1.getline(string1,256);
         file2.getline(string2,256);
         j++;
         if(strcmp(string1,string2) != 0)
         {
             cout << j << "-the strings are not equal " << endl;
             cout << " file1   " << string1 << endl;
             cout << " file2:  " << string2 << endl;
             error_count++;
         }
     }
     if (error_count > 0) {
      cout << "files are diffrent"<< endl;
      return false;
     }
     else
     {
       cout << "files are the same"<< endl;
     }

     return true;
 }

};


#endif /* MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILETEST_H_ */
