#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/System.h"
#include "MantidKernel/MultiFileValidator.h"
#include "MantidKernel/Property.h"
#include <Poco/Path.h>
#include "MantidAPI/FileFinder.h"

#include <ctype.h>

#include <boost/algorithm/string.hpp>

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
  }
  
  /** Constructor
   *
   * @param name ::          The name of the property
   * @param exts ::          The allowed/suggested extensions
   * @param optional ::      If ture, the property is optional
   */
  MultipleFileProperty::MultipleFileProperty(
    const std::string & name,
    const std::vector<std::string> & exts
  ) : PropertyWithValue<std::vector<std::vector<std::string> > >(
        name, 
        std::vector<std::vector<std::string> >(), 
        new MultiFileValidator(exts), 
        Direction::Input),
      m_exts(exts),
      m_parser()
  {}

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MultipleFileProperty::~MultipleFileProperty() {}

  /** Set the value, with a comma- and plus-separated string of filenames
   *
   * @param propValue :: comma- and plus-separated string of filenames
   * @return A string indicating the outcome of the attempt to set the property. An empty string indicates success.
   */
  std::string MultipleFileProperty::setValue(const std::string & propValue)
  {
    // No empty value is allowed.
    if( propValue.empty())
      return "No file(s) specified.";

    std::string value = propValue;
    
    // Remove whitespace.
    value.erase(std::remove_if( // ("Erase-remove" idiom.)
        value.begin(), value.end(),
        isspace),
      value.end());
    
    std::stringstream errorMsg;

    // Assume a format of "dir/inst_1,2,...n.raw", and try to parse using parser.
    try
    {
      m_parser.parse(value);
    }
    catch(const std::runtime_error & re)
    {
      errorMsg << "Unable to parse multi file runs: \"" << re.what() << "\". ";
    }

    std::vector<std::vector<std::string> > fileNames = m_parser.fileNames();

    AppendFullFileName appendFullFileName(m_exts);
    std::string fullFileNames("");

    // If unsuccessful, then assume a format of:
    //
    // "dir/inst_1.raw, dir/inst_2.raw, ...  dir/inst_n.raw" (where n may equal 1).
    //
    // Tokenise on commas, and and try to find full files names of each token.
    if(fileNames.empty())
    {
      std::vector<std::string> tokens;
      tokens = boost::split(tokens, value, boost::is_any_of(","));
      fileNames = unflattenFileNames(tokens);
      try
      {
        fullFileNames = std::accumulate(
          fileNames.begin(), fileNames.end(),
          std::string(""),
          appendFullFileName);
      }
      catch(const std::runtime_error & re)
      {
        errorMsg << "Tried to find as single file(s), but also failed: \"" << re.what() << "\".";
        return errorMsg.str();
      }
    }
    // Else, for each file name in the vector, change it into a full file name where possible,
    // then append it onto a comma- and plus-separated string.
    else
    {
      // If there is only one file, then we should use the string passed to the property, which
      // has not been tampered with. This will enable us to deal with the case where a user is 
      // trying to load a single file with incorrect zero padding, or some other anomaly.
      if(fileNames.size() == 1 && fileNames[0].size() == 1)
        fileNames[0][0] = propValue;

      try
      {
        fullFileNames = std::accumulate(
          fileNames.begin(), fileNames.end(),
          std::string(""),
          appendFullFileName);
      }
      catch(const std::runtime_error & re)
      {
        return re.what();
      }
    }  

    // Now re-set the value using the full paths found.
    return PropertyWithValue<std::vector<std::vector<std::string> > >::setValue(fullFileNames);
  }

  /**
   * Set a property value via a DataItem
   * @param data :: A shared pointer to a data item
   * @return "" if the assignment was successful or a user level description of the problem
  */
  std::string MultipleFileProperty::setValue(const boost::shared_ptr<Kernel::DataItem> data )
  {
    // Implemented this method for documentation reasons. Just calls base class method.
    return PropertyWithValue<std::vector<std::vector<std::string> > >::setValue(data);
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

    /** Takes in a vector of filenames, tries to find their full path if possible, then cumulatively appends 
      *  them to the result string.
      *  @param result :: the cumulative result so far
      *  @param fileNames :: the name to look for, and append to the result
      *  @return the cumulative result, after the filenames have been appended.
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

    /** Takes in a filename, tries to find it's full path if possible, then cumulatively appends it to a result string.
      *  @param result :: the cumulative result so far
      *  @param fileName :: the name to look for, and append to the result
      *  @return the cumulative result, after the filename has been appended.
      *  @throws std::runtime_error if an individual filename could not be set to the FileProperty object
      */
    std::string & AppendFullFileName::operator()(std::string & result, const std::string & fileName)
    {
      // Append nothing if there is no file name to add.
      if(fileName.empty())
        return result;

      if(!result.empty())
        result += "+";

      // Initialise a "slave" FileProperty object to do all the work.
      FileProperty slaveFileProp(
        "Slave",
        "",
        FileProperty::Load,
        m_exts,
        Direction::Input);

      std::string error = slaveFileProp.setValue(fileName);

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
  } // anonymous namespace

} // namespace Mantid
} // namespace API
