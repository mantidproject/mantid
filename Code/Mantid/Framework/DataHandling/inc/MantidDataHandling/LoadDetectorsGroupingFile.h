#ifndef MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILE_H_
#define MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/GroupingWorkspace.h"

#include <fstream>

namespace Poco { namespace XML {
  class Document;
  class Element;
  class Node;
}}

namespace Mantid
{
namespace DataHandling
{

  /**
    LoadDetectorsGroupingFile

    Algorithm used to generate a GroupingWorkspace from an .xml or .map file containing the
    detectors' grouping information.
    
    @date 2011-11-17

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport LoadDetectorsGroupingFile: public API::Algorithm
  {
  public:
    LoadDetectorsGroupingFile();
    virtual ~LoadDetectorsGroupingFile();
    
    ///
    virtual const std::string name() const { return "LoadDetectorsGroupingFile";};
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Load an XML or Map file, which contains definition of detectors grouping, to a GroupingWorkspace.";}

    /// Algorithm's version for identification
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "DataHandling;Transforms\\Grouping";}

   private:
     
     /// Initialise the properties
     void init();
     /// Run the algorithm
     void exec();
     /// Initialize XML parser
     void initializeXMLParser(const std::string & filename);
     /// Parse XML
     void parseXML();
     /// Initialize a Mask Workspace
     void intializeGroupingWorkspace();
     /// Set workspace->group ID map by components
     void setByComponents();
     /// Set workspace->group ID map by detectors (range)
     void setByDetectors();
     /// Set workspace index/group ID by spectraum ID
     void setBySpectrumIDs();
     /// Convert detector ID combination string to vector of detectors
     void parseDetectorIDs(std::string inputstring, std::vector<detid_t>& detids);
     /// Convert spectrum IDs combintation string to vector of spectrum ids
     void parseSpectrumIDs(std::string inputstring, std::vector<int>& specids);
     /// Get attribute value from an XML node
     static std::string getAttributeValueByName(Poco::XML::Node* pNode, std::string attributename, bool& found);
     /// Split and convert string
     void parseRangeText(std::string inputstr, std::vector<int32_t>& singles, std::vector<int32_t>& pairs);
     /// Generate a GroupingWorkspace w/o instrument
     void generateNoInstrumentGroupWorkspace();

     /// Grouping Workspace
     DataObjects::GroupingWorkspace_sptr mGroupWS;

     /// Instrument to use if given by user
     Geometry::Instrument_const_sptr mInstrument;

     /// XML document loaded
     Poco::XML::Document* pDoc;
     /// Root element of the parsed XML
     Poco::XML::Element* pRootElem;

     /// Data structures to store XML to Group/Detector conversion map
     std::map<int, std::vector<std::string> > mGroupComponentsMap;
     std::map<int, std::vector<detid_t> > mGroupDetectorsMap;
     std::map<int, std::vector<int> > mGroupSpectraMap;



  };


  class DLLExport LoadGroupXMLFile
  {
  public:
    LoadGroupXMLFile();
    ~LoadGroupXMLFile();

    void loadXMLFile(std::string xmlfilename);
    void setDefaultStartingGroupID(int startgroupid)
    {
      mStartGroupID = startgroupid;
    }

    std::string getInstrumentName() { return mInstrumentName; }
    bool isGivenInstrumentName() { return mUserGiveInstrument; }

    std::string getDate() { return mDate; }
    bool isGivenDate() { return mUserGiveDate; }

    std::string getDescription() { return mDescription; }
    bool isGivenDescription() { return mUserGiveDescription; }

    /// Data structures to store XML to Group/Detector conversion map
    std::map<int, std::vector<std::string> > getGroupComponentsMap()
    {
      return mGroupComponentsMap;
    }
    std::map<int, std::vector<detid_t> > getGroupDetectorsMap()
    {
      return  mGroupDetectorsMap;
    }
    std::map<int, std::vector<int> > getGroupSpectraMap()
    {
      return mGroupSpectraMap;
    }

    std::map<int, std::string> getGroupNamesMap()
    {
      return mGroupNamesMap;
    }

  private:
    /// Instrument name
    std::string mInstrumentName;
    /// User-define instrument name
    bool mUserGiveInstrument;

    /// Date in ISO 8601 for which this grouping is relevant
    std::string mDate;
    /// Whether date is given by user
    bool mUserGiveDate;

    /// Grouping description. Empty if not specified.
    std::string mDescription;
    /// Whether description is given by user
    bool mUserGiveDescription;

    /// XML document loaded
    Poco::XML::Document* pDoc;
    /// Root element of the parsed XML
    Poco::XML::Element* pRootElem;
    /// Data structures to store XML to Group/Detector conversion map
    std::map<int, std::vector<std::string> > mGroupComponentsMap;
    std::map<int, std::vector<detid_t> > mGroupDetectorsMap;
    std::map<int, std::vector<int> > mGroupSpectraMap;
    int mStartGroupID;

    /// Map of group names
    std::map<int, std::string> mGroupNamesMap;

    /// Initialize XML parser
    void initializeXMLParser(const std::string & filename);
    /// Parse XML
    void parseXML();
    /// Get attribute value from an XML node
    static std::string getAttributeValueByName(Poco::XML::Node* pNode, std::string attributename, bool& found);

  };

  /**
   * Class used to load a grouping information from .map file.
   *
   * @author Arturs Bekasovs
   * @date 21/08/2013
   */
  class DLLExport LoadGroupMapFile
  {
  public:
    /// Constructor. Opens a file.
    LoadGroupMapFile(const std::string& fileName, Kernel::Logger& log);

    /// Desctructor. Closes a file.
    ~LoadGroupMapFile();

    /// Parses grouping information from .map file
    void parseFile();

    /// Return the map parsed from file. Should only be called after the file is parsed,
    /// otherwise a map will always be empty.
    std::map<int, std::vector<int> > getGroupSpectraMap() { return m_groupSpectraMap; }

  private:

    /// Skips all the empty lines and comment lines, and returns next line with real data
    bool nextDataLine(std::string& line);

    /// The name of the file being parsed
    const std::string m_fileName;

    /// Logger used
    Kernel::Logger& m_log;

    /// group_id -> [list of spectra]
    std::map<int, std::vector<int> > m_groupSpectraMap;

    /// The file being parsed
    std::ifstream m_file;

    /// Number of the last line parsed
    int m_lastLineRead;
  };

} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILE_H_ */
