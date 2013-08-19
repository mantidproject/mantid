#ifndef MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILETEST_H_
#define MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SaveGSASInstrumentFile.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include <fstream>
#include <Poco/File.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

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
    saver.setPropertyValue("BankIDs", "1");
    saver.setProperty("Instrument", "PG3");
    saver.setPropertyValue("ChopperFrequency", "60");
    saver.setProperty("IDLine", "PG60_2011B");
    saver.setProperty("Sample", "LaB6");
    saver.setProperty("L1", 60.0);
    saver.setProperty("TwoTheta", 90.0);

    // Execute the algorithm
    saver.execute();
    TS_ASSERT(saver.isExecuted());

    // Check the output file's existence and size;
    TS_ASSERT(Poco::File("test.iparm").exists());
    Poco::File::FileSize size = Poco::File("test.iparm").getSize();
    // TS_ASSERT(size >= 16191 && size <= 16209); Removed due to windows

    AnalysisDataService::Instance().remove("PG3ProfileTable");
    Poco::File("test.iparm").remove();
  }

  void Xtest_SaveGSSInstrumentFile_MultiBank()
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
    saver.setPropertyValue("BankIDs", "1");
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
  }

  // Load table workspace containing instrument parameters
  void loadProfileTable(string wsname)
  {
    // The data befow is from Bank1 in pg60_2011B.irf

    TableWorkspace_sptr tablews(new TableWorkspace);
    tablews->addColumn("str", "Name");
    tablews->addColumn("double", "Value_1");

    vector<string> parnames;
    vector<double> parvalues;

    parnames.push_back("BANK");     parvalues.push_back(1.);
    parnames.push_back("Alph0"); 	parvalues.push_back(0.5    );
    parnames.push_back("Alph0t"); 	parvalues.push_back(65.14  );
    parnames.push_back("Alph1"); 	parvalues.push_back(8.15   );
    parnames.push_back("Alph1t"); 	parvalues.push_back(0      );
    parnames.push_back("Beta0"); 	parvalues.push_back(3.201  );
    parnames.push_back("Beta0t"); 	parvalues.push_back(78.412 );
    parnames.push_back("Beta1"); 	parvalues.push_back(7.674  );
    parnames.push_back("Beta1t"); 	parvalues.push_back(0      );
    parnames.push_back("Dtt1"); 	parvalues.push_back(22780.57);
    parnames.push_back("Dtt1t"); 	parvalues.push_back(22790.129);
    parnames.push_back("Dtt2"); 	parvalues.push_back(0      );
    parnames.push_back("Dtt2t"); 	parvalues.push_back(0.3    );
    parnames.push_back("Gam0"); 	parvalues.push_back(0      );
    parnames.push_back("Gam1"); 	parvalues.push_back(0      );
    parnames.push_back("Gam2"); 	parvalues.push_back(0      );
    parnames.push_back("Sig0"); 	parvalues.push_back(0      );
    parnames.push_back("Sig1"); 	parvalues.push_back(sqrt(10.0));
    parnames.push_back("Sig2"); 	parvalues.push_back(sqrt(403.30));
    parnames.push_back("Tcross"); 	parvalues.push_back(0.3560 );
    parnames.push_back("Width"); 	parvalues.push_back(1.2141 );
    parnames.push_back("Zero"); 	parvalues.push_back(0      );
    parnames.push_back("Zerot"); 	parvalues.push_back(-70.60  );
    parnames.push_back("step"); 	parvalues.push_back(5      );
    parnames.push_back("tof-max"); 	parvalues.push_back(46760  );
    parnames.push_back("tof-min"); 	parvalues.push_back(2278.06);
    parnames.push_back("twotheta"); parvalues.push_back(90.807 );

    for (size_t i = 0; i < parnames.size(); ++i)
    {
      TableRow row = tablews->appendRow();
      row << parnames[i] << parvalues[i];
    }

    AnalysisDataService::Instance().addOrReplace(wsname, tablews);

    return;
  }

  // Compare 2 files
  bool compare2Files(std::string filename1, std::string filename2)
  {
    ifstream file1(filename1.c_str(), std::ifstream::in);
    ifstream file2(filename2.c_str(), std::ifstream::in);


    // file1.open( filename1.c_str(), ios::binary ); //c_str() returns C-style string pointer
    // file2.open( filename2.c_str(), ios::binary );

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
