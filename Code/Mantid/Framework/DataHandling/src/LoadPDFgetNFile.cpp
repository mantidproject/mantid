#include "MantidDataHandling/LoadPDFgetNFile.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace std;
using namespace boost;

namespace Mantid
{
namespace DataHandling
{

  DECLARE_ALGORITHM(LoadPDFgetNFile)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadPDFgetNFile::LoadPDFgetNFile()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadPDFgetNFile::~LoadPDFgetNFile()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Init documentation
    */
  void LoadPDFgetNFile::initDocs()
  {
    this->setWikiSummary("Load various types of PDFgetN data file.");
    this->setOptionalMessage("Types of PDFgetN data files include .sqa, .sq, .gr, and etc.");
  }

  //----------------------------------------------------------------------------------------------
  /** Define input
    */
  void LoadPDFgetNFile::init()
  {
    std::vector<std::string> exts;
    exts.push_back(".sq");
    exts.push_back(".sqa");
    exts.push_back(".sqb");
    exts.push_back(".gr");
    exts.push_back(".ain");
    exts.push_back(".braw");
    exts.push_back(".bsmo");

    auto fileproperty = new FileProperty("InputFile", "", FileProperty::Load, exts, Kernel::Direction::Input);
    this->declareProperty(fileproperty, "Name of the PDFgetN file to load.");

    auto wsproperty = new WorkspaceProperty<Workspace2D>("OutputWorkspace", "Anonymous", Kernel::Direction::Output);
    this->declareProperty(wsproperty, "Name of output workspace. ");
  }

  //----------------------------------------------------------------------------------------------
  /** Main executor
    */
  void LoadPDFgetNFile::exec()
  {
    // 1. Parse input file
    std::string inpfilename = getProperty("InputFile");

    parseDataFile(inpfilename);

    // 2. Generate output workspace
    generateDataWorkspace();

    setProperty("OutputWorkspace", outWS);

    return;
  }
  
