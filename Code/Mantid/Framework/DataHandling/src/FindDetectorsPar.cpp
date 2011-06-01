#include "MantidDataHandling/FindDetectorsPar.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/TableRow.h"
#include <limits>
#include <iostream>

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FindDetectorsPar)

    //----------------------------------------------------------------
Kernel::Logger& FindDetectorsPar::g_log=Kernel::Logger::get("DataHandling");

/// Sets documentation strings for this algorithm
void FindDetectorsPar::initDocs()
{
  this->setWikiSummary("The algorithm [[FindDetectorsPar]] returns the angular parameters and second flight path for a workspace detectors (data, usually availble in par or phx file)");
  this->setOptionalMessage("The algorithm returns the angular parameters and second flight path for a workspace detectors (data, usually availble in par or phx file)");
}


using namespace Kernel;
using namespace API;
// nothing here according to mantid
FindDetectorsPar::FindDetectorsPar(){};
FindDetectorsPar::~FindDetectorsPar(){};

void FindDetectorsPar::init()
{
    CompositeValidator<> * wsValidator = new CompositeValidator<> ;
      wsValidator->add(new API::InstrumentValidator<>);
      wsValidator->add(new API::CommonBinsValidator<>);
  // input workspace
  declareProperty(
     new WorkspaceProperty<>("InputWorkspace","", Direction::Input,wsValidator),
    "The name of the workspace that will be used as input for the algorithm" );
 // optional par or phx file
  std::vector<std::string> fileExts(2);
    fileExts[0]=".par";
    fileExts[1]=".phx";
    declareProperty(new FileProperty("ParFile","not_used.par",FileProperty::OptionalLoad, fileExts),
    "An optional file that contains of the list of angular parameters for the detectors and detectors groups;\n"
    "If specified, will use data from file instead of the data, calculated from the instument description");

  //
  // Outputs:
  declareProperty("OutputParTable","","If not empty, a table workspace of that "
      "name will contain the calculated par-values for the detectors");

}

void 
FindDetectorsPar::exec()
{

 // Get the input workspace
  const MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  if(inputWS.get()==NULL){
      throw(Kernel::Exception::NotFoundError("can not obtain InoputWorkspace for the algorithm to work",""));
  }
 // Number of spectra
  const size_t nHist = inputWS->getNumberHistograms();

  // try to load par file if one is availible
  std::string fileName = this->getProperty("ParFile");
  if(fileName!="not_used.par"){
      size_t nPars = loadParFile(fileName);
      if(nPars == nHist){
          populate_values_from_file();
          return;
      }else{
          g_log.information()<<" number of parameters in the file: "<<fileName<<"  not equal to the nuber of histohramm in the workspace"
                              << inputWS->getName()<<std::endl;
          g_log.information()<<" trying to calculate detectors parameters algorithmically\n";
      }
  }
  
   
  // Get a pointer to the sample
   Geometry::IObjComponent_const_sptr sample =inputWS->getInstrument()->getSample();


  azimuthal.assign(nHist,std::numeric_limits<double>::quiet_NaN());
  polar.assign(nHist,std::numeric_limits<double>::quiet_NaN());
  azimuthal_width.assign(nHist,std::numeric_limits<double>::quiet_NaN());
  polar_width.assign(nHist,std::numeric_limits<double>::quiet_NaN());
  secondary_flightpath.assign(nHist,std::numeric_limits<double>::quiet_NaN());

   Progress progress(this,0,1,100);
   const int progStep = (int)(ceil(double(nHist)/100.0));
        
   // Loop over the spectra
   for (size_t i = 0; i < nHist; i++)
   {
     Geometry::IDetector_sptr spDet;
     try{
        spDet= inputWS->getDetector(i);
     }catch(Kernel::Exception::NotFoundError &){
        continue;
     }
    // Check that we aren't writing a monitor...
    if (!spDet->isMonitor()){             

          Geometry::det_topology group_shape= spDet->getTopology();
          if(group_shape == Geometry::cyl){  // we have a ring;
              calc_cylDetPar(spDet,sample,azimuthal[i], polar[i], 
                                          azimuthal_width[i], polar_width[i],secondary_flightpath[i]);
          }else{  // we have a detector or a rectangular shape
              calc_rectDetPar(inputWS,spDet,sample,azimuthal[i],polar[i],
                                                   azimuthal_width[i],polar_width[i],secondary_flightpath[i]);
          }
       }
        // make regular progress reports and check for canceling the algorithm
     if ( i % progStep == 0 ){
           progress.report();
     }
   }
   std::string output = getProperty("OutputParTable");
   if(!output.empty()){
    // Store the result in a table workspace
     try{
      declareProperty(new WorkspaceProperty<API::ITableWorkspace>("OutputParTableWS","",Direction::Output));
     }catch(std::exception &err){
         g_log.information()<<" findDetecotorsPar: error while declaring property: OutputParTableWS\n";
         g_log.information()<<" findDetecotorsPar: the error is: "<<err.what()<<std::endl;
     }

     // Set the name of the new workspace
      setPropertyValue("OutputParTableWS",output);

      Mantid::API::ITableWorkspace_sptr m_result = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
       m_result->addColumn("double","azimuthal");
       m_result->addColumn("double","polar");
       m_result->addColumn("double","secondary_flightpath");
       m_result->addColumn("double","azimuthal_width");
       m_result->addColumn("double","polar_width");

       for(size_t i=0;i<azimuthal.size();i++){
           Mantid::API::TableRow row = m_result->appendRow();
           row << azimuthal[i] << polar[i] << secondary_flightpath[i] << azimuthal_width[i] << polar_width[i];
       }
       setProperty("OutputParTableWS",m_result);
  
   }

   

}
// Constant for converting Radians to Degrees
const double rad2deg = 180.0 / M_PI;

