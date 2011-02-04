//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadGSS.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/LoadAlgorithmFactory.h"

#include <boost/math/special_functions/fpclassify.hpp>
#include <Poco/File.h>
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadGSS)

//register the algorithm into loadalgorithm factory
DECLARE_LOADALGORITHM(LoadGSS)
//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
* Initialise the algorithm
*/
void LoadGSS::init()
{
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load), "The input filename of the stored data");
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace", "", Kernel::Direction::Output));
}

/**
* Execute the algorithm
*/
void LoadGSS::exec()
{
  using namespace Mantid::API;
  std::string filename = getProperty("Filename");

  std::vector<MantidVec*> gsasDataX;
  std::vector<MantidVec*> gsasDataY;
  std::vector<MantidVec*> gsasDataE;

  std::string wsTitle;

  std::ifstream input(filename.c_str(), std::ios_base::in);

  MantidVec* X = new MantidVec();
  MantidVec* Y = new MantidVec();
  MantidVec* E = new MantidVec();

  int nSpec = 0;

  Progress* prog = NULL;

  char currentLine[256];

  // Gather data
  if ( input.is_open() )
  {
    if ( ! input.eof() )
    {
      // Get workspace title (should be first line)
      input.getline(currentLine, 256);
      wsTitle = currentLine;
    }
    while ( ! input.eof() && input.getline(currentLine, 256) )
    {
      if ( nSpec != 0 && prog == NULL )
      {
        prog = new Progress(this, 0.0, 1.0, nSpec);
      }
      double bc1;
      double bc2;
      double bc4;
      if (  currentLine[0] == '\n' || currentLine[0] == '#' )
      {
        if ( nSpec == 0 )
        {
          int noSpectra = 0;
          std::string line;

          std::istringstream inputLine(currentLine, std::ios::in);
          inputLine.ignore(256, ' ');
          inputLine >> noSpectra >> line;

          if ( ( noSpectra != 0 ) && ( line == "Histograms" ) )
          {
            nSpec = noSpectra;
          }          
        }
        continue;
      }
      else if ( currentLine[0] == 'B' )
      {
        if ( X->size() != 0 )
        {
          gsasDataX.push_back(X);
          gsasDataY.push_back(Y);
          gsasDataE.push_back(E);

          if ( prog != NULL )
            prog->report();
        }

        /* BANK <SpectraNo> <NBins> <NBins> RALF <BC1> <BC2> <BC1> <BC4> 
        *  BC1 = X[0] * 32
        *  BC2 = X[1] * 32 - BC1
        *  BC4 = ( X[1] - X[0] ) / X[0]
        */
        X = new MantidVec();
        Y = new MantidVec();
        E = new MantidVec();

        std::istringstream inputLine(currentLine, std::ios::in);
        inputLine.ignore(256, 'F');
        inputLine >> bc1 >> bc2 >> bc1 >> bc4;

        double x0 = bc1 / 32;
        X->push_back(x0);
      }
      else
      {
        double xValue;
        double yValue;
        double eValue;

        double xPrev;

        if ( X->size() != 0 )
        {
          xPrev = X->back();
        }
        else
        {
          throw Mantid::Kernel::Exception::NotImplementedError("LoadGSS: File was not in expected format.");
        }

        std::istringstream inputLine(currentLine, std::ios::in);
        inputLine >> xValue >> yValue >> eValue;

        xValue = (2 * xValue) - xPrev;
        yValue = yValue / ( xPrev * bc4 );
        eValue = eValue / ( xPrev * bc4 );
        X->push_back(xValue);
        Y->push_back(yValue);
        E->push_back(eValue);
      }
    }
    if ( X->size() != 0 )
    { // Put final spectra into data
      gsasDataX.push_back(X);
      gsasDataY.push_back(Y);
      gsasDataE.push_back(E);
    }
    input.close();
  }

  int nHist = gsasDataX.size();
  int xWidth = X->size();
  int yWidth = Y->size();

  // Create workspace
  MatrixWorkspace_sptr outputWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace> (WorkspaceFactory::Instance().create("Workspace2D", nHist, xWidth, yWidth));
  // GSS Files data is always in TOF
  outputWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  // Set workspace title
  outputWorkspace->setTitle(wsTitle);
  // Put data from MatidVec's into outputWorkspace
  for ( int i = 0; i < nHist; ++i )
  {
    // Move data across
    outputWorkspace->dataX(i) = *gsasDataX[i];
    outputWorkspace->dataY(i) = *gsasDataY[i];
    outputWorkspace->dataE(i) = *gsasDataE[i];
  }

  setProperty("OutputWorkspace", outputWorkspace);
  return;
}


/**This method does a quick file type check by checking the first 100 bytes of the file 
 *  @param filePath- path of the file including name.
 *  @param nread :: no.of bytes read
 *  @param header :: The first 100 bytes of the file as a union
 *  @return true if the given file is of type which can be loaded by this algorithm
 */
    bool LoadGSS::quickFileCheck(const std::string& filePath,size_t nread,const file_header& header)
    {
      std::string extn=extension(filePath);
      bool bascii(false);
      (!extn.compare("txt"))?bascii=true:bascii=false;

      bool is_ascii (true);
      for(size_t i=0; i<nread; i++)
      {
        if (!isascii(header.full_hdr[i]))
          is_ascii =false;
      }
      return(is_ascii|| bascii?true:false);
    }

/**checks the file by opening it and reading few lines 
 *  @param filePath :: name of the file inluding its path
 *  @return an integer value how much this algorithm can load the file 
 */
    int LoadGSS::fileCheck(const std::string& filePath)
    {      
      std::ifstream file(filePath.c_str());
      if (!file)
      {
        g_log.error("Unable to open file: " + filePath);
        throw Exception::FileError("Unable to open file: " , filePath);
      }
      std::string str;
      getline(file,str);//workspace ttile first line
      while (!file.eof())
      {
        getline(file,str);
        if(str.empty()||str[0]=='#')
        {
          continue;
        }
        if(!str.substr(0,4).compare("BANK")&& (str.find("RALF")!=std::string::npos)&& (str.find("FXYE")!=std::string::npos))
        {
          return 80;
        }
        return 0;
      }
      return  0;

    }

