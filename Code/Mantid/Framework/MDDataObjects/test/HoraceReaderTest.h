#ifndef MD_HORACE_READER_TEST_H
#define MD_HORACE_READER_TEST_H

#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MD_FileFormatFactory.h"
#include <Poco/Path.h>
#include "MantidKernel/System.h"
#include "MantidKernel/ConfigService.h"

#include "MDDataObjects/MD_FileHoraceReader.h"
#include "MDDataObjects/MDImage.h"
#include "MDDataObjects/MDDataPoints.h"
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
	  size_t getNConributedPixels()const{return nTestPixels;}
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
        std::string instrumentPath = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
		std::string test_file;
		std::string testFile = findTestFileLocation("../../../../Test/AutoTestData/test_horace_reader.sqw");

		TSM_ASSERT_THROWS_NOTHING("Can not construct file reader, all tests will fail",pReader = 
			std::auto_ptr<HoraceReaderTester>(new HoraceReaderTester(testFile.c_str())));
	}
	void testValuesReadCorrectly(){
		TSM_ASSERT_EQUALS("Number of values from the test file have not been read correctly",pReader->check_values_correct(),0);
	}
	void testGetNpixCorrect(){
		TSM_ASSERT_EQUALS("Not getting proper Number of pixels contiributed into dataset",1523850,pReader->getNPix());
	}

	void testReadBasis(){
		// this is currently hardcoded so no problem shouls be here but it will read crystall in a futute. 
		TSM_ASSERT_THROWS_NOTHING("basis should be read without problem",pReader->read_basis(basis));

	}
	void testReadGeometry(){
		// this constructor should be tested elsewhere
		TSM_ASSERT_THROWS_NOTHING("Geometry description should be able to build from basis ",pGeomDescription = std::auto_ptr<Geometry::MDGeometryDescription>(new Geometry::MDGeometryDescription(basis)));
		// and here is the test of reading
		TS_ASSERT_THROWS_NOTHING(pReader->read_MDGeomDescription(*pGeomDescription));

		// verify what has been read;
	}
	void testReadMDImgData(){
		TSM_ASSERT_THROWS_NOTHING("MD Image has not been constructred by empty constructor",
			pImg=std::auto_ptr<MDDataObjects::MDImage>(new MDDataObjects::MDImage(*pGeomDescription,basis)));


		TSM_ASSERT_THROWS_NOTHING("MD image reader should not normaly throw",
			this->pReader->read_MDImg_data(*pImg));

        // check what has been read;
	}
	void testReadAllPixels(){
		MDPointDescription defaultDescr;
		boost::shared_ptr<MDImage const> emptyImg = boost::shared_ptr<MDImage const>(new MDImage());
		MDDataPointsDescription pd(defaultDescr);
		MDDataPoints points(pd);
		TSM_ASSERT_THROWS("You implemented the Horace all_pix reader, write test for it",pReader->read_pix(points),Kernel::Exception::NotImplementedError);
	}
	void testReadPixelsSelectionAll(){
		// read all 
		int nCells = this->pImg->getGeometry()->getGeometryExtend();

    	selected_cells.resize(nCells);
        pix_buf.resize(pReader->getNConributedPixels()*9*8);

		size_t starting_cell(0),n_cell_read;
		size_t n_pix_in_buffer(0);
		for(int i=0;i<nCells;i++){
			selected_cells[i]=i;
		}

		TSM_ASSERT_THROWS_NOTHING("Horace reader should not normaly throw",
			n_cell_read=this->pReader->read_pix_subset(*pImg,selected_cells,starting_cell,pix_buf, n_pix_in_buffer));

		// check if the data coinside with what was put there;
        TSM_ASSERT_EQUALS("Have not read all pixels epxected: ",1523850,n_pix_in_buffer);
        TSM_ASSERT_EQUALS("Have not read all cells epxected: ", 64,n_cell_read);
	}
	void testReadFirst2Selection(){
		// read first two (buffer is already allocated above)
	
		size_t starting_cell(0),n_cell_read;
		size_t n_pix_in_buffer(0);

        selected_cells.resize(2);
	    selected_cells[0]=0;
	    selected_cells[1]=1;
   

		TSM_ASSERT_THROWS_NOTHING("Horace reader should not normaly throw",
			n_cell_read=this->pReader->read_pix_subset(*pImg,selected_cells,starting_cell,pix_buf, n_pix_in_buffer));

		// check if the data coinside with what was put there;
        TSM_ASSERT_EQUALS("Have not read all pixels epxected: ",77292,n_pix_in_buffer);
        TSM_ASSERT_EQUALS("Have not read all cells epxected: ", 2,n_cell_read);
	}
	void testReadOneSelection(){
		// read one (buffer is already allocated above)
	
		size_t starting_cell(0),n_cell_read;
		size_t n_pix_in_buffer(0);

        selected_cells.resize(1);
	    selected_cells[0]=3;


		TSM_ASSERT_THROWS_NOTHING("Horace reader should not normaly throw",
			n_cell_read=this->pReader->read_pix_subset(*pImg,selected_cells,starting_cell,pix_buf, n_pix_in_buffer));

		// check if the data coinside with what was put there;
        TSM_ASSERT_EQUALS("Have not read all pixels epxected: ",1,n_pix_in_buffer);
        TSM_ASSERT_EQUALS("Have not read all cells epxected: ", 1,n_cell_read);
	}
	void testRead2Selection(){
	// read random two (buffer is already allocated above)
		size_t starting_cell(0),n_cell_read;
		size_t n_pix_in_buffer(0);

        selected_cells.resize(2);
	    selected_cells[0]=3;
	    selected_cells[1]=10;
   

		TSM_ASSERT_THROWS_NOTHING("Horace reader should not normaly throw",
			n_cell_read=this->pReader->read_pix_subset(*pImg,selected_cells,starting_cell,pix_buf, n_pix_in_buffer));

		// check if the data coinside with what was put there;
       TSM_ASSERT_EQUALS("Have not read all pixels epxected: ",447,n_pix_in_buffer);
       TSM_ASSERT_EQUALS("Have not read all cells epxected: ", 2,n_cell_read);

	}

	void testReadFirstLastSelection(){
		size_t starting_cell(0),n_cell_read;
		size_t n_pix_in_buffer(0);

        selected_cells[0]=0;
        selected_cells[1]=this->pImg->getGeometry()->getGeometryExtend()-1;

		TSM_ASSERT_THROWS_NOTHING("Horace reader should not normaly throw",
			n_cell_read=this->pReader->read_pix_subset(*pImg,selected_cells,starting_cell,pix_buf, n_pix_in_buffer));

		// check if the data coinside with what was put there;
        TSM_ASSERT_EQUALS("Have not read all pixels epxected: ",21496,n_pix_in_buffer);
        TSM_ASSERT_EQUALS("Have not read all cells epxected: ", 2,n_cell_read);
	}
	
	void testReadSmallBufferSelectionResized(){
	// read random two (buffer is already allocated above)
		size_t starting_cell(0),n_cell_read;
		size_t n_pix_in_buffer(0);

	    selected_cells[0]=1;
	    selected_cells[1]=10;
        pix_buf.resize(100);

		TSM_ASSERT_THROWS_NOTHING("Horace reader should not normaly throw",
			n_cell_read=this->pReader->read_pix_subset(*pImg,selected_cells,starting_cell,pix_buf, n_pix_in_buffer));

		// check if the data coinside with what was put there;
        TSM_ASSERT_EQUALS("Have not read all pixels epxected: ",63006,n_pix_in_buffer);
        TSM_ASSERT_EQUALS("Have not read all cells epxected: ", 1,n_cell_read);
        TSM_ASSERT_EQUALS("Data buffer size differs from expected: ", n_pix_in_buffer*9*4,pix_buf.size());

	}
	void testReadSmallBufferSelection(){
	// read random two (buffer is already allocated above)
		size_t starting_cell(0);
		size_t n_pix_in_buffer(0);

        selected_cells.resize(3);
        // cells should be usually sorted for efficiency but will do for the time being;
        selected_cells[0]=3;
	    selected_cells[1]=1;
	    selected_cells[2]=10;

        pix_buf.resize(100);

        unsigned int ic(0);
        while(starting_cell<selected_cells.size()){
		TSM_ASSERT_THROWS_NOTHING("Horace reader should not normaly throw",
			starting_cell=this->pReader->read_pix_subset(*pImg,selected_cells,starting_cell,pix_buf, n_pix_in_buffer));
            ic++;
        }
		// check if the data coinside with what was put there;
        // the buffer size defived from the largest cell exceeding 100
       TSM_ASSERT_EQUALS("Data buffer size differs from expected: ", 63006*9*4,pix_buf.size());
       TSM_ASSERT_EQUALS("Number of cells read differs from expected: ",selected_cells.size(),starting_cell);
       TSM_ASSERT_EQUALS("Number of read attempts differs from expected: ",3,ic);
	}


	void testWriteMDD(){
		TSM_ASSERT_THROWS("Looks like Horace writer has been implemented, why? ",pReader->write_mdd(*pImg),Kernel::Exception::NotImplementedError);
	}

private:
	std::auto_ptr<HoraceReaderTester> pReader;
	// the components of the workspace which the reader supplies with data
    Geometry::MDGeometryBasis basis;
	std::auto_ptr<Geometry::MDGeometryDescription> pGeomDescription;
	std::auto_ptr<MDDataObjects::MDImage> pImg;
//******************************************************************
	std::vector<size_t> selected_cells;
    std::vector<char> pix_buf;

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