void 
FindDetectorsPar::calc_cylDetPar(const Geometry::IDetector_sptr spDet,const Geometry::IObjComponent_const_sptr sample,
                                 double &azim, double &polar, double &azim_width, double &polar_width,double &dist)
{
        // polar values are constants for ring;
        azim_width= 0;
        azim      = 0;

        // accumulators;
        double d1_min(FLT_MAX);
        double d1_max(-FLT_MAX);
        double angle_sum(0);
        double dist_sum(0);

        std::vector<Geometry::V3D> coord(3);

        // get vector leading from the sample to the ring centre 
        Geometry::V3D GroupCenter   = spDet->getPos();
        Geometry::V3D Observer      = sample->getPos();
        coord[1]  = (GroupCenter-Observer);
        double d0 = coord[1].norm();
        coord[1] /= d0;
        // access contribured detectors;
        Geometry::DetectorGroup * pDetGroup = dynamic_cast<Geometry::DetectorGroup *>(spDet.get());
        if(!pDetGroup){
             g_log.error()<<"calc_cylDetPar: can not downcast IDetector_sptr to detector group for det->ID: "<<spDet->getID()<<std::endl;
             throw(std::bad_cast());
        }
        std::vector<Geometry::IDetector_sptr> pDets = pDetGroup->getDetectors();
        Geometry::BoundingBox bbox;

        // loop through all detectors in the group 
        for(size_t i=0;i<pDets.size();i++){
            Geometry::V3D center= pDets[i]->getPos();
            coord[0]  = center-GroupCenter;
            double d1 = coord[0].norm();
            coord[0] /= d1;
            coord[2]  = coord[0].cross_prod(coord[1]);

             // obtain the bounding box, aligned accordingly to the coordinates;
            bbox.setBoxAlignment(center,coord);
            pDets[i]->getBoundingBox(bbox);

            double d_min = d1+bbox.xMin();  if(d_min<d1_min)d1_min = d_min;
            double d_max = d1+bbox.xMax();  if(d_max>d1_max)d1_max = d_max;

            angle_sum+=atan2(d1,d0);
            dist_sum +=d1*d1+d0*d0;
        }

        polar_width = (atan2(d1_max,d0)-atan2(d1_min,d0))*rad2deg;
        polar       = (angle_sum/double(pDets.size()))*rad2deg;
        dist        = sqrt(dist_sum/double(pDets.size()));
        
}

