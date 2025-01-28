// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/GroupingWorkspace.h"

#include <Poco/DOM/Document.h>

#include <fstream>

namespace Poco {
namespace XML {
class Element;
class Node;
} // namespace XML
} // namespace Poco

namespace Mantid {
namespace DataHandling {

/**
  LoadDetectorsGroupingFile

  Algorithm used to generate a GroupingWorkspace from an .xml or .map file
  containing the
  detectors' grouping information.

  @date 2011-11-17
*/
class MANTID_DATAHANDLING_DLL LoadDetectorsGroupingFile final : public API::Algorithm {
public:
  ///
  const std::string name() const override { return "LoadDetectorsGroupingFile"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load an XML or Map file, which contains definition of detectors "
           "grouping, to a GroupingWorkspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"SaveDetectorsGrouping", "GroupDetectors"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Grouping;Transforms\\Grouping"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Initialize XML parser
  void initializeXMLParser(const std::string &filename);
  /// Parse XML
  void parseXML();
  /// Initialize a Mask Workspace
  void intializeGroupingWorkspace();
  /// Set workspace->group ID map by components
  void setByComponents();
  /// Set workspace->group ID map by detectors (range)
  void setByDetectors();
  /// Set workspace index/group ID by spectrum Number
  void setBySpectrumNos();
  /// Convert detector ID combination string to vector of detectors
  void parseDetectorIDs(std::string inputstring, std::vector<detid_t> &detids);
  /// Convert spectrum Nos combination string to vector of spectrum Nos
  void parseSpectrumNos(std::string inputstring, std::vector<int> &specids);
  /// Get attribute value from an XML node
  static std::string getAttributeValueByName(Poco::XML::Node *pNode, std::string attributename, bool &found);
  /// Split and convert string
  void parseRangeText(std::string inputstr, std::vector<int32_t> &singles, std::vector<int32_t> &pairs);
  /// Generate a GroupingWorkspace w/o instrument
  void generateNoInstrumentGroupWorkspace();

  /// Grouping Workspace
  DataObjects::GroupingWorkspace_sptr m_groupWS;

  /// Instrument to use if given by user
  Geometry::Instrument_const_sptr m_instrument;

  /// XML document loaded
  Poco::XML::Document *m_pDoc{nullptr};
  /// Root element of the parsed XML
  Poco::XML::Element *m_pRootElem{nullptr};

  /// Data structures to store XML to Group/Detector conversion map
  std::map<int, std::vector<std::string>> m_groupComponentsMap;
  std::map<int, std::vector<detid_t>> m_groupDetectorsMap;
  std::map<int, std::vector<int>> m_groupSpectraMap;
};

class MANTID_DATAHANDLING_DLL LoadGroupXMLFile {
public:
  LoadGroupXMLFile();

  void loadXMLFile(const std::string &xmlfilename);
  void setDefaultStartingGroupID(int startgroupid) { m_startGroupID = startgroupid; }

  std::string getInstrumentName() { return m_instrumentName; }
  bool isGivenInstrumentName() const { return m_userGiveInstrument; }

  const std::string &getDate() const { return m_date; }
  bool isGivenDate() const { return m_userGiveDate; }

  const std::string &getDescription() const { return m_description; }
  bool isGivenDescription() const { return m_userGiveDescription; }

  /// Data structures to store XML to Group/Detector conversion map
  const std::map<int, std::vector<std::string>> &getGroupComponentsMap() const { return m_groupComponentsMap; }
  const std::map<int, std::vector<detid_t>> &getGroupDetectorsMap() const { return m_groupDetectorsMap; }
  const std::map<int, std::vector<int>> &getGroupSpectraMap() const { return m_groupSpectraMap; }

  std::map<int, std::string> getGroupNamesMap() { return m_groupNamesMap; }

private:
  /// Instrument name
  std::string m_instrumentName;
  /// User-define instrument name
  bool m_userGiveInstrument;

  /// Date in ISO 8601 for which this grouping is relevant
  std::string m_date;
  /// Whether date is given by user
  bool m_userGiveDate;

  /// Grouping description. Empty if not specified.
  std::string m_description;
  /// Whether description is given by user
  bool m_userGiveDescription;

  /// XML document loaded
  Poco::AutoPtr<Poco::XML::Document> m_pDoc;
  /// Data structures to store XML to Group/Detector conversion map
  std::map<int, std::vector<std::string>> m_groupComponentsMap;
  std::map<int, std::vector<detid_t>> m_groupDetectorsMap;
  std::map<int, std::vector<int>> m_groupSpectraMap;
  int m_startGroupID;

  /// Map of group names
  std::map<int, std::string> m_groupNamesMap;

  /// Initialize XML parser
  void initializeXMLParser(const std::string &filename);
  /// Parse XML
  void parseXML();
  /// Get attribute value from an XML node
  static std::string getAttributeValueByName(Poco::XML::Node *pNode, const std::string &attributename, bool &found);
};

/**
 * Class used to load a grouping information from .map file.
 *
 * @author Arturs Bekasovs
 * @date 21/08/2013
 */
class MANTID_DATAHANDLING_DLL LoadGroupMapFile {
public:
  /// Constructor. Opens a file.
  LoadGroupMapFile(const std::string &fileName, Kernel::Logger &log);

  /// Desctructor. Closes a file.
  ~LoadGroupMapFile();

  /// Parses grouping information from .map file
  void parseFile();

  /// Return the map parsed from file. Should only be called after the file is
  /// parsed,
  /// otherwise a map will always be empty.
  std::map<int, std::vector<int>> getGroupSpectraMap() { return m_groupSpectraMap; }

private:
  /// Skips all the empty lines and comment lines, and returns next line with
  /// real data
  bool nextDataLine(std::string &line);

  /// The name of the file being parsed
  const std::string m_fileName;

  /// Logger used
  Kernel::Logger &m_log;

  /// group_id -> [list of spectra]
  std::map<int, std::vector<int>> m_groupSpectraMap;

  /// The file being parsed
  std::ifstream m_file;

  /// Number of the last line parsed
  int m_lastLineRead;
};

} // namespace DataHandling
} // namespace Mantid
