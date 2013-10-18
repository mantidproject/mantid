/*WIKI* 

The LoadAscii2 algorithm reads in spectra data from a text file and stores it in a [[Workspace2D]] as data points. The data in the file must be organized in columns separated by commas, tabs, spaces, colons or semicolons. Only one separator type can be used throughout the file; use the "Separator" property to tell the algorithm which to use. The algorithm [[SaveAscii2]] is normally able to produce such a file.

The format must be:
* A single integer or blank line to denote a new spectra
* For each bin, between two and four columns of delimted data in the following order: 1st column=X, 2nd column=Y, 3rd column=E, 4th column=DX (X error)
* Comments can be included by prefixing the line with a non-numerical character which must be consistant throughout the file and specified when you load the file. Defaults to "#"
* The number of bins is defined by the number of rows and must be identical for each spectra

The following is an example valid file of 4 spectra of 2 bins each with no X error
# X , Y , E
1
2.00000000,2.00000000,1.00000000
4.00000000,1.00000000,1.00000000
2
2.00000000,5.00000000,2.00000000
4.00000000,4.00000000,2.00000000
3
2.00000000,3.00000000,1.00000000
4.00000000,0.00000000,0.00000000
4
2.00000000,0.00000000,0.00000000
4.00000000,0.00000000,0.00000000

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadAscii2.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include <fstream>

#include <boost/tokenizer.hpp>
#include <Poco/StringTokenizer.h>
// String utilities
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

namespace Mantid
{
  namespace DataHandling
  {
    DECLARE_FILELOADER_ALGORITHM(LoadAscii2);

    /// Sets documentation strings for this algorithm
    void LoadAscii2::initDocs()
    {
      this->setWikiSummary("Loads data from a text file and stores it in a 2D [[workspace]] ([[Workspace2D]] class). ");
      this->setOptionalMessage("Loads data from a text file and stores it in a 2D workspace (Workspace2D class).");
    }

    using namespace Kernel;
    using namespace API;

    /// Empty constructor
    LoadAscii2::LoadAscii2() : m_columnSep(), m_separatorIndex()
    {
    }

    /**
    * Return the confidence with with this algorithm can load the file
    * @param descriptor A descriptor for the file
    * @returns An integer specifying the confidence level. 0 indicates it will not be used
    */
    int LoadAscii2::confidence(Kernel::FileDescriptor & descriptor) const
    {
      const std::string & filePath = descriptor.filename();
      const size_t filenameLength = filePath.size();

      // Avoid some known file types that have different loaders
      int confidence(0);
      if( filePath.compare(filenameLength - 12,12,"_runinfo.xml") == 0 ||
        filePath.compare(filenameLength - 6,6,".peaks") == 0 ||
        filePath.compare(filenameLength - 10,10,".integrate") == 0 )
      {
        confidence = 0;
      }
      else if(descriptor.isAscii())
      {
        confidence = 10; // Low so that others may try
      }
      return confidence;
    }

    /**
    * Check if a file is a text file
    * @param file :: The file pointer
    * @returns true if the file an ascii text file, false otherwise
    */
    bool LoadAscii2::isAscii(FILE *file)
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
    * Reads the data from the file. It is assumed that the provided file stream has its position
    * set such that the first call to getline will be give the first line of data
    * @param file :: A reference to a file stream
    * @returns A pointer to a new workspace
    */
    API::Workspace_sptr LoadAscii2::readData(std::ifstream & file)
    {
      //there should be no need for processheader now as this will now skip blanks and comment lines and throw on anything unusual
      //it's probably more stirct versus version 1, but then this is a format change and we don't want any bad data getting into the workspace
      //there is still flexibility, but the format should jsut make more sense in general

      m_baseCols = 0;
      m_specNo = 0;
      m_lastBins = 0;
      m_curBins = 0;
      m_spectraStart = true;
      m_specIDs = 0;

      int lineNo = 0;
      m_spectra.clear();
      m_curSpectra = new DataObjects::Histogram1D();
      std::string line;

      std::list<std::string> columns;

      setcolumns(file, line, columns);

      while( getline(file,line) )
      {
        lineNo++;
        if (line.empty())
        {
          //the line is empty, treat as a break before a new spectra
          //Signifies the start of a new spectra if it wasn't preceeded with another blank line or a spectra ID
          newSpectra();
        }
        else if (!skipLine(line))
        {
          parseLine(line, columns, lineNo);
        }
      }

      newSpectra();

      const size_t numSpectra = m_spectra.size();
      MatrixWorkspace_sptr localWorkspace = WorkspaceFactory::Instance().create("Workspace2D",numSpectra, m_lastBins, m_lastBins);

      writeToWorkspace(localWorkspace, numSpectra);
      delete m_curSpectra;
      return localWorkspace;
    }

    /**
    * Check the start of the file for the first data set, then set the number of columns that hsould be expected thereafter
    * @param[in] line : The current line of data
    * @param[in] columns : the columns of values in the current line of data
    * @param[in] lineNo : the current line number
    */
    void LoadAscii2::parseLine(const std::string & line, std::list<std::string> & columns, const int & lineNo)
    {
      if (std::isdigit(line.at(0)))
      {
        const int cols = splitIntoColumns(columns, line);
        if (cols > 4 || cols < 0)
        {
          //there were more separators than there should have been, which isn't right, or something went rather wrong
          throw std::runtime_error("Sets of values must have between 1 and 3 delimiters");
        }
        else if (cols == 1)
        {
          //a size of 1 is a spectra ID as long as there are no alphabetic characters in it. Signifies the start of a new spectra if it wasn't preceeded with a blank line
          newSpectra();

          //at this point both vectors should be the same size (or the ID counter should be 0, but as we're here then that's out the window),
          if (m_spectra.size() == m_specIDs)
          {
            m_specIDs++;
          }
          else
          {
            //if not then they've ommitted IDs in the the file previously and just decided to include one (which is wrong and confuses everything)
            throw std::runtime_error("Inconsistent inclusion of spectra IDs. All spectra must have IDs or all spectra must not have IDs. "
              "Check for blank lines, as they symbolize the end of one spectra and the start of another.");
          }
          //this will overwrite the old id in the case of consecutive spectra ID lines
          m_curSpectra->setSpectrumNo(boost::lexical_cast<int>(*(columns.begin())));
        }
        else if (cols != 1)
        {
          inconsistantIDCheck();

          checkLineColumns(cols);

          addToCurrentSpectra(columns);
        }
      }
      else if (badLine(line))
      {
        throw std::runtime_error("Unexpected character found at beggining of line " + std::to_string(lineNo) + ". Lines muct either be a single integer, a list of numeric values, blank, or a text line beggining with the specified comment indicator:" + m_comment +".");
      }
      else
      {
        //strictly speaking this should never be hit, but just being sure
        throw std::runtime_error("Unknown format at line " + std::to_string(lineNo) + ". Lines muct either be a single integer, a list of numeric values, blank, or a text line beggining with the specified comment indicator:" + m_comment +".");
      }
    }

    /**
    * Construct the workspace
    * @param[out] localWorkspace : the workspace beign constructed
    * @param[in] numSpectra : The number of spectra found in the file
    */
    void LoadAscii2::writeToWorkspace(API::MatrixWorkspace_sptr & localWorkspace, const size_t & numSpectra) const
    {
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
        localWorkspace->dataX(i) = m_spectra[i].readX();
        localWorkspace->dataY(i) = m_spectra[i].readY();
        //if E or DX are ommitted they're implicitly initalised as 0
        if (m_baseCols == 4 || m_baseCols == 3)
        {
          //E in file
          localWorkspace->dataE(i) = m_spectra[i].readE();
        }
        if (m_baseCols == 4)
        {
          //DX in file
          localWorkspace->dataDx(i) = m_spectra[i].readDx();
        }
        if (m_specIDs!= 0)
        {
          localWorkspace->getSpectrum(i)->setSpectrumNo(m_spectra[i].getSpectrumNo());
        }
        else
        {
          localWorkspace->getSpectrum(i)->setSpectrumNo(static_cast<specid_t>(i)+1);
        }
      }
    }

    /**
    * Check the start of the file for the first data set, then set the number of columns that hsould be expected thereafter
    * @param[in] file : The file stream
    * @param[in] line : The current line of data
    * @param[in] columns : the columns of values in the current line of data
    */
    void LoadAscii2::setcolumns(std::ifstream & file, std::string & line, std::list<std::string> & columns)
    {
      //first find the first data set and set that as the template for the number of data collumns we expect from this file
      while( getline(file,line) && m_baseCols == 0)
      {
        if (!line.empty())
        {
          if (std::isdigit(line.at(0)))
          {
            const int cols = splitIntoColumns(columns, line);
            //we might have the first set of values but there can't be more than 3 commas if it is
            //int values = std::count(line.begin(), line.end(), ',');
            if (cols > 4 || cols < 1)
            {
              //there were more separators than there should have been, which isn't right, or something went rather wrong
              throw std::runtime_error("Sets of values must have between 1 and 3 delimiters");
            }
            else if (cols != 1)
            {
              //a size of 1 is most likely a spectra ID so ignore it, a value of 2, 3 or 4 is a valid data set
              m_baseCols = cols;
            }
          }
        }
      }
      //make sure some valid data has been found to set the amount of columns, and the file isn't at EOF
      if (m_baseCols > 4 || m_baseCols < 2 || file.eof())
      {
        throw std::runtime_error("No valid data in file, check separator settings.");
      }

      //start from the top again, this time filling in the list
      file.seekg(0,std::ios_base::beg);
    }

    /**
    * Check if the file has been found to incosistantly include spectra IDs
    * @param[in] columns : the columns of values in the current line of data
    */
    void LoadAscii2::addToCurrentSpectra(std::list<std::string> & columns)
    {
      std::vector<double> values(m_baseCols, 0.);
      m_spectraStart = false;
      fillInputValues(values, columns);
      //add X and Y
      m_curSpectra->dataX().push_back(values[0]);
      m_curSpectra->dataY().push_back(values[1]);
      //check for E and DX
      switch (m_baseCols)
      {
        // if only 2 collumns X and Y in file, E = 0 is implicit when constructing workspace, omit DX
      case 3:
        {
          //E in file, include it, omit DX
          m_curSpectra->dataE().push_back(values[2]);
          break;
        }
      case 4:
        {
          //E and DX in file, include both
          m_curSpectra->dataE().push_back(values[2]);
          m_curSpectra->dataDx().push_back(values[3]);
          break;
        }
      }
      m_curBins++;
    }

    /**
    * Check if the file has been found to incosistantly include spectra IDs
    * @param[in] cols : the number of columns in the current line of data
    */
    void LoadAscii2::checkLineColumns(const int & cols) const
    {
      //a size of 2, 3 or 4 is a valid data set, but first see if it's the same as the first observed one
      if (m_baseCols != cols)
      {
        throw std::runtime_error("Number of data columns not consistent throughout file");
      }
    }

    /**
    * Check if the file has been found to incosistantly include spectra IDs
    * @param[in] spectraSize : the number of spectra recorded so far
    */
    void LoadAscii2::inconsistantIDCheck() const
    {
      //we need to do a check regarding spectra ids before doing anything else
      //is this the first bin in the spectra? if not this check has already been done for this spectra
      //If the ID vector is completly empty then it's ok we're assigning them later
      //if there are equal or less IDs in their vector than there are spectra in thiers, then there's been no ID assigned to this spectra and there should be
      if (m_spectraStart && m_specIDs != 0 && !(m_spectra.size() < m_specIDs))
      {
        throw std::runtime_error("Inconsistent inclusion of spectra IDs. All spectra must have IDs or all spectra must not have IDs. Check for blank lines, as they symbolize the end of one spectra and the start of another.");
      }
    }

    /**
    * Check if the file has been found to incosistantly include spectra IDs
    */
    void LoadAscii2::newSpectra()
    {
      if (!m_spectraStart)
      {
        if (m_lastBins == 0)
        {
          m_lastBins = m_curBins;
          m_curBins = 0;
        }
        else if (m_lastBins == m_curBins)
        {
          m_curBins = 0;
        }
        else
        {
          throw std::runtime_error("Number of bins per spectra not consistant.");
        }

        if (m_curSpectra)
        {
          size_t specSize = m_curSpectra->size();
          if (specSize > 0 && specSize == m_lastBins)
          {
            m_spectra.push_back(*m_curSpectra);
          }
          delete m_curSpectra;
        }

        m_curSpectra = new DataObjects::Histogram1D();
        m_spectraStart = true;
      }
    }

    /**
    * Return true if the line is to be skipped.
    * @param[in] line :: The line to be checked
    * @return True if the line should be skipped
    */
    bool LoadAscii2::skipLine(const std::string & line) const
    {
      // Comments are skipped, Empty actually means somehting and shouldn't be skipped
      //just checking the comment's first character should be ok as comment cahracters can't be numeric at all, so they can't really be confused
      return (line.at(0) == m_comment.at(0));
    }

    /**
    * Return true if the line doesn't start wiht a valid character.
    * @param[in] line :: The line to be checked
    * @return :: True if the line doesn't start wiht a valid character.
    */
    bool LoadAscii2::badLine(const std::string & line) const
    {
      // Empty or comment
      return (!std::isdigit(line.at(0)) && line.at(0) != m_comment.at(0));
    }


    /**
    * Split the data into columns based on the input separator
    * @param[out] columns :: A reference to a list to store the column data
    * @param[in] str :: The input string
    * @returns The number of columns
    */
    int LoadAscii2::splitIntoColumns(std::list<std::string> & columns, const std::string & str) const
    {
      boost::split(columns, str, boost::is_any_of(m_columnSep), boost::token_compress_on);
      return static_cast<int>(columns.size());
    }

    /**
    * Fill the given vector with the data values. Its size is assumed to be correct
    * @param[out] values :: The data vector fill
    * @param[in] columns :: The list of strings denoting columns
    */
    void LoadAscii2::fillInputValues(std::vector<double> &values, const std::list<std::string>& columns) const
    {
      values.resize(columns.size());
      std::list<std::string>::const_iterator iend = columns.end();
      int i = 0;
      for( std::list<std::string>::const_iterator itr = columns.begin(); itr != iend; ++itr )
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
    void LoadAscii2::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".dat");
      exts.push_back(".txt");
      exts.push_back(".csv");
      exts.push_back("");

      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The name of the text file to read, including its full or relative path. The file extension must be .tst, .dat, or .csv");
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace",
        "",Direction::Output), "The name of the workspace that will be created, filled with the read-in data and stored in the [[Analysis Data Service]].");

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
      declareProperty("Separator", "Automatic", boost::make_shared<StringListValidator>(sepOptions),
        "The separator between data columns in the data file. The possible values are \"CSV\", \"Tab\", "
        "\"Space\", \"SemiColon\", or \"Colon\" (default: Automatic selection).");
      declareProperty("CommentIndicator", "#", "Character(s) found front of comment lines. Cannot contain numeric characters");

      std::vector<std::string> units = UnitFactory::Instance().getKeys();
      units.insert(units.begin(),"Dimensionless");
      declareProperty("Unit","Energy", boost::make_shared<StringListValidator>(units),
        "The unit to assign to the X axis (anything known to the [[Unit Factory]] or \"Dimensionless\")");

      auto mustBePosInt = boost::make_shared<BoundedValidator<int> >();
      mustBePosInt->setLower(0);
    }

    /** 
    *   Executes the algorithm.
    */
    void LoadAscii2::exec()
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
      m_comment = getProperty("CommentIndicator");

      boost::regex test("[^0-9]+", boost::regex::perl);
      if (!boost::regex_match(m_comment.begin(), m_comment.end(), test))
      {
        throw std::runtime_error("Comment markers cannot contain numeric characters");
      }

      // Process the header information.
      //processHeader(file);
      // Read the data
      MatrixWorkspace_sptr outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(readData(file));
      outputWS->mutableRun().addProperty("Filename",filename);
      setProperty("OutputWorkspace", outputWS);
    }
  } // namespace DataHandling
} // namespace Mantid