void 
FindDetectorsPar::calc_rectDetPar(const API::MatrixWorkspace_sptr inputWS, 
                                 const Geometry::IDetector_sptr spDet,const Geometry::IObjComponent_const_sptr sample,
                                 double &azim, double &polar, double &azim_width, double &polar_width,double &dist)
{
    // Get Sample->Detector distance
     dist     =  spDet->getDistance(*sample);
     polar    =  inputWS->detectorTwoTheta(spDet)*rad2deg;
     azim     =  spDet->getPhi()*rad2deg;    
    // Now let's work out the detector widths
    // TODO: This is the historically wrong method...update it!
    // Get the bounding box
    Geometry::BoundingBox bbox;
    spDet->getBoundingBox(bbox);
    double xsize = bbox.xMax() - bbox.xMin();
    double ysize = bbox.yMax() - bbox.yMin();

    polar_width  = atan2((ysize/2.0), dist)*rad2deg;
    azim_width   = atan2((xsize/2.0), dist)*rad2deg;
}
void 
FindDetectorsPar::fill_property(Kernel::Property *const pProperty,std::vector<double> const&data)
{
    std::stringstream Buf;
    if(!data.empty()){
        Buf<<data[0];
    }else{
        pProperty->setValue("");
        return;
    }
    for(size_t i=1;i<data.size();i++){
        Buf<<","<<data[i];
    }
    Buf<<std::endl;
    pProperty->setValue( Buf.str());
}
//
size_t 
FindDetectorsPar::loadParFile(const std::string &fileName){
    g_log.information()<<" load par file "<<fileName<<" is not implemented at the moment\n";
    return 0;
}
// 
void 
FindDetectorsPar::populate_values_from_file()
{
}
//
int 
FindDetectorsPar::count_changes(const char *const Buf,size_t buf_size)
{
    bool is_symbol(false),is_space(true);
    int  space_to_symbol_change(0),symbol_to_space_change(0);
    size_t symbols_start(0);
    // supress leading spaces;
    for(size_t i=0;i<buf_size;i++){
       if(Buf[i]==0)break;
       if(Buf[i]==' '){
            continue;
       }else{
          symbols_start=i;
          break;
       }
    }
    // calculate number of changes from space to symbol assuming start from symbol;
    for(size_t i=symbols_start;i<buf_size;i++){
        if(Buf[i]==0)break;
        if(Buf[i]>='+'&&Buf[i]<='z'){  // this is a symbol
            if(is_space){
                is_space=false;
                space_to_symbol_change++;
            }
            is_symbol=true;
        }
        if(Buf[i]==' '){  // this is a space
            if(is_symbol){
                is_symbol=false;
                symbol_to_space_change++;
            }
            is_space =true;
        }
    }
    return space_to_symbol_change;
}

/**! The function reads line from inout stream and puts it into buffer. 
*   It behaves like std::ifstream getline but the getline reads additional symbol from a row in a Unix-formatted file under windows;
*/
size_t
FindDetectorsPar::get_my_line(std::ifstream &in, char *buf, size_t buf_size,const char DELIM)
{
    size_t i;
    for(i=0;i<buf_size;i++){
        in.get(buf[i]);
        if(buf[i]==DELIM){	
            buf[i]=0;
            return i;
        }
    }
    buf[buf_size-1]=0;
    g_log.information()<<" data obtained from ASCII data file trunkated to "<<buf_size<<" characters\n";
    return buf_size;
}
/**!
 *  The function loads ASCII file header and tries to identify the type of the header.
 *  Possible types are
 *  SPE, PAR or PHS
 *
 *  if none three above identified, returns "undefined" type
 *  it also returns the FileTypeDescriptor, which identifyes the position of the data in correcponding ASCII file 
 *  plus characteristics of the data extracted from correspondent data header. 
*/
FileTypeDescriptor 
FindDetectorsPar::get_ASCII_header(std::string const &fileName, std::ifstream &data_stream)
{
    std::vector<char> BUF(1024);
    FileTypeDescriptor file_descriptor;
    file_descriptor.Type = NumFileTypes; // set the autotype to invalid

    data_stream.open(fileName.c_str(),std::ios_base::in|std::ios_base::binary);
    if(!data_stream.is_open()){
        g_log.error()<<" can not open existing ASCII data file: "<<fileName<<std::endl;
        throw(Kernel::Exception::FileError(" Can not open existing input data file",fileName));
    }
    // let's identify the EOL symbol; As the file may have been prepared on different OS, from where you are reading it 
    // and no conversion have been performed; 
    char symbol;
    data_stream.get(symbol);
    while(symbol>0x1F){
        data_stream.get(symbol);
    }
    char EOL;
    if(symbol==0x0D){ // Win or old Mac file
            data_stream.get(symbol);
            if(symbol==0x0A){ // Windows file
                EOL=0x0A;
            }else{            // Mac
                EOL=0x0D;
                data_stream.putback(symbol);
            }
    }else if(symbol==0x0A){   // unix file. 
        EOL=0x0A;
    }else{
        g_log.error()<<" Error reading the first row of the input ASCII data file: "<<fileName<<" as it contains unprintable characters\n";
        throw(Kernel::Exception::FileError(" Error reading the first row of the input ASCII data file, as it contains unprintable characters",fileName));
    }

    file_descriptor.line_end=EOL;
    data_stream.seekg(0,std::ios::beg);


    get_my_line(data_stream,&BUF[0],BUF.size(),EOL);
    if(!data_stream.good()){  
        g_log.error()<<" Error reading the first row of the input data file "<<fileName<<", It may be bigger then 1024 symbols\n";
        throw(Kernel::Exception::FileError(" Error reading the first row of the input data file, It may be bigger then 1024 symbols",fileName));
    }

    //let's find if there is one or more groups of symbols inside of the buffer;
    int space_to_symbol_change=count_changes(&BUF[0],BUF.size());
    if(space_to_symbol_change>1){  // more then one group of symbols in the string, spe file
        int nDatas = sscanf(&BUF[0]," %d %d ",&file_descriptor.nData_records,&file_descriptor.nData_blocks);
        if(nDatas!=2){    			
            g_log.error()<<" File "<<fileName<<" iterpreted as SPE but does not have two numbers in the first row\n";
            throw(Kernel::Exception::FileError(" File iterpreted as SPE but does not have two numbers in the first row",fileName));
        }
        file_descriptor.Type=SPE_type;
        get_my_line(data_stream,&BUF[0],BUF.size(),EOL);
        if(BUF[0]!='#'){ 
            g_log.error()<<" File "<<fileName<<"iterpreted as SPE does not have symbol # in the second row\n";
            throw(Kernel::Exception::FileError(" File iterpreted as SPE does not have symbol # in the second row",fileName));
        }
        file_descriptor.data_start_position = data_stream.tellg(); // if it is SPE file then the data begin after the second line;
    }else{
        file_descriptor.data_start_position = data_stream.tellg(); // if it is PHX or PAR file then the data begin after the first line;
        file_descriptor.nData_records       = atoi(&BUF[0]);
        file_descriptor.nData_blocks        = 0;

        // let's ifendify now if is PHX or PAR file;
        data_stream.getline(&BUF[0],BUF.size(),EOL);

        int space_to_symbol_change=count_changes(&BUF[0],BUF.size());
        if(space_to_symbol_change==6||space_to_symbol_change==5){       // PAR file
                file_descriptor.Type         = PAR_type;
                file_descriptor.nData_blocks = space_to_symbol_change;
        }else if(space_to_symbol_change==7){ // PHX file
                file_descriptor.Type=PHX_type;
                file_descriptor.nData_blocks = space_to_symbol_change;
        }else{   // something unclear or damaged
            g_log.error()<<" can not identify format of the input data file "<<fileName<<std::endl;
            throw(Kernel::Exception::FileError(" can not identify format of the input data file",fileName));
        }

    }
    return file_descriptor;
}

