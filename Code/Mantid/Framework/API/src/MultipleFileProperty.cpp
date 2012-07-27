
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/MultipleFileProperty.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MultiFileValidator.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/System.h"

#include <Poco/Path.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <ctype.h>
#include <functional>
#include <numeric>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace API
{
  // Forward declarations
  namespace
  {
    /**
     * A functor that stores a list of extensions and then accumulates the full, resolved file
     * names that are passed to it on to an output string.  Used with the accumulate STL algorithm.
     */
    class AppendFullFileName
    {
    public:
      AppendFullFileName(const std::vector<std::string> & exts);

      std::string & operator()(std::string & result, const std::vector<std::string> & fileNames);
      std::string & operator()(std::string & result, const std::string & fileName);

    private:
      const std::vector<std::string> & m_exts;
    };

    std::vector<std::vector<std::string> > unflattenFileNames(
      const std::vector<std::string> & flattenedFileNames);

    std::string toSingleString(const std::vector<std::vector<std::string>> & filenames);
  }
  
  /**
   * Constructor
   *
   * @param name ::          The name of the property
   * @param exts ::          The allowed/suggested extensions
   * @param optional ::      If true, the property is optional
   */
  MultipleFileProperty::MultipleFileProperty(
    const std::string & name,
    const std::vector<std::string> & exts
  ) : PropertyWithValue<std::vector<std::vector<std::string> > >( name,
        std::vector<std::vector<std::string> >(), boost::make_shared<MultiFileValidator>(exts), Direction::Input),
      m_multiFileLoadingEnabled(),
      m_exts(exts),
      m_parser(),
      m_defaultExt(""),
      g_log(Kernel::Logger::get("MultipleFileProperty"))
  {
    std::string allowMultiFileLoading = Kernel::ConfigService::Instance().getString("loading.multifile");

    if( boost::iequals(allowMultiFileLoading, "On") )
      m_multiFileLoadingEnabled = true;
    else
      m_multiFileLoadingEnabled = false;
  }

  /**
   * Destructor
   */
  MultipleFileProperty::~MultipleFileProperty() {}

  /**
   * Convert the given propValue into a comma and plus separated list of full filenames, and pass to the parent's
   * setValue method to store as a vector of vector of strings.
   *
   * READ HEADER FILE DOCUMENTATION FOR A MORE DETAILED OVERVIEW.
   *
   * @param propValue :: A string of the allowed format, indicating the user's choice of files.
   * @return A string indicating the outcome of the attempt to set the property. An empty string indicates success.
   */
  std::string MultipleFileProperty::setValue(const std::string & propValue)
  {
    // No empty value is allowed.
    if( propValue.empty())
      return "No file(s) specified.";

    if( ! m_multiFileLoadingEnabled )
    {
      g_log.debug("MultiFile loading is not enabled, acting as standard FileProperty.");
      
      // Use a slave FileProperty to do the job for us.
      FileProperty slaveFileProp( "Slave", "", FileProperty::Load, m_exts, Direction::Input);

      std::string error = slaveFileProp.setValue(propValue);

      if(!error.empty())
        return error;

      // Store.
      try
      {
        std::vector<std::vector<std::string>> result;
        toValue(slaveFileProp(), result, "", "");
        PropertyWithValue<std::vector<std::vector<std::string> > >::operator=(result);
        return "";
      }
      catch ( std::invalid_argument& except)
      {
        g_log.debug() << "Could not set property " << name() << ": " << except.what();
        return except.what();
      }
      return "";
    }

    const std::string INVALID = "\\+\\+|,,|\\+,|,\\+";
    boost::smatch invalid_substring;
    if( boost::regex_search(
          propValue.begin(), propValue.end(), 
          invalid_substring,
          boost::regex(INVALID)) )
      return "Unable to parse filename due to an empty token.";

    // Else if multifile loading *is* enabled, then users make the concession that they cannot use "," or "+" in
    // directory names; they are used as operators only.
    const std::string NUM_COMMA_ALPHA   = "(?<=\\d)\\s*,\\s*(?=\\D)";
    const std::string ALPHA_COMMA_ALPHA = "(?<=\\D)\\s*,\\s*(?=\\D)";
    const std::string NUM_PLUS_ALPHA    = "(?<=\\d)\\s*\\+\\s*(?=\\D)";
    const std::string ALPHA_PLUS_ALPHA  = "(?<=\\D)\\s*\\+\\s*(?=\\D)";
    const std::string COMMA_OPERATORS   = NUM_COMMA_ALPHA + "|" + ALPHA_COMMA_ALPHA;
    const std::string PLUS_OPERATORS    = NUM_PLUS_ALPHA  + "|" + ALPHA_PLUS_ALPHA;
    
    std::stringstream errorMsg;

    // Tokenise on allowed comma operators, and iterate over each token.
    boost::sregex_token_iterator end;
    boost::sregex_token_iterator commaToken(
      propValue.begin(), propValue.end(), 
      boost::regex(COMMA_OPERATORS), -1);
    
    std::vector<std::vector<std::string> > fileNames;

    try
    {
      for(; commaToken != end; ++commaToken)
      {
        const std::string comma = commaToken->str();
        
        // Tokenise on allowed plus operators, and iterate over each token.
        boost::sregex_token_iterator plusToken(
          comma.begin(), comma.end(), 
          boost::regex(PLUS_OPERATORS, boost::regex_constants::perl), -1);

        std::vector<std::vector<std::vector<std::string>>> temp;

        for(; plusToken != end; ++plusToken)
        {
          const std::string plus = plusToken->str();

          try
          {
            m_parser.parse(plus);
          }
          catch(const std::runtime_error & re)
          {
            errorMsg << "Unable to parse runs: \"" << re.what() << "\". ";
          }

          std::vector<std::vector<std::string>> f = m_parser.fileNames();

          // If there are no files, then we should use this token as it was passed to the property,
          // in its untampered form. This will enable us to deal with the case where a user is trying to 
          // load a single (and possibly existing) file within a token, but which has unexpected zero 
          // padding, or some other anomaly.
          if( flattenFileNames(f).size() == 0 )
            f.push_back(std::vector<std::string>(1, plus));
          
          temp.push_back(f);
        }

        // See [3] in header documentation.  Basically, for reasons of ambiguity, we cant add 
        // together plusTokens if they contain more than one file.  Throw on any instances of this.
        if( temp.size() > 1 )
        {
          for(auto tempFiles = temp.begin(); tempFiles != temp.end(); ++tempFiles)
            if( flattenFileNames(*tempFiles).size() > 1 )
              throw std::runtime_error("Adding a range of files to another file(s) is not currently supported.");
        }

        for( auto multifile = temp.begin(); multifile != temp.end(); ++multifile )
          fileNames.insert(
            fileNames.end(),
            multifile->begin(), multifile->end());
      }
    }
    catch(const std::runtime_error & re)
    {
      errorMsg << "Unable to parse runs: \"" << re.what() << "\". ";
      return errorMsg.str();
    }

    if(fileNames.size() == 1 && fileNames[0].size() == 1)
      fileNames[0][0] = propValue;

    
    std::string fullFileNames = "";
    try
    {
      // Use an AppendFullFileName functor object with std::accumulate to append
      // full filenames to a single string.
      AppendFullFileName appendFullFileName(m_exts);
      fullFileNames = std::accumulate(
        fileNames.begin(), fileNames.end(),
        std::string(""),
        appendFullFileName);
    }
    catch(const std::runtime_error & re)
    {
      return re.what();
    }

    // Now re-set the value using the full paths found.
    return PropertyWithValue<std::vector<std::vector<std::string> > >::setValue(fullFileNames);
  }
  
  std::string MultipleFileProperty::value() const
  {
    if( ! m_multiFileLoadingEnabled )
      return toString(m_value, "", "");

    return toString(m_value);
  }

  /**
   * Get the value the property was initialised with -its default value
   * @return The default value
   */
  std::string MultipleFileProperty::getDefault() const
  {
    if( ! m_multiFileLoadingEnabled )
      return toString(m_initialValue, "", "");

    return toString(m_initialValue);
  }

  /**
   * A convenience function for the cases where we dont use the MultiFileProperty to
   * *add* workspaces - only to list them.  It "flattens" the given vector of vectors
   * into a single vector which is much easier to traverse.  For example:
   *
   * ((1), (2), (30), (31), (32), (100), (102)) becomes (1, 2, 30, 31, 32, 100, 102)
   *
   * Used on a vector of vectors that *has* added filenames, the following behaviour is observed:
   *
   * ((1), (2), (30, 31, 32), (100), (102)) becomes (1, 2, 30, 31, 32, 100, 102)
   *
   * @param - a vector of vectors, containing all the file names.
   * @return a single vector containing all the file names.
   */
  std::vector<std::string> MultipleFileProperty::flattenFileNames(
    const std::vector<std::vector<std::string> > & fileNames)
  {
    std::vector<std::string> flattenedFileNames;

    std::vector<std::vector<std::string> >::const_iterator it = fileNames.begin();

    for(; it != fileNames.end(); ++it)
    {
      flattenedFileNames.insert(
        flattenedFileNames.end(),
        it->begin(), it->end());
    }

    return flattenedFileNames;
  }

  //////////////////////////////////////////////////////////////////////
  // Anonymous
  //////////////////////////////////////////////////////////////////////

  namespace
  {
    // Functor with overloaded function operator, for use with the "accumulate" STL algorithm.
    // Has state to store extensions.
    AppendFullFileName:: AppendFullFileName(const std::vector<std::string> & exts) :
      m_exts(exts)
    {}

    /**
     * Takes in a vector of filenames, tries to find their full path if possible, then cumulatively appends 
     * them to the result string.
     *
     * @param result :: the cumulative result so far
     * @param fileNames :: the name to look for, and append to the result
     * @return the cumulative result, after the filenames have been appended.
     */
    std::string & AppendFullFileName::operator()(std::string & result, const std::vector<std::string> & fileNames)
    {
      // Append nothing if there are no file names to add.
      if(fileNames.empty())
        return result;

      if(!result.empty())
        result += ",";

      AppendFullFileName appendFullFileName(m_exts);

      // Change each file name into a full file name, and append each one onto a plus-separated string.
      std::string fullFileNames;
      fullFileNames = std::accumulate(
        fileNames.begin(), fileNames.end(),
        fullFileNames, 
        appendFullFileName); // Call other overloaded operator on each filename in vector
        
      // Append the file names to result, and return them.
      result += fullFileNames;
      return result;
    }

    /**
     * Takes in a filename, tries to find it's full path if possible, then cumulatively appends it to a result string.
     *
     * @param result :: the cumulative result so far
     * @param fileName :: the name to look for, and append to the result
     * @return the cumulative result, after the filename has been appended.
     * @throws std::runtime_error if an individual filename could not be set to the FileProperty object
     */
    std::string & AppendFullFileName::operator()(std::string & result, const std::string & fileName)
    {
      // Append nothing if there is no file name to add.
      if(fileName.empty())
        return result;

      if(!result.empty())
        result += "+";
      
      // Trim whitespace from filename.
      std::string value = fileName;
      boost::algorithm::trim(value);

      // Initialise a "slave" FileProperty object to do all the work.
      FileProperty slaveFileProp("Slave", "", FileProperty::Load, m_exts, Direction::Input);

      std::string error = slaveFileProp.setValue(value);

      // If an error was returned then we throw it out of the functor, to be
      // returned by MultiFileProperty.setvalue(...).
      if(!error.empty())
        throw std::runtime_error(error);

      // Append the file name to result, and return it.
      result += slaveFileProp();
      return result;
    }

    /**
     * Turn (1, 2, 30, 31) into ((1), (2), (30), (31)).
     */
    std::vector<std::vector<std::string> > unflattenFileNames(
      const std::vector<std::string> & flattenedFileNames)
    {
      std::vector<std::vector<std::string> > unflattenedFileNames;

      std::vector<std::string>::const_iterator it = flattenedFileNames.begin();

      for(; it != flattenedFileNames.end(); ++it)
      {
        unflattenedFileNames.push_back(
          std::vector<std::string>(1,(*it)));
      }

      return unflattenedFileNames;
    }

    /**
     * Converts a vector of vector of strings into a single comma and plus separated string.
     * For example [["a", "b"],["x", "y", "z"]] into "a+b,x+y+z".
     *
     * @param - vector of vector of strings (filenames).
     *
     * @returns a single comma and plus separated string.
     */
    std::string toSingleString(const std::vector<std::vector<std::string>> & filenames)
    {
      std::string result;

      for( auto filenameList = filenames.begin(); filenameList != filenames.end(); ++filenameList)
      {
        std::string innerResult = "";
        
        for( auto filename = filenameList->begin(); filename != filenameList->end(); ++filename)
        {
          if( ! innerResult.empty() )
            innerResult += "+";
          innerResult += *filename;
        }

        if( ! result.empty() )
          result += ",";
        result += innerResult;
      }

      return result;
    }
  } // anonymous namespace

} // namespace Mantid
} // namespace API
