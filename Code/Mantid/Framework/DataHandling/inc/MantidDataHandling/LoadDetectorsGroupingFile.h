#ifndef MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILE_H_
#define MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/GroupingWorkspace.h"

namespace Mantid
{
namespace DataHandling
{

  /** LoadDetectorsGroupingFile : TODO: DESCRIPTION
    
    @date 2011-11-17

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    /// Algorithm's version for identification
    virtual int version() const { return 1;};
    /// Algorithm's category for identification
    virtual const std::string category() const { return "DataHandling;Transforms\\Grouping";}

   private:
     /// Sets documentation strings for this algorithm
     virtual void initDocs();
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
     /// Instrument name
     std::string mInstrumentName;
     /// User-define instrument name
     bool mUserGiveInstrument;
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

    std::string getInstrumentName()
    {
      return mInstrumentName;
    }
    bool isGivenInstrumentName()
    {
      return mUserGiveInstrument;
    }

    std::string getDescription()
    {
        return mDescription;
    }

    bool isGivenDescription()
    {
        return mUserGiveDescription;
    }

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
    /// Convert detector ID combination string to vector of detectors
    void parseDetectorIDs(std::string inputstring, std::vector<detid_t>& detids);
    /// Convert spectrum IDs combintation string to vector of spectrum ids
    void parseSpectrumIDs(std::string inputstring, std::vector<int>& specids);
    /// Get attribute value from an XML node
    static std::string getAttributeValueByName(Poco::XML::Node* pNode, std::string attributename, bool& found);
    /// Split and convert string
    void parseRangeText(std::string inputstr, std::vector<int32_t>& singles, std::vector<int32_t>& pairs);

  };

} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_LOADDETECTORSGROUPINGFILE_H_ */
