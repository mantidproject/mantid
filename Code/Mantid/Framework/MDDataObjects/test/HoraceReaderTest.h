#ifndef MD_HORACE_READER_TEST_H
#define MD_HORACE_READER_TEST_H

#include <cxxtest/TestSuite.h>
#include "MDDataObjects/MD_FileFormatFactory.h"
#include <Poco/Path.h>
#include "MantidKernel/System.h"
#include "MantidAPI/FileFinder.h"

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
        cdp.detectors_start = 902;
        cdp.data_start      = 676815;
        cdp.n_cell_pix_start= 677439;
        cdp.pix_start       = 677771;
        //
        nTestDim = 4;
        nTestFiles=2;
        nTestPixels=580;

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
    std::string testFile = API::FileFinder::Instance().getFullPath("test_horace_reader.sqw");

    TSM_ASSERT_THROWS_NOTHING("Can not construct file reader, all tests will fail",spReader = 
      boost::shared_ptr<HoraceReaderTester>(new HoraceReaderTester(testFile.c_str())));
  }
  void testValuesReadCorrectly(){
    TSM_ASSERT_EQUALS("Number of values from the test file have not been read correctly",spReader->check_values_correct(),0);
  }
  void testGetNpixCorrect(){
    TSM_ASSERT_EQUALS("Not getting proper Number of pixels contiributed into dataset",580,spReader->getNPix());
  }

  void testReadBasis(){
    // this is currently hardcoded so no problem shouls be here but it will read crystall in a futute. 
	  pBasis = std::auto_ptr<Geometry::MDGeometryBasis>(new Geometry::MDGeometryBasis());
    TSM_ASSERT_THROWS_NOTHING("basis should be read without problem",spReader->read_basis(*pBasis));

  }
  void testReadGeometry(){
    // this constructor should be tested elsewhere
    TSM_ASSERT_THROWS_NOTHING("Geometry description should be able to build from basis ",pGeomDescription = std::auto_ptr<Geometry::MDGeometryDescription>(new Geometry::MDGeometryDescription(*pBasis)));
    // and here is the test of reading
    TS_ASSERT_THROWS_NOTHING(spReader->read_MDGeomDescription(*pGeomDescription));

    // verify what has been read;
  }
  void testReadMDImgData(){
    TSM_ASSERT_THROWS_NOTHING("MD Image has not been constructred by empty constructor",
		spImg=boost::shared_ptr<MDDataObjects::MDImage>(new MDDataObjects::MDImage(*pGeomDescription,*pBasis)));


    TSM_ASSERT_THROWS_NOTHING("MD image reader should not normaly throw",
      this->spReader->read_MDImg_data(*spImg));

    // check what has been read;	
    TSM_ASSERT_THROWS_NOTHING("Image control sums should be coorrect",spImg->validateNPix());
    //
    TSM_ASSERT_EQUALS("Image has to be consistent witn MD data points",spReader->getNPix(),spImg->getNMDDPoints());  

  }
   void testMDImageCorrect(){
         // if the image we've loaded is correct image (the same as we've put there)
     
         std::vector<point3D> img_data;
         std::vector<unsigned int> selection(2,0);

         spImg->getPointData(selection,img_data);
         double sum(0);
         for(size_t i=0;i<img_data.size();i++){
             sum+=img_data[i].S();
         }
         TSM_ASSERT_DELTA("The signal in this cell should be specified value",0,img_data[0 ].S(),1.e-4);
         TSM_ASSERT_DELTA("The signal in this cell should be specified value",0.3792,img_data[3].S(),1.e-4);
         TSM_ASSERT_DELTA("The signal in this cell should be specified value",0.0,    img_data[8].S(),1.e-4);
         TSM_ASSERT_DELTA("The sum of all signals in the signals selection should be specific value",0.3792,    sum,1.e-4);

         selection[0]=1;
         selection[1]=1;
         spImg->getPointData(selection,img_data);

         sum = 0;
         for(size_t i=0;i<img_data.size();i++){
             sum+=img_data[i].S();
         }

         TSM_ASSERT_DELTA("The signal in this cell should be specified value",0,      img_data[ 0].S(),1.e-4);
         TSM_ASSERT_DELTA("The signal in this cell should be specified value",0,img_data[ 3].S(),1.e-4);
         TSM_ASSERT_DELTA("The signal in this cell should be specified value",0,img_data[8].S(),1.e-4);

         TSM_ASSERT_DELTA("The sum of all signals in the signals selection should be specific value",0, sum,1.e-4);

  
     }
  void testReadAllPixels(){
    MDPointStructure  horPointInfo;
	horPointInfo.NumPixCompressionBits=0;
	horPointInfo.DimIDlength =4;
	horPointInfo.SignalLength=4;
	std::vector<std::string> dimID = this->pBasis->getBasisIDs();
	std::vector<std::string> dataID(9);
	dataID[0]=dimID[3];
	dataID[1]=dimID[2];
	dataID[2]=dimID[1];
	dataID[3]=dimID[0];
	dataID[4]="S";
	dataID[5]="Err";
	dataID[6]="PixID";
	dataID[7]="RunID";
	dataID[8]="enID";

	MDPointDescription horPointDescr(horPointInfo,dataID);

    MDDataPointsDescription pd(horPointDescr);
    MDDataPoints points(pd);
	points.initialize(spImg,spReader);
	bool pix_placed_in_memory;
    TSM_ASSERT_THROWS_NOTHING("Horace all pix reader should not throw",pix_placed_in_memory=spReader->read_pix(points,true));
	if(pix_placed_in_memory){
		//TSM_ASSERT_EQUALS(" All data are in memory so these values have to be equal",points.getNumPointsInMemory(),spReader->getNPix());

	}else{
		TS_FAIL("This test request enough memory to read all MDDPoints (~580*36 bytes)");
	}
  }
  void testReadPixelsSelectionAll(){
    // read all 
    int nCells = this->spImg->get_const_MDGeometry().getGeometryExtend();

    selected_cells.resize(nCells);
    pix_buf.resize(spReader->getNConributedPixels()*9*8);

    size_t starting_cell(0),n_cell_read;
    size_t n_pix_in_buffer(0);
    for(int i=0;i<nCells;i++){
      selected_cells[i]=i;
    }

    TSM_ASSERT_THROWS_NOTHING("Horace reader should not normaly throw",
      n_cell_read=this->spReader->read_pix_subset(*spImg,selected_cells,starting_cell,pix_buf, n_pix_in_buffer));

    // check if the data coinside with what was put there;
    TSM_ASSERT_EQUALS("Have not read all pixels epxected: ",580,n_pix_in_buffer);
    TSM_ASSERT_EQUALS("Have not read all cells epxected: ", 36,n_cell_read);
  }
  void testReadFirst2Selection(){
    // read first two (buffer is already allocated above)

    size_t starting_cell(0),n_cell_read;
    size_t n_pix_in_buffer(0);

    selected_cells.resize(2);
    selected_cells[0]=0;
    selected_cells[1]=3;


    TSM_ASSERT_THROWS_NOTHING("Horace reader should not normaly throw",
      n_cell_read=this->spReader->read_pix_subset(*spImg,selected_cells,starting_cell,pix_buf, n_pix_in_buffer));

    // check if the data coinside with what was put there;
    TSM_ASSERT_EQUALS("Have not read all pixels epxected: ",300,n_pix_in_buffer);
    TSM_ASSERT_EQUALS("Have not read all cells epxected: ", 2,n_cell_read);
  }
  void testReadOneSelection(){
    // read one (buffer is already allocated above)

    size_t starting_cell(0),n_cell_read;
    size_t n_pix_in_buffer(0);

    selected_cells.resize(1);
    selected_cells[0]=3;


    TSM_ASSERT_THROWS_NOTHING("Horace reader should not normaly throw",
      n_cell_read=this->spReader->read_pix_subset(*spImg,selected_cells,starting_cell,pix_buf, n_pix_in_buffer));

    // check if the data coinside with what was put there;
    TSM_ASSERT_EQUALS("Have not read all pixels epxected: ",300,n_pix_in_buffer);
    TSM_ASSERT_EQUALS("Have not read all cells epxected: ", 1,n_cell_read);
  }
  void testRead2Selection(){
    // read random two (buffer is already allocated above)
    size_t starting_cell(0),n_cell_read;
    size_t n_pix_in_buffer(0);

    selected_cells.resize(2);
    selected_cells[0]=3;
    selected_cells[1]=7;


    TSM_ASSERT_THROWS_NOTHING("Horace reader should not normaly throw",
      n_cell_read=this->spReader->read_pix_subset(*spImg,selected_cells,starting_cell,pix_buf, n_pix_in_buffer));

    // check if the data coinside with what was put there;
    TSM_ASSERT_EQUALS("Have not read all pixels epxected: ",300,n_pix_in_buffer);
    TSM_ASSERT_EQUALS("Have not read all cells epxected: ", 2,n_cell_read);

  }

  void testReadFirstLastSelection(){
    size_t starting_cell(0),n_cell_read;
    size_t n_pix_in_buffer(0);

    selected_cells[0]=0;
    selected_cells[1]=this->spImg->get_const_MDGeometry().getGeometryExtend()-1;

    TSM_ASSERT_THROWS_NOTHING("Horace reader should not normaly throw",
      n_cell_read=this->spReader->read_pix_subset(*spImg,selected_cells,starting_cell,pix_buf, n_pix_in_buffer));

    // check if the data coinside with what was put there;
    TSM_ASSERT_EQUALS("Have not read all pixels epxected: ",0,n_pix_in_buffer);
    TSM_ASSERT_EQUALS("Have not read all cells epxected: ", 2,n_cell_read);
  }

  void testReadSmallBufferSelectionResized(){
    // read random two (buffer is already allocated above)
    size_t starting_cell(0),n_cell_read;
    size_t n_pix_in_buffer(0);

    selected_cells[0]=3;
    selected_cells[1]=10;
    pix_buf.resize(100);

    TSM_ASSERT_THROWS_NOTHING("Horace reader should not normaly throw",
      n_cell_read=this->spReader->read_pix_subset(*spImg,selected_cells,starting_cell,pix_buf, n_pix_in_buffer));

    // check if the data coinside with what was put there;
    TSM_ASSERT_EQUALS("Have not read all pixels epxected: ",300,n_pix_in_buffer);
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
        starting_cell=this->spReader->read_pix_subset(*spImg,selected_cells,starting_cell,pix_buf, n_pix_in_buffer));
      ic++;
    }
    // check if the data coinside with what was put there;
    // the buffer size defived from the largest cell exceeding 100
    TSM_ASSERT_EQUALS("Data buffer size differs from expected: ", 300*9*4,pix_buf.size());
    TSM_ASSERT_EQUALS("Number of cells read differs from expected: ",selected_cells.size(),starting_cell);
    TSM_ASSERT_EQUALS("Number of read attempts differs from expected: ",2,ic);
  }


  void testWriteMDD(){
    TSM_ASSERT_THROWS("Looks like Horace writer has been implemented, why? ",spReader->write_mdd(*spImg),Kernel::Exception::NotImplementedError);
  }

private:
  boost::shared_ptr<HoraceReaderTester> spReader;
  // the components of the workspace which the reader supplies with data
  std::auto_ptr<Geometry::MDGeometryBasis> pBasis;
  std::auto_ptr<Geometry::MDGeometryDescription> pGeomDescription;
  boost::shared_ptr<MDDataObjects::MDImage> spImg;
  //******************************************************************
  std::vector<size_t> selected_cells;
  std::vector<char> pix_buf;
};

#endif
