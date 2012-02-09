/*WIKI* 

The LoadAscii algorithm reads in spectra data from a text file and stores it in a [[Workspace2D]] as data points. The data in the file must be organized in columns separated by commas, tabs, spaces, colons or semicolons. Only one separator type can be used throughout the file; use the "Separator" property to tell the algorithm which to use. 

By default the algorithm attempts to guess which lines are header lines by trying to see where a contiguous block of numbers starts. This can be turned off by specifying the "SkipNumLines" property, which will then tell the algorithm to simply use that as the the number of header lines.

The format can be one of:
* Two columns: 1st column=X, 2nd column=Y, E=0
* For a workspace of ''n'' spectra, 2''n''+1 columns: 1''st'' column=X, 2i''th'' column=Y, 2i+1''th'' column =E

The number of bins is defined by the number of rows.

The resulting workspace will have common X binning for all spectra.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadAscii.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/LoadAlgorithmFactory.h"
#include <fstream>

#include <boost/tokenizer.hpp>
#include <Poco/StringTokenizer.h>
// String utilities
#include <boost/algorithm/string.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(LoadAscii)
    //register the algorithm into loadalgorithm factory
    DECLARE_LOADALGORITHM(LoadAscii)
    
    /// Sets documentation strings for this algorithm
    void LoadAscii::initDocs()
    {
      this->setWikiSummary("Loads data from a text file and stores it in a 2D [[workspace]] ([[Workspace2D]] class). ");
      this->setOptionalMessage("Loads data from a text file and stores it in a 2D workspace (Workspace2D class).");
    }
    

    using namespace Kernel;
    using namespace API;

    /// Empty constructor
    LoadAscii::LoadAscii() : m_columnSep(), m_separatorIndex()
    {
    }

    /** This method does a quick file check by checking the no.of bytes read nread params and header buffer
    *  @param filePath :: path of the file including name.
    *  @param nread :: no.of bytes read
    *  @param header :: The first 100 bytes of the file as a union
    *  @return true if the given file is of type which can be loaded by this algorithm
    */
    bool LoadAscii::quickFileCheck(const std::string& filePath,size_t nread,const file_header& header)
    {
      std::string extn=extension(filePath);
      if (!extn.compare("dat")||!extn.compare("csv")|| !extn.compare("txt")|| !extn.compare("")) //If the file is of type ascii then have a go
      {
        return true;
      }
      else //See if file looks like Ascii and have a go at doing something about it
      {
        bool is_ascii (true);
        for(size_t i=0; i<nread; i++)
        {
          if (!isascii(header.full_hdr[i]))
            is_ascii =false;
        }
        return (is_ascii);
      }
    }

    /**
    * Checks the file by opening it and reading few lines 
    * @param filePath name of the file including its path
    * @return an integer value how much this algorithm can load the file 
    */
    int LoadAscii::fileCheck(const std::string& filePath)
    {      
      FILE* file = fopen(filePath.c_str(), "rb");
      if (!file)
      {
        g_log.error("Unable to open file: " + filePath);
        throw Exception::FileError("Unable to open file: " , filePath);
      }
      
      int confidence(0);

      if (isAscii(file))
      {
        confidence = 10; //Lower because should load other files first
      }
      return confidence;
    }

    /**
    * Check if a file is a text file
    * @param file :: The file pointer
    * @returns true if the file an ascii text file, false otherwise
    */
    bool LoadAscii::isAscii(FILE *file)
    {
          char data[256];
      char *pend = &data[fread(data, 1, sizeof(data), file)];
      fseek(file,0,SEEK_SET);
      /*
      * Call it a binary file if we find a non-ascii character in the 
      * first 256 bytes of the file.
      */
      for( char *p = data;  p < pend; ++p )
      {
        unsigned long ch = (unsigned long)*p;
        if( !(ch <= 0x7F) )
        {
          return false;
        }

      }
      return true;
    }

    //--------------------------------------------------------------------------
    // Protected methods
    //--------------------------------------------------------------------------
    /**
    * Process the header information. This implementation just skips it entirely. 
    * @param file :: A reference to the file stream
    */
    void LoadAscii::processHeader(std::ifstream & file) const
    {

      // Most files will have some sort of header. If we've haven't been told how many lines to 
      // skip then try and guess 
      int numToSkip = getProperty("SkipNumLines");
      if( numToSkip == EMPTY_INT() )
      {
        const int rowsToMatch(5);
        // Have a guess where the data starts. Basically say, when we have say "rowsToMatch" lines of pure numbers
        // in a row then the line that started block is the top of the data
        int numCols(-1), matchingRows(0), row(0);
        std::string line;
        std::vector<double> values;
        while( getline(file,line) )
        {
          ++row;
          //int nchars = (int)line.length(); TODO dead code?
          boost::trim(line);
          if( this->skipLine(line) )
          {
            continue;
          }

          std::list<std::string> columns;
          int lineCols = this->splitIntoColumns(columns, line);
          try
          {
            fillInputValues(values, columns);
          }
          catch(boost::bad_lexical_cast&)
          {
            continue;
          }
          if( numCols < 0 ) numCols = lineCols;
          if( lineCols == numCols )
          {
            ++matchingRows;
            if( matchingRows == rowsToMatch ) break;
          }
          else
          {
            numCols = lineCols;
            matchingRows = 1;
          }
        }
        // Seek the file pointer back to the start.
        // NOTE: Originally had this as finding the stream position of the data and then moving the file pointer
        // back to the start of the data. This worked when a file was read on the same platform it was written
        // but failed when read on a different one due to underlying differences in the stream translation.
        file.seekg(0,std::ios::beg);
        // We've read the header plus the number of rowsToMatch
        numToSkip = row - rowsToMatch;
      }
      int i(0);
      std::string line;
      while( i < numToSkip && getline(file, line) )
      {
        ++i;
      }
      g_log.information() << "Skipped " << numToSkip << " line(s) of header information()\n";
    }

    /**
    * Reads the data from the file. It is assumed that the provided file stream has its position
    * set such that the first call to getline will be give the first line of data
    * @param file :: A reference to a file stream
    * @returns A pointer to a new workspace
    */
    API::Workspace_sptr LoadAscii::readData(std::ifstream & file) const
    {
      // Get the first line and find the number of spectra from the number of columns
      std::string line;
      getline(file,line);
      boost::trim(line);

      std::list<std::string> columns;
      const int numCols = splitIntoColumns(columns, line);
      if( numCols < 2 ) 
      {
        g_log.error() << "Invalid data format found in file \"" << getPropertyValue("Filename") << "\"\n";
        throw std::runtime_error("Invalid data format. Fewer than 2 columns found.");
      }
      size_t numSpectra(0);
      bool haveErrors(false);
      bool haveXErrors(false);
      // Assume single data set with no errors
      if( numCols == 2 )
      {
        numSpectra = numCols/2;
      }
      // Data with errors
      else if( (numCols-1) % 2 == 0 )
      {
        numSpectra = (numCols - 1)/2;
        haveErrors = true;
      }
      // Data with errors on both X and Y (4-column file)
      else if( numCols == 4 )
      {
        numSpectra = 1;
        haveErrors = true;
        haveXErrors = true;
      }
      else
      {
        g_log.error() << "Invalid data format found in file \"" << getPropertyValue("Filename") << "\"\n";
        g_log.error() << "LoadAscii requires the number of columns to be an even multiple of either 2 or 3.";
        throw std::runtime_error("Invalid data format.");
      }

      // A quick check at the number of lines won't be accurate enough as potentially there
      // could be blank lines and comment lines
      int numBins(0), lineNo(0);
      std::vector<DataObjects::Histogram1D> spectra(numSpectra);
      std::vector<double> values(numCols, 0.);
      do
      {
        ++lineNo;
        boost::trim(line);
        if( this->skipLine(line) ) continue;
        columns.clear();
        int lineCols = this->splitIntoColumns(columns, line); 
        if( lineCols != numCols )
        {
          std::ostringstream ostr;
          ostr << "Number of columns changed at line " << lineNo;
          throw std::runtime_error(ostr.str());
        }

        try
        {
          fillInputValues(values, columns); //ignores nans and replaces them with 0
        }
        catch(boost::bad_lexical_cast&)
        {
          g_log.error() << "Invalid value on line " << lineNo << " of \""
            << getPropertyValue("Filename") << "\"\n";
          throw std::runtime_error("Invalid value encountered.");
        }

        for (size_t i = 0; i < numSpectra; ++i)
        {
          spectra[i].dataX().push_back(values[0]);
          spectra[i].dataY().push_back(values[i*2+1]);
          if( haveErrors )
          {
            spectra[i].dataE().push_back(values[i*2+2]);
          }
          else
          {
            spectra[i].dataE().push_back(0.0);
          }
          if( haveXErrors )
          {
            // Note: we only have X errors with 4-column files.
            // We are only here when i=0.
            spectra[i].dataDx().push_back(values[3]);
          }
          else
          {
            spectra[i].dataDx().push_back(0.0);
          }
        }
        ++numBins;
      }
      while(getline(file,line));

      MatrixWorkspace_sptr localWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace>
        (WorkspaceFactory::Instance().create("Workspace2D",numSpectra,numBins,numBins));
      try 
      {
        localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create(getProperty("Unit"));
      } 
      catch (Exception::NotFoundError&) 
      {
        // Asked for dimensionless workspace (obviously not in unit factory)
      }

      for (size_t i = 0; i < numSpectra; ++i)
      {
        localWorkspace->dataX(i) = spectra[i].dataX();
        localWorkspace->dataY(i) = spectra[i].dataY();
        localWorkspace->dataE(i) = spectra[i].dataE();
        // Just have spectrum number start at 1 and count up
        localWorkspace->getAxis(1)->spectraNo(i) = static_cast<specid_t>(i+1);
      }
      return localWorkspace;
    }

    /**
    * Peek at a line without extracting it from the stream
    */
    void LoadAscii::peekLine(std::ifstream & is, std::string & str) const
    {
      getline(is, str);
      is.seekg(-(int)str.length(),std::ios::cur);
      boost::trim(str);
    }

    /**
    * Return true if the line is to be skipped.
    * @param line :: The line to be checked
    * @return True if the line should be skipped
    */
    bool LoadAscii::skipLine(const std::string & line) const
    {
      // Empty or comment
      return ( line.empty() || boost::starts_with(line, "#") );
    }

    /**
    * Split the data into columns based on the input separator
    * @param[out] columns :: A reference to a list to store the column data
    * @param[in] str :: The input string
    * @returns The number of columns
    */
    int LoadAscii::splitIntoColumns(std::list<std::string> & columns, const std::string & str) const
    {
      boost::split(columns, str, boost::is_any_of(m_columnSep), boost::token_compress_on);
      return static_cast<int>(columns.size());
    }

    /**
    * Fill the given vector with the data values. Its size is assumed to be correct
    * @param[out] values :: The data vector fill
    * @param columns :: The list of strings denoting columns
    */
    void LoadAscii::fillInputValues(std::vector<double> &values, 
      const std::list<std::string>& columns) const
    {
      values.resize(columns.size());
      std::list<std::string>::const_iterator iend = columns.end();
      int i = 0;
      for( std::list<std::string>::const_iterator itr = columns.begin();
        itr != iend; ++itr )
      {
        std::string value = *itr;
        boost::trim(value);
        boost::to_lower(value);
        if (value == "nan"|| value == "1.#qnan") //ignores nans (not a number) and replaces them with a nan
        { 
          double nan = std::numeric_limits<double>::quiet_NaN();//(0.0/0.0);
          values[i] = nan;
        }
        else
        {
          values[i] = boost::lexical_cast<double>(value);
        }
        ++i;
      }
    }

    //--------------------------------------------------------------------------
    // Private methods
    //--------------------------------------------------------------------------
    /// Initialisation method.
    void LoadAscii::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".dat");
      exts.push_back(".txt");
      exts.push_back(".csv");
      exts.push_back("");

      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "A comma separated Ascii file");
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace",
        "",Direction::Output), "The name of the workspace that will be created.");

      std::string spacers[6][6] = { {"Automatic", ",\t:; "}, {"CSV", ","},
          {"Tab", "\t"}, {"Space", " "}, {"Colon", ":"}, {"SemiColon", ";"} };
      // For the ListValidator
      std::vector<std::string> sepOptions;
      for( size_t i = 0; i < 5; ++i )
      {
        std::string option = spacers[i][0];
        m_separatorIndex.insert(std::pair<std::string,std::string>(option, spacers[i][1]));
        sepOptions.push_back(option);
      }
      declareProperty("Separator", "Automatic", new ListValidator(sepOptions),
        "The column separator character (default: Automatic selection)");

      std::vector<std::string> units = UnitFactory::Instance().getKeys();
      units.insert(units.begin(),"Dimensionless");
      declareProperty("Unit","Energy",new Kernel::ListValidator(units),
        "The unit to assign to the X axis (default: Energy)");
      BoundedValidator<int> * mustBePosInt = new BoundedValidator<int>();
      mustBePosInt->setLower(0);
      declareProperty("SkipNumLines", EMPTY_INT(), mustBePosInt,
        "If set, this number of lines from the top of the file are ignored.");
    }

    /** 
    *   Executes the algorithm.
    */
    void LoadAscii::exec()
    {
      std::string filename = getProperty("Filename");
      std::ifstream file(filename.c_str());
      if (!file)
      {
        g_log.error("Unable to open file: " + filename);
        throw Exception::FileError("Unable to open file: " , filename);
      }

      std::string sepOption = getProperty("Separator");
      m_columnSep = m_separatorIndex[sepOption];
      // Process the header information.
      processHeader(file);
      // Read the data
      Workspace_sptr outputWS = readData(file);
      setProperty("OutputWorkspace", outputWS);
    }


  } // namespace DataHandling
} // namespace Mantid
