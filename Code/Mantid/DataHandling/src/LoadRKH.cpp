//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadRKH.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/date_parsing.hpp"

#include <fstream>

using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadRKH)
//register the algorithm into loadalgorithm factory
DECLARE_LOADALGORITHM(LoadRKH)
//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
 * Initialise the algorithm
 */
void LoadRKH::init()
{  
  std::vector<std::string> exts;
  exts.push_back(".txt");
  exts.push_back(".Q");
  declareProperty(new API::FileProperty("Filename","", API::FileProperty::Load, exts),
    "Name of the RKH file to load");
  declareProperty(
    new API::WorkspaceProperty<>("OutputWorkspace", "", Kernel::Direction::Output),
    "The name to use for the output workspace" );
  //Get the units registered with the UnitFactory
  std::vector<std::string> propOptions = Kernel::UnitFactory::Instance().getKeys();
  m_unitKeys.insert(propOptions.begin(), propOptions.end());

  //Add some others that will make this orient the other way
  m_RKHKeys.insert("SpectrumNumber");
  propOptions.insert(propOptions.end(), m_RKHKeys.begin(), m_RKHKeys.end());
  declareProperty("FirstColumnValue", "Wavelength",
    new Kernel::ListValidator(propOptions),
    "The units of the first column in the RKH file (default\n"
    "Wavelength)" );
}

/**
 * Execute the algorithm
 */
void LoadRKH::exec()
{
  using namespace Mantid::Kernel;
  using namespace Mantid::API;

  //Retrieve filename and try to open the file
  std::string filename = getPropertyValue("Filename");

  std::ifstream file(filename.c_str());
  if( !file )
  {
    g_log.error("Unable to open file " + filename);
    throw Exception::FileError("Unable to open File: ", filename);	  
  }

  //The first 2 lines of the file do not contain any useful information
  skipLines(file, 2);

  //The 3rd line contains information regarding the number of points in the file and
  // start and end reading points
  int totalPoints(0), readStart(0), readEnd(0), buried(0);
  std::string fileline("");
  getline(file, fileline);
  std::istringstream is(fileline);
  //Get data information
  for( int counter = 1; counter < 8; ++counter )
  {
    switch( counter )
    {
    case 1: 
      is >> totalPoints;
      break;
    case 5:
      is >> readStart;
      break;
    case 6:
      is >> readEnd;
      break;
    default:
      is >> buried;
      break;
    }
  }

  g_log.information() << "Total number of data points declared to be in the data file: " 
		      << totalPoints << "\n";

  //What are we reading?
  std::string firstColVal = getProperty("FirstColumnValue");
  bool colIsUnit(true);
  if( m_RKHKeys.find( firstColVal ) != m_RKHKeys.end() ) 
  {
    colIsUnit = false;
    readStart = 1;
    readEnd = totalPoints;
  }
  
  if( readStart < 1 || readEnd < 1 || readEnd < readStart ||
      readStart > totalPoints || readEnd > totalPoints )
  {
    g_log.error("Invalid data range specfied.");
    file.close();
    throw std::invalid_argument("Invalid data range specfied.");
  }

  g_log.information() << "Reading started on data line: "  << readStart << "\n";
  g_log.information() << "Reading finished on data line: " << readEnd << "\n";
  
  //The 4th and 5th line do not contain useful information either
  skipLines(file, 2);

  int pointsToRead = readEnd - readStart + 1;
  //Now stream sits at the first line of data 
  fileline = "";
  std::vector<double> columnOne, ydata, errdata;
  columnOne.reserve(readEnd);
  ydata.reserve(readEnd);
  errdata.reserve(readEnd);

  Progress prog(this,0.0,1.0,readEnd);
  for( int index = 1; index <= readEnd; ++index )
  {
    getline(file, fileline);
    if( index < readStart ) continue;
    double x(0.), y(0.), yerr(0.);
    std::istringstream datastr(fileline);
    datastr >> x >> y >> yerr;
    columnOne.push_back(x);
    ydata.push_back(y);
    errdata.push_back(yerr);
    prog.report();
  }
  file.close();

  assert( pointsToRead == static_cast<int>(columnOne.size()) );
  assert( pointsToRead == static_cast<int>(ydata.size()) );
  assert( pointsToRead == static_cast<int>(errdata.size()) );
    
  //The output workspace
  API::MatrixWorkspace_sptr localworkspace;
  if( colIsUnit )
  {
    localworkspace = 
      WorkspaceFactory::Instance().create("Workspace1D", 1, pointsToRead, pointsToRead);
    localworkspace->getAxis(0)->unit() = UnitFactory::Instance().create(firstColVal);
    localworkspace->dataX(0) = columnOne;
    localworkspace->dataY(0) = ydata;
    localworkspace->dataE(0) = errdata;
  }
  else
  {
    localworkspace = 
      WorkspaceFactory::Instance().create("Workspace2D", pointsToRead, 1, 1);
    //Set the appropriate values
    for( int index = 0; index < pointsToRead; ++index )
    {
      localworkspace->getAxis(1)->spectraNo(index) = static_cast<int>(columnOne[index]);
      localworkspace->dataY(index)[0] = ydata[index];
      localworkspace->dataE(index)[0] = errdata[index];
    }
  }

  //Set the output workspace
  setProperty("OutputWorkspace", localworkspace);
}

