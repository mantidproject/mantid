//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadRKH.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace1D.h"

#include <fstream>
#include <iomanip>

using namespace Mantid::DataHandling;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadRKH)

// Get a reference to the logger. It is used to print out information, warning and error messages
Mantid::Kernel::Logger& LoadRKH::g_log = Mantid::Kernel::Logger::get("LoadRKH");

//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
 * Initialise the algorithm
 */
void LoadRKH::init()
{
  declareProperty("Filename","", new Kernel::FileValidator());
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace", "", Kernel::Direction::Output));
  
  Kernel::BoundedValidator<int> *mustBePositive = new Kernel::BoundedValidator<int>();
  mustBePositive->setLower(1);
  declareProperty("DataStart",1, mustBePositive);
  declareProperty("DataEnd",1, mustBePositive->clone());
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
  int fileStart(0), fileEnd(0), buried(0);
  std::string fileline("");
  getline(file, fileline);
  std::istringstream is(fileline);
  //Get data information
  for( int counter = 1; counter < 8; ++counter )
  {
    switch( counter )
    {
    case 1: 
      is >> m_intTotalPoints;
      break;
    case 5:
      is >> fileStart;
      break;
    case 6:
      is >> fileEnd;
      break;
    default:
      is >> buried;
      break;
    }
  }

  g_log.information() << "Total number of data points in the data file: " 
		      << m_intTotalPoints << "\n";
  m_intReadStart = fileStart;
  m_intReadEnd = fileEnd;

  //Check property start and end points
  checkOptionalProperties();

  if( m_intReadStart != fileStart )
    g_log.warning() << "Overriding file default starting point, started reading at: " 
		    << m_intReadStart << "\n";
  if( m_intReadEnd != fileEnd )
    g_log.warning() << "Overriding file default end point, finished reading at: " 
		    << m_intReadEnd << "\n";
  
  //The 4th and 5th line do not contain useful information either
  skipLines(file, 2);
  int pointsToRead = m_intReadEnd - m_intReadStart + 1;
  //Now stream sits at the first line of data 
  fileline = "";
  std::vector<double> xdata, ydata, errdata;
  for( int index = 1; index <= m_intReadEnd; ++index )
  {
    getline(file, fileline);
    if( index < m_intReadStart ) continue;
    double x(0.), y(0.), yerr(0.);
    std::istringstream datastr(fileline);
    datastr >> x >> y >> yerr;
    xdata.push_back(x);
    ydata.push_back(y);
    errdata.push_back(yerr);
  }

  //The data is bin centred and so needs to be adjusted to a histogram format
  std::vector<double> xnew(pointsToRead + 1, 0.0);
  for( int i = 0; i < pointsToRead; ++i )
  {
    if( i == 0 )
    {
      double delta = xdata[i+1] - xdata[i];
      xnew[i] = xdata[i] - delta/2.0;
      xnew[i + 1] = xdata[i] + delta/2.0;
    }
    else
    {
      double delta = xdata[i] - xnew[i];
      xnew[i + 1] = xdata[i] + delta;
    }
  }

  API::MatrixWorkspace_sptr localworkspace = WorkspaceFactory::Instance().create("Workspace2D", 1, pointsToRead + 1, pointsToRead);
  localworkspace->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
  localworkspace->dataX(0) = xnew;
  localworkspace->dataY(0) = ydata;
  localworkspace->dataE(0) = errdata;

  setProperty("OutputWorkspace", localworkspace);
  
  file.close();
}

void LoadRKH::checkOptionalProperties()
{
  Kernel::Property* prop = getProperty("DataStart");
  if( !prop->isDefault() ) 
    m_intReadStart = getProperty("DataStart");
  
  prop = getProperty("DataEnd");
  if( !prop->isDefault() ) 
    m_intReadEnd = getProperty("DataEnd");

  if( m_intReadStart < 1 || m_intReadEnd < 1 || m_intReadEnd < m_intReadStart ||
      m_intReadStart > m_intTotalPoints || m_intReadEnd > m_intTotalPoints )
  {
    g_log.error("Invalid data range specfied.");
    throw std::invalid_argument("Invalid data range specfied.");
  }
 
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
