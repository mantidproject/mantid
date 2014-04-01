/*WIKI* 

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadReflTBL.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidKernel/Strings.h"
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
    DECLARE_FILELOADER_ALGORITHM(LoadReflTBL);

    /// Sets documentation strings for this algorithm
    void LoadReflTBL::initDocs()
    {
      this->setWikiSummary("Loads data from a reflectometry table file and stores it in a table [[workspace]] ([[TableWorkspace]] class). ");
      this->setOptionalMessage("Loads data from a reflectometry table file and stores it in a table workspace (TableWorkspace class).");
    }

    using namespace Kernel;
    using namespace API;

    /// Empty constructor
    LoadReflTBL::LoadReflTBL()
    {
    }

    /**
    * Return the confidence with with this algorithm can load the file
    * @param descriptor A descriptor for the file
    * @returns An integer specifying the confidence level. 0 indicates it will not be used
    */
    int LoadReflTBL::confidence(Kernel::FileDescriptor & descriptor) const
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
        std::istream & stream = descriptor.data();
        std::string firstLine;
        Kernel::Strings::extractToEOL(stream, firstLine);
        std::vector<std::string> columns;
        try
        {
          getCells(firstLine, columns);
          if (columns.size() == 17) //right ammount of columns
          {
            if (filePath.compare(filenameLength - 4,4,".tbl") == 0 )
            {
              confidence = 40;
            }
            else
            {
              confidence = 20;
            }
          }
          else //incorrect amount of columns
          {
            confidence = 0;
          }
        }
        catch (std::length_error)
        {
          confidence = 0;
        }
      }
      return confidence;
    }
    int LoadReflTBL::getCells(std::string line, std::vector<std::string> & cols) const
    {
      //first check the number of commas in the line.
      const size_t expectedCommas = 16;
      size_t pos = 0;
      size_t found = 0;
      pos=line.find(',',pos);
      if (pos!=std::string::npos)
      {
        ++found;
      }
      while (pos!=std::string::npos)
      {
        pos=line.find(',',pos+1);
        if (pos!=std::string::npos)
        {
          ++found;
        }
      }
      if (found == expectedCommas)
      {
        //If there are 16 that simplifies things and i can get boost to do the hard work
        boost::split(cols, line, boost::is_any_of(","), boost::token_compress_on);
      }
      else if (found < expectedCommas)
      {
        //less than 16 means the line isn't properly formatted. So Throw
        std::string message = "A line must contain 16 cell-delimiting commas. Found " + boost::lexical_cast<std::string>(found) + ".";
        throw std::length_error(message);
      }
      else
      {
        //More than 16 will need further checks. If there are pairs of double quotes, it's OK.
        //if there aren't any qutoes throw
        //look for pairs of double quotes so we can discount any commas inside them
        size_t quoteOne = 0;
        size_t quoteTwo = 0;
        size_t foundPairs = 0;
        std::vector<std::vector<size_t>> quoteBounds;
        while (quoteOne!=std::string::npos && quoteTwo!=std::string::npos)
        {
          if (quoteTwo == 0)
          {
            quoteOne=line.find('"');
          }
          else
          {
            quoteOne=line.find('"', quoteTwo + 1);
          }
          if (quoteOne!=std::string::npos)
          {
            quoteTwo=line.find('"', quoteOne + 1);
            if (quoteTwo!=std::string::npos)
            {
              ++foundPairs;
              std::vector<size_t> quotepair;
              quotepair.push_back(quoteOne);
              quotepair.push_back(quoteTwo);
              quoteBounds.push_back(quotepair);
            }
          }
        }
        //now check to see how many pairs of quotes were found, if none, then there are too many commas
        //as there'd need to be some commas inside quotes to make them part of the value and discount them as delimiters
        //otherwise we definitely have too many delimiters
        if (quoteBounds.size() == 0)
        {
          std::string message = "A line must contain 16 cell-delimiting commas. Found " + boost::lexical_cast<std::string>(found) + ".";
          throw std::length_error(message);
        }
        //now go through and split it up manually. Throw if we find ourselves in a positon where we'd add a 18th value to the vector
        size_t pairID = 0;
        size_t valsFound = 0;
        size_t lastComma = 0;
        pos = 0;
        cols.clear();
        while (pos!=std::string::npos)
        {
          pos=line.find(',',pos+1);
          if (pos!=std::string::npos)
          {
            if (pairID < quoteBounds.size() && pos>quoteBounds.at(pairID).at(0))
            {
              if (pos>quoteBounds.at(pairID).at(1))
              {
                //use the quote indexes to get the substring
                cols.push_back(line.substr(quoteBounds.at(pairID).at(0) + 1,
                                            quoteBounds.at(pairID).at(1) - (quoteBounds.at(pairID).at(0) + 1)));
                ++pairID;
                ++valsFound;
              }
            }
            else
            {
              if (lastComma != 0)
              {
                cols.push_back(line.substr(lastComma + 1,pos - (lastComma + 1)));
              }
              else
              {
                cols.push_back(line.substr(0,pos));
              }
              ++valsFound;
            }
            lastComma = pos;
          }
        }
        if (lastComma + 1 < line.length())
        {
          cols.push_back("");
        }
        else
        {
          cols.push_back(line.substr(lastComma + 1));
        }
        if (cols.size() > 17)
        {
          std::string message = "A line must contain 16 cell-delimiting commas. Found " + boost::lexical_cast<std::string>(cols.size() - 1) + ".";
          throw std::length_error(message);
        }
      }
      return static_cast<int>(cols.size());
    }
    //--------------------------------------------------------------------------
    // Private methods
    //--------------------------------------------------------------------------
    /// Initialisation method.
    void LoadReflTBL::init()
    {
      std::vector<std::string> exts;
      exts.push_back(".tbl");

      declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "The name of the table file to read, including its full or relative path. The file extension must be .tbl");
      declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace",
        "",Direction::Output), "The name of the workspace that will be created, "
        "filled with the read-in data and stored in the [[Analysis Data Service]].");
    }

    /** 
    *   Executes the algorithm.
    */
    void LoadReflTBL::exec()
    {
      std::string filename = getProperty("Filename");
      std::ifstream file(filename.c_str());
      if (!file)
      {
        g_log.error("Unable to open file: " + filename);
        throw Exception::FileError("Unable to open file: " , filename);
      }
      std::string line = "";
      Kernel::Strings::extractToEOL(file, line);

      std::vector<std::string> columns;
      getCells(line, columns);
    }


  } // namespace DataHandling
} // namespace Mantid