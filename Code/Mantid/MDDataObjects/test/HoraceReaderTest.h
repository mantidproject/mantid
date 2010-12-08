#ifndef MD_HORACE_READER_TEST_H
#define MD_HORACE_READER_TEST_H

#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MD_FileFormatFactory.h"
#include "Poco/Path.h"
#include "MantidKernel/System.h"

#include "MDDataObjects/MD_FileHoraceReader.h"
#include <boost/algorithm/string/case_conv.hpp>

using namespace Mantid;
using namespace MDDataObjects;

class HoraceReaderTester: public HoraceReader::MD_FileHoraceReader
{
public: 
	HoraceReaderTester(const char *file_name):
	  HoraceReader::MD_FileHoraceReader(file_name)
	{

		// proper field should start from: 
		cdp.if_sqw_start = 18;
		cdp.n_dims_start = 22;
		cdp.sqw_header_start=26;
		//cdp.component_headers_starts //= 106; 2 contributing files
		cdp.detectors_start = 906;
		cdp.data_start      = 676819;
		cdp.n_cell_pix_start= 677679;
		cdp.pix_start       = 678235;
		//
		nTestDim = 4;
		nTestFiles=2;
		nTestPixels=1523850;

	 }
	int check_values_correct(){
		int rez = 0;
		if(cdp.if_sqw_start != positions.if_sqw_start)          {std::cerr<<" pixels location differs from expected"      <<positions.if_sqw_start
			                                                                                              <<" expected: " <<cdp.if_sqw_start<<std::endl; rez++;}
		if(cdp.n_dims_start != positions.n_dims_start)          {std::cerr<<" n_dims location differs from expected"      <<positions.n_dims_start
			                                                                                              <<" expected: " <<cdp.n_dims_start<<std::endl;  rez++;}
		if(cdp.sqw_header_start != positions.sqw_header_start)  {std::cerr<<" sqw_header location differs from expected"  <<positions.sqw_header_start
			                                                                                              <<" expected: " <<cdp.sqw_header_start<<std::endl;rez++;}
		if(cdp.detectors_start != positions.detectors_start)    {std::cerr<<" detectors location differs from expected"   <<positions.detectors_start
			                                                                                              <<" expected: " <<cdp.detectors_start<<std::endl; rez++;}
		if(cdp.data_start != positions.data_start)              {std::cerr<<" data location differs from expected"        <<positions.data_start
			                                                                                              <<" expected: " <<cdp.data_start<<std::endl; rez++;}
		if(cdp.n_cell_pix_start != positions.n_cell_pix_start)  {std::cerr<<" cells pixels location differs from expected"<<positions.n_cell_pix_start
			                                                                                              <<" expected: " <<cdp.n_cell_pix_start<<std::endl; rez++;}
		if(cdp.pix_start != positions.pix_start)                {std::cerr<<" pixels location differs from expected"      <<positions.pix_start
			                                                                                              <<" expected: " <<cdp.pix_start<<std::endl; rez++;}


	     if(nTestDim    != this->nDims)                          {std::cerr<<" number of dimensions differs from expected"<<this->nDims
			                                                                                               <<" expected: "<< nTestDim<<std::endl; rez++;}
	     if(nTestFiles  != positions.component_headers_starts.size())  {std::cerr<<" number of contributing files differs from expected"<<positions.component_headers_starts.size()
			                                                                                               <<" expected: "<<nTestFiles<<std::endl; rez++;}
	     if(nTestPixels != this->nDataPoints)                    {std::cerr<<" number of dataPoints differs  from expected"<<this->nDataPoints 
			                                                                                               <<" expected: "<< nTestPixels<<std::endl; rez++;}
		return rez;
	}
private:
	// correct data positions in the file test_horace_reader.sqw;
	HoraceReader::data_positions cdp;
	int nTestDim,nTestFiles,nTestPixels;
};

class HoraceReaderTest :    public CxxTest::TestSuite
{
public:
	void testConstructor(){
		std::string test_file;
		std::string testFile = findTestFileLocation("../../../../Test/AutoTestData/test_horace_reader.sqw");

		TSM_ASSERT_THROWS_NOTHING("Can not construct file reader, all tests will fail",pReader = 
			std::auto_ptr<HoraceReaderTester>(new HoraceReaderTester(testFile.c_str())));
	}
	void testValuesReadCorrectly(){
		TSM_ASSERT_EQUALS("Number of values from the test file have not been read correctly",pReader->check_values_correct(),0);
	}
private:
	std::auto_ptr<HoraceReaderTester> pReader;
//******************************************************************
   std::string findTestFileLocation(const char *filePath=NULL){

    std::string search_path = Mantid::Kernel::getDirectoryOfExecutable();
	std::string real_path   = search_path;
	boost::to_upper(search_path);

    char pps[2];
    pps[0]=Poco::Path::separator();
    pps[1]=0; 
    std::string sps(pps);
	std::string fileName = "test_horace_reader.sqw";
	

    std::string root_path;
    size_t   nPos = search_path.find("MANTID"+sps+"CODE");

	if(nPos==std::string::npos){
      std::cout <<" can not identify application location\n";
	  root_path.assign(filePath);
    }else{
      root_path=real_path.substr(0,nPos)+"Mantid/Test/AutoTestData/"+fileName;
    }
    std::cout << "\n\n test file location: "<< root_path<< std::endl;
    return root_path;
  }
};

#endif