/**
 * Remove lines from an input stream
 * @param strm The input stream to adjust
 * @param nlines The number of lines to remove
 */
void LoadRKH::skipLines(std::istream & strm, int nlines)
{
  std::string buried("");
  for( int i = 0 ; i < nlines; ++i )
  {
    getline(strm, buried);
  }
}

/**This method does a quick file check by checking the no.of bytes read nread params and header buffer
 *  @param filePath- path of the file including name.
 *  @param nread - no.of bytes read
 *  @param header_buffer - buffer containing the 1st 100 bytes of the file
 *  @return true if the given file is of type which can be loaded by this algorithm
 */
bool LoadRKH::quickFileCheck(const std::string& filePath,size_t nread,unsigned char* header_buffer)
{
  std::string extn=extension(filePath);
  bool bascii(false);
  (!extn.compare("txt")|| extn.compare("q"))?bascii=true:bascii=false;

  bool is_ascii (true);
  for(int i=0; i<nread; i++)
  {
    if (!isascii(header_buffer[i]))
      is_ascii =false;
  }
  return(is_ascii|| bascii?true:false);
}

/**checks the file by opening it and reading few lines 
 *  @param filePath name of the file inluding its path
 *  @return an integer value how much this algorithm can load the file 
 */
int LoadRKH::fileCheck(const std::string& filePath)
{ 
  int bret=0;
  std::ifstream file(filePath.c_str());
  if (!file)
  {
    g_log.error("Unable to open file: " + filePath);
    throw Exception::FileError("Unable to open file: " , filePath);
  }

  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
   boost::char_separator<char> sep(" ");

  std::string fileline("");
  std::string ws,da,dt,time;
  int ncols=0;
  //get first line
  getline(file, fileline);
  std::istringstream is(fileline);
  //get diff words from first line
  is>>ws>>da>>dt>>time;
  //get the day,year and month part from first line
  std::string::size_type pos=dt.find_last_of("-");
  if(pos==std::string::npos)
  {
    return 0;
  }
  std::string year(dt.substr(pos+1,dt.length()-pos));
  
  std::string::size_type pos1=dt.find_last_of("-",pos-1);
  if(pos1==std::string::npos)
  {
    return 0;
  }
  std::string month(dt.substr(pos1+1,pos-pos1));
  
  std::string day(dt.substr(0,pos1));
  std::string rkhdate=year+"-";
  rkhdate+=month;
  rkhdate+="-";
  rkhdate+=day;
//if the first line contains date this could be rkh file
  try
  {
    boost::gregorian::date d(boost::gregorian::from_string(rkhdate));
    boost::gregorian::date::ymd_type ymd = d.year_month_day();
    if(ymd.month>=1 && ymd.month<13)
    {
      bret+=10;
    }
    if(ymd.day>=1 && ymd.month<32)
    {
      bret+=10;
    }
  }
  catch(std::exception&)
  {
    return 0;
  }
 
  
  //read second line
  getline(file, fileline);
  if(fileline.find("(")!=std::string::npos && fileline.find(")")!=std::string::npos)
  {
    bret+=10;
  }
  //read 3rd line
  getline(file, fileline);
  if(fileline.find("(")!=std::string::npos && fileline.find(")")!=std::string::npos)
  {
    bret+=10;
    if(fileline.find("    0    0    0    1")!=std::string::npos)
    {
      bret+=10;
    }
  }
  else
  { 
    tokenizer tok(fileline,sep);
    for (tokenizer::iterator beg=tok.begin(); beg!=tok.end(); ++beg)
    {		 
      ++ncols;
    }
    if(ncols==7)
    {
      bret+=10;
    }
  }
  //4th line
  getline(file, fileline);
  if(fileline.find("         0         0         0         0")!=std::string::npos)
  {
    bret+=10;
  }
  else if(fileline.find("  0 ")!=std::string::npos)
  {
      bret+=10;
  }
  //5th line
  getline(file, fileline);
  if(fileline.find("3 (F12.5,2E16.6)")!=std::string::npos)
  {
    bret+=10;
  }
  else if (fileline.find("  1\n")!=std::string::npos)
  { 
      bret+=10;
  }
  ncols=0;  
  //6th line data line
  getline(file, fileline);
  tokenizer tok1(fileline, sep); 
  for (tokenizer::iterator beg=tok1.begin(); beg!=tok1.end(); ++beg)
  {		 
    ++ncols;
  }
  if(ncols==3)
  {
    bret+=20;
  }
  
  return bret;
}
