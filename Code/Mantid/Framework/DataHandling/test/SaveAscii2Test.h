#ifndef SAVEASCIITEST_H_
#define SAVEASCIITEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataHandling/SaveAscii2.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FrameworkManager.h"
#include <fstream>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

// This test tests that SaveAscii produces a file of the expected form
// It does not test that the file can be loaded by loadAscii.
// The test LoadSaveAscii does that and should be run in addition to this test
// if you modify SaveAscii.

class SaveAscii2Test : public CxxTest::TestSuite
{

public:

  static SaveAscii2Test *createSuite() { return new SaveAscii2Test(); }
  static void destroySuite(SaveAscii2Test *suite) { delete suite; }

  SaveAscii2Test()
  {
    m_filename = "SaveAsciiTestFile.dat";
    m_filename_nohead = "SaveAsciiTestFileWithoutHeader.dat";
    m_name = "SaveAsciiWS";
  }
  ~SaveAscii2Test()
  {
  }

  void testExec()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT( Poco::File(filename).exists() );

    // Now make some checks on the content of the file
    std::ifstream in(m_filename.c_str());
    int specID;
    std::string header1, header2, header3, separator, comment;

    // Test that the first few column headers, separator and first two bins are as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> specID;
    TS_ASSERT_EQUALS(specID, 1 );
    TS_ASSERT_EQUALS(comment,"#" );
    TS_ASSERT_EQUALS(separator,"," );
    TS_ASSERT_EQUALS(header1,"X" );
    TS_ASSERT_EQUALS(header2,"Y" );
    TS_ASSERT_EQUALS(header3,"E" );

    std::string binlines;
    std::vector<std::string> binstr;
    std::vector<double> bins;
    std::getline(in,binlines);
    std::getline(in,binlines);

    boost::split(binstr, binlines,boost::is_any_of(","));
    for (int i = 0; i < binstr.size(); i++)
    {
      bins.push_back(boost::lexical_cast<double>(binstr.at(i)));
    }
    TS_ASSERT_EQUALS(bins[0], 0 );
    TS_ASSERT_EQUALS(bins[1], 2 );
    TS_ASSERT_EQUALS(bins[2], 1 );

    std::getline(in,binlines);
    bins.clear();
    boost::split(binstr, binlines,boost::is_any_of(","));
    for (int i = 0; i < binstr.size(); i++)
    {
      bins.push_back(boost::lexical_cast<double>(binstr.at(i)));
    }
    TS_ASSERT_EQUALS(bins[0], 1.66667 );
    TS_ASSERT_EQUALS(bins[1], 8.66667 );
    TS_ASSERT_EQUALS(bins[2], 1 );

    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void testExec_DX()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave = boost::dynamic_pointer_cast<
      Mantid::DataObjects::Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", 2, 3, 3));
    for (int i = 0; i < 2; i++)
    {
      std::vector<double>& X = wsToSave->dataX(i);
      std::vector<double>& Y = wsToSave->dataY(i);
      std::vector<double>& E = wsToSave->dataE(i);
      std::vector<double>& DX = wsToSave->dataDx(i);
      for (int j = 0; j < 3; j++)
      {
        X[j] = 1.5 * j / 0.9;
        Y[j] = (i + 1) * (2. + 4. * X[j]);
        E[j] = 1.;
        DX[j] = i + 1;
      }
    }

    AnalysisDataService::Instance().add(m_name, wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("WriteXError", "1"));
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT( Poco::File(filename).exists() );

    // Now make some checks on the content of the file
    std::ifstream in(m_filename.c_str());
    int specID;
    std::string header1, header2, header3, header4, separator, comment;

    // Test that the first few column headers, separator and first two bins are as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> separator >> header4 >> specID;
    TS_ASSERT_EQUALS(specID, 1 );
    TS_ASSERT_EQUALS(comment, "#");
    TS_ASSERT_EQUALS(separator, ",");
    TS_ASSERT_EQUALS(header1, "X");
    TS_ASSERT_EQUALS(header2, "Y");
    TS_ASSERT_EQUALS(header3, "E");
    TS_ASSERT_EQUALS(header4, "DX");

    std::string binlines;
    std::vector<std::string> binstr;
    std::vector<double> bins;
    std::getline(in,binlines);
    std::getline(in,binlines);