/*!
 *  function to load PHX or PAR file
 *  the file should be already opened and the FILE_TYPE structure properly defined using
 *  get_ASCII_header function
*/
static std::vector<char> BUF(1024,0);
void 
FindDetectorsPar::load_plain(std::ifstream &stream,std::vector<double> &Data,FileTypeDescriptor const &FILE_TYPE)
{

    char par_format[]=" %g %g %g %g %g";
    char phx_format[]=" %g %g %g %g %g %g";
    float data_buf[7];
    char *format;
    int BlockSize;
    char EOL = FILE_TYPE.line_end;

    switch(FILE_TYPE.Type){
        case(PAR_type):{
            format = par_format;
            BlockSize=5;
            break;
                        }
        case(PHX_type):{
            format = phx_format;
            BlockSize=6;
            break;
                        }
        default:		{
            g_log.error()<< " trying to load data in FindDetectorsPar::load_plain but the data type is not recognized\n";
            throw(std::invalid_argument(" trying to load data but the data type is not recognized"));
                        }
    }
    Data.resize(BlockSize*FILE_TYPE.nData_records);

    stream.seekg(FILE_TYPE.data_start_position,std::ios_base::beg);
    if(!stream.good()){		
        g_log.error()<<" can not rewind the file to the initial position where the data begin\n";
        throw(std::invalid_argument(" can not rewind the file to the initial position where the data begin"));
    }

    int nRead_Data;
    for(unsigned int i=0;i<FILE_TYPE.nData_records;i++){
        stream.getline(&BUF[0],BUF.size(),EOL);
        if(!stream.good()){	
            g_log.error()<<" error reading input file\n";
            throw(std::invalid_argument(" error reading input file\n"));
        }

        switch(FILE_TYPE.Type){
            case(PAR_type):{
                nRead_Data= sscanf(&BUF[0],format,data_buf,data_buf+1,data_buf+2,data_buf+3,data_buf+4);
                break;
                            }
            case(PHX_type):{
                nRead_Data= sscanf(&BUF[0],format,data_buf,data_buf+1,data_buf+2,data_buf+3,data_buf+4,data_buf+5);
                break;
                            }
        }
        if(nRead_Data!=BlockSize){
            g_log.error()<<" Error reading data at file, row "<<i+1<<" column "<<nRead_Data<<" from total "<<FILE_TYPE.nData_records<<" rows, "<<BlockSize<<" columns\n";
            throw(std::invalid_argument("error while interpreting data "));

        }
        for(int j=0;j<nRead_Data;j++){
            Data[i*BlockSize+j]=(double)data_buf[j];
        }

    }
}

}// end DataHandling namespace
}// end MantidNamespace
