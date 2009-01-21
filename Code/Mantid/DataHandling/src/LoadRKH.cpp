//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadRKH.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace1D.h"

#include <fstream>

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
  std::vector<double> xdata, ydata, errdata;
  for( int index = 1; index <= readEnd; ++index )
  {
    getline(file, fileline);
    if( index < readStart ) continue;
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

  API::MatrixWorkspace_sptr localworkspace = 
    WorkspaceFactory::Instance().create("Workspace2D", 1, pointsToRead + 1, pointsToRead);
  localworkspace->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
  localworkspace->dataX(0) = xnew;
  localworkspace->dataY(0) = ydata;
  localworkspace->dataE(0) = errdata;

  setProperty("OutputWorkspace", localworkspace);
  
  file.close();
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