    boost::split(binstr, binlines,boost::is_any_of(","));
    for (int i = 0; i < binstr.size(); i++)
    {
      bins.push_back(boost::lexical_cast<double>(binstr.at(i)));
    }
    TS_ASSERT_EQUALS(bins[0], 0 );
    TS_ASSERT_EQUALS(bins[1], 2 );
    TS_ASSERT_EQUALS(bins[2], 1 );

    std::getline(in,binlines);
    bins.clear();
    boost::split(binstr, binlines,boost::is_any_of(","));
    for (int i = 0; i < binstr.size(); i++)
    {
      bins.push_back(boost::lexical_cast<double>(binstr.at(i)));
    }
    TS_ASSERT_EQUALS(bins[0], 1.66667 );
    TS_ASSERT_EQUALS(bins[1], 8.66667 );
    TS_ASSERT_EQUALS(bins[2], 1 );

    in.close();

    Poco::File(filename).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void testExec_no_header()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT( Poco::File(filename).exists() );

    save.setPropertyValue("Filename", m_filename_nohead);
    save.setPropertyValue("InputWorkspace", m_name);
    TS_ASSERT_THROWS_NOTHING(save.setProperty("ColumnHeader", false));
    std::string filename_nohead = save.getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT( Poco::File(filename_nohead).exists() );

    // Now we check that the first line of the file without header matches the second line of the file with header
    std::ifstream in1(m_filename.c_str());
    std::string line2header;
    std::ifstream in2(m_filename_nohead.c_str());
    std::string line1noheader;
    getline(in1,line2header);
    getline(in1,line2header);
    getline(in1,line2header); // 3rd line of file with header
    getline(in2,line1noheader);
    getline(in2,line1noheader); // 2nd line of file without header
    TS_ASSERT_EQUALS(line1noheader,line2header);
    in1.close();
    in2.close();
    // Remove files
    Poco::File(filename).remove();
    Poco::File(filename_nohead).remove();
    AnalysisDataService::Instance().remove(m_name);
  }

  void test_CustomSeparator_override()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    //Separator is left at default on purpouse to see if Customseparator overrides it
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CustomSeparator", "/" ));

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // has the algorithm written a file to disk?
    TS_ASSERT( Poco::File(filename).exists() );

    // Now make some checks on the content of the file
    std::ifstream in(m_filename.c_str());
    int specID;
    std::string header1, header2, header3, separator, comment;

    // Test that the first few column headers, separator and first two bins are as expected
    in >> comment >> header1 >> separator >> header2 >> separator >> header3 >> specID;
    TS_ASSERT_EQUALS(specID, 1 );
    TS_ASSERT_EQUALS(comment,"#" );
    //the algorithm will use a custom one if supplied even if the type selected is not "UserDefined"
    TS_ASSERT_EQUALS(separator,"/" );
    TS_ASSERT_EQUALS(header1,"X" );
    TS_ASSERT_EQUALS(header2,"Y" );
    TS_ASSERT_EQUALS(header3,"E" );

    in.close();
    Poco::File(filename).remove();

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_SpectrumList()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("SpectrumList", "2, 1"));

    TS_ASSERT_THROWS_ANYTHING(save.execute());

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(filename).exists() );

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_workspace()
  {
    SaveAscii2 save;
    save.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(save.initialize());
    TS_ASSERT(save.isInitialized());
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Filename", m_filename));
    std::string filename = save.getPropertyValue("Filename"); //Get absolute path
    TS_ASSERT_THROWS_ANYTHING(save.setPropertyValue("InputWorkspace", "NotARealWS"));
    TS_ASSERT_THROWS_ANYTHING(save.execute());

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(filename).exists() );
  }

  void test_fail_invalid_IndexMax()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("WorkspaceIndexMin", "1"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("WorkspaceIndexMax", "5"));

    TS_ASSERT_THROWS(save.execute(), std::invalid_argument);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(filename).exists() );

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_IndexMin()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_ANYTHING(save.setPropertyValue("WorkspaceIndexMin", "0"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("WorkspaceIndexMax", "2"));

    TS_ASSERT_THROWS(save.execute(), std::invalid_argument);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(filename).exists() );

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_IndexMin_Max_Overlap()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("WorkspaceIndexMin", "3"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("WorkspaceIndexMax", "2"));

    TS_ASSERT_THROWS(save.execute(), std::invalid_argument);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(filename).exists() );

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_SpectrumList()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_ANYTHING(save.setPropertyValue("SpectrumList", "2 3 1"));

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // the algorithm will have used a defualt and written a file to disk
    TS_ASSERT( Poco::File(filename).exists() );
    Poco::File(filename).remove();

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_SpectrumList_exceeds()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("SpectrumList", "2, 3, 1"));

    TS_ASSERT_THROWS(save.execute(),std::invalid_argument);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(filename).exists() );

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_Precision()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_ANYTHING(save.setPropertyValue("Precision", "-4"));

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // the algorithm will have used a defualt and written a file to disk
    TS_ASSERT( Poco::File(filename).exists() );
    Poco::File(filename).remove();

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_CommentIndicator_number()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CommentIndicator", "3" ));

    TS_ASSERT_THROWS(save.execute(), std::invalid_argument);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(filename).exists() );

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_CommentIndicator_e()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CommentIndicator", "e" ));

    TS_ASSERT_THROWS(save.execute(), std::invalid_argument);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(filename).exists() );

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_CommentIndicator_hyphen()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CommentIndicator", "-"));

    TS_ASSERT_THROWS(save.execute(), std::invalid_argument);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(filename).exists() );

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_CommentIndicator_plus()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CommentIndicator", "+"));

    TS_ASSERT_THROWS(save.execute(), std::invalid_argument);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(filename).exists() );

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_Separator_e()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Separator","UserDefined"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CustomSeparator","e"));

    TS_ASSERT_THROWS(save.execute(), std::invalid_argument);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(filename).exists() );

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_Separator_number()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Separator","UserDefined"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CustomSeparator","3"));

    TS_ASSERT_THROWS(save.execute(), std::invalid_argument);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(filename).exists() );

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_Separator_plus()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Separator","UserDefined"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CustomSeparator","+"));

    TS_ASSERT_THROWS(save.execute(), std::invalid_argument);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(filename).exists() );

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_Separator_hyphen()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Separator","UserDefined"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CustomSeparator","-"));

    TS_ASSERT_THROWS(save.execute(), std::invalid_argument);

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(filename).exists() );

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_invalid_Separator()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_ANYTHING(save.setPropertyValue("Separator", "NotAValidChoice"));

    TS_ASSERT_THROWS_NOTHING(save.execute());

    // the algorithm will have used a defualt and written a file to disk
    TS_ASSERT( Poco::File(filename).exists() );
    Poco::File(filename).remove();

    AnalysisDataService::Instance().remove(m_name);
  }

  void test_fail_clash_CustomSeparator_CustomComment()
  {
    Mantid::DataObjects::Workspace2D_sptr wsToSave;
    writeSampleWS(wsToSave);

    SaveAscii2 save;
    std::string filename = initSaveAscii2(save);

    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CommentIndicator", "@"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Separator", "UserDefined"));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("CustomSeparator", "@"));

    TS_ASSERT_THROWS_ANYTHING(save.execute());

    // the algorithm shouldn't have written a file to disk
    TS_ASSERT( !Poco::File(filename).exists() );

    AnalysisDataService::Instance().remove(m_name);
  }