  //----------------------------------------------------------------------------------------------
  /** Parse PDFgetN data file to
    * 1. a 2D vector for column data
    * 2. a 1D string vector for column name
    */
  void LoadPDFgetNFile::parseDataFile(std::string filename)
  {
    // 1. Open file
    std::ifstream ifile;
    ifile.open((filename.c_str()));

    if (!ifile.is_open())
    {
      stringstream errmsg;
      errmsg << "Unable to open file " << filename << ".  Quit!";
      g_log.error() << errmsg.str() << std::endl;
      throw std::runtime_error(errmsg.str());
    }
    else
    {
      g_log.notice() << "Open PDFgetN File " << filename << std::endl;
    }

    // 2. Parse
    bool readdata = false;

    char line[256];
    while(ifile.getline(line, 256))
    {
      string sline(line);

      if (!readdata && startsWith(sline, "#L"))
      {
        // a) Find header line for the data segment in the file
        parseColumnNameLine(sline);
        readdata = true;

        // Set up the data structure
        size_t numcols = mColumnNames.size();
        for (size_t i = 0; i < numcols; ++i)
        {
          std::vector<double> tempvec;
          mData.push_back(tempvec);
        }

      }
      else if (readdata)
      {
        // b) Parse data
        parseDataLine(sline);
      }
      else
      {
        // c) Do nothing
        ;
      }
    } // ENDWHILE

    if (!readdata)
    {
      stringstream errmsg;
      errmsg << "Unable to find a line staring with #L as the indicator of data segment. ";
      g_log.error() << errmsg.str() << std::endl;
      throw std::runtime_error(errmsg.str());
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Check whether the line starts with some specific character
    */
  bool LoadPDFgetNFile::startsWith(std::string s, std::string header)
  {
    bool answer = true;

    if (s.size() < header.size())
    {
      answer = false;
    }
    else
    {
      size_t numchars = header.size();
      for (size_t i = 0; i < numchars; ++i)
      {
        char c0 = s[i];
        char c1 = header[i];
        if (c0 != c1)
        {
          answer = false;
          break;
        }
      }
    }

    return answer;
  }

  /** Parse column name line staring with #L
    */
  void LoadPDFgetNFile::parseColumnNameLine(std::string line)
  {
    // 1. Split
    vector<string> terms;
    boost::split(terms, line, is_any_of(" \t\n"), token_compress_on);

    // 2. Validate
    if (terms.size() == 0)
    {
      throw std::runtime_error("There is nothing in the input line!");
    }

    string header = terms[0];
    if (header.compare("#L") != 0)
    {
      stringstream errmsg;
      errmsg << "Expecting header as #L.  Input line has header as " << header
             << ". Unable to proceed. ";
      g_log.error() << errmsg.str() << endl;
      throw std::runtime_error(errmsg.str());
    }

    // 3. Parse
    size_t numcols = terms.size()-1;
    for (size_t i = 0; i < numcols; ++i)
    {
      this->mColumnNames.push_back(terms[i+1]);
      cout << "Column " << i << ": " << mColumnNames[i] << std::endl;
    }

    return;
  }

  /** Parse data line
    */
  void LoadPDFgetNFile::parseDataLine(string line)
  {
    // 1. Trim (stripg) and Split
    boost::trim(line);
    vector<string> terms;
    boost::split(terms, line, is_any_of(" \t\n"), token_compress_on);

    // 2. Validate
    size_t numcols = mData.size();
    if (terms.size() != numcols)
    {
      g_log.warning() << "Line (" << line << ") has incorrect number of columns other than " << numcols << " as expected. " << std::endl;
      /*
      for (size_t i = 0; i < terms.size(); ++i)
      {
        cout << "Term " << i << " = " << terms[i] << "   Size = " << terms[i].size() << std::endl;
      }
      */
      // throw std::runtime_error("Input data line has wrong number of columns.");
      return;
    }

    // 3. Parse
    for (size_t i = 0; i < numcols; ++i)
    {
      string temps = terms[i];
      double tempvalue;
      if (temps.compare("NaN") == 0)
      {
        // FIXME:  Need to discuss with Peter about how to treat NaN value
        tempvalue = DBL_MAX-1.0;
        tempvalue = 0.0;
      }
      else if (temps.compare("-NaN") == 0)
      {
        tempvalue = -DBL_MAX+1.0;
        // FIXME:  Need to discuss with Peter about how to treat NaN value
        tempvalue = 0.0;
      }
      else
      {
        tempvalue = atof(temps.c_str());
      }

      mData[i].push_back(tempvalue);
    }

    /*
    if (mData[0].size() < 10)
    {
      cout << mData.size() << ", " << mData[0].size() << ", " << mData[1].size() << std::endl;
      cout << mData[0].back() << ", " << mData[1].back() << endl;
    }
    */

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate output data workspace
    */
  void LoadPDFgetNFile::generateDataWorkspace()
  {
    // 0. Check
    if (mData.size() == 0)
    {
      throw runtime_error("Data set has not been initialized. Quit!");
    }

    // 1. Figure out number of data set
    size_t numsets = 0;
    vector<size_t> numptsvec;
    size_t arraysize = mData[0].size();
    if (arraysize <= 1)
    {
      throw runtime_error("Data array size is less and equal to 1.  Unphysically too small.");
    }

    double prex = mData[0][0];
    size_t vecsize = 1;
    for (size_t i = 1; i < arraysize; ++i)
    {
      double curx = mData[0][i];
      if (curx < prex)
      {
        numsets += 1;
        numptsvec.push_back(vecsize);
        vecsize = 1;
      }
      else
      {
        ++ vecsize;
      }
      prex = curx;
    }
    ++ numsets;
    numptsvec.push_back(vecsize);

    bool samesize = true;
    for (size_t i = 0; i < numsets; ++i)
    {
      if (i > 0)
      {
        if (numptsvec[i] != numptsvec[i-1])
        {
          samesize = false;
        }
      }
      std::cout << "Set " << i << ":  Number of Points = " << numptsvec[i] << std::endl;
    }
    if (!samesize)
    {
      stringstream errmsg;
      errmsg << "Multiple bank (number of banks = " << numsets << ") have different size of data array.  Unable to handle this situation.";
      g_log.error() << errmsg.str() << std::endl;
      throw std::runtime_error(errmsg.str());
    }
    size_t size = numptsvec[0];

    // 2. Start the
    outWS = boost::dynamic_pointer_cast<Workspace2D>(
          API::WorkspaceFactory::Instance().create("Workspace2D", numsets, size, size));

    // 3. Set number
    size_t numspec = outWS->getNumberHistograms();
    for (size_t i = 0; i < numspec; ++i)
    {
      MantidVec& X = outWS->dataX(i);
      MantidVec& Y = outWS->dataY(i);
      MantidVec& E = outWS->dataE(i);

      size_t baseindex = i*size;
      for (size_t j = 0; j < size; ++j)
      {
        X[j] = mData[0][baseindex+j];
        Y[j] = mData[1][baseindex+j];
        E[j] = mData[2][baseindex+j];
      }
    }

    return;
  }

} // namespace DataHandling
} // namespace Mantid






