private:

  void writeSampleWS(Mantid::DataObjects::Workspace2D_sptr & wsToSave)
  {
    wsToSave = boost::dynamic_pointer_cast<
      Mantid::DataObjects::Workspace2D>(WorkspaceFactory::Instance().create("Workspace2D", 2, 3, 3));
    for (int i = 0; i < 2; i++)
    {
      std::vector<double>& X = wsToSave->dataX(i);
      std::vector<double>& Y = wsToSave->dataY(i);
      std::vector<double>& E = wsToSave->dataE(i);
      for (int j = 0; j < 3; j++)
      {
        X[j] = 1.5 * j / 0.9;
        Y[j] = (i + 1) * (2. + 4. * X[j]);
        E[j] = 1.;
      }
    }

    AnalysisDataService::Instance().add(m_name, wsToSave);
  }

  std::string initSaveAscii2(SaveAscii2 & save)
  {
    save.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(save.initialize());
    TS_ASSERT(save.isInitialized());
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("Filename", m_filename));
    TS_ASSERT_THROWS_NOTHING(save.setPropertyValue("InputWorkspace", m_name));
    return save.getPropertyValue("Filename"); //return absolute path
  }

  std::string m_filename;
  std::string m_filename_nohead;
  std::string m_name;
};


#endif /*SAVEASCIITEST_H_*/
