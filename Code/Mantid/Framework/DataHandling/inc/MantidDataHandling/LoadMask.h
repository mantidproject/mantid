#ifndef MANTID_DATAHANDLING_LOADMASK_H_
#define MANTID_DATAHANDLING_LOADMASK_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"

namespace Poco {
namespace XML {
class Document;
class Element;
}
}

namespace Mantid {
namespace DataHandling {

/** LoadMask : Load masking file to generate a SpecialWorkspace2D object
  (masking workspace).

  @author
  @date 2011-11-02

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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
class DLLExport LoadMask : public API::Algorithm {
public:
  LoadMask();
  ~LoadMask();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "LoadMask"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Load file containing masking information to a SpecialWorkspace2D "
           "(masking workspace). This algorithm is renamed from "
           "LoadMaskingFile.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "DataHandling;Transforms\\Masking";
  }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();
  /// Initialize XML parser
  void initializeXMLParser(const std::string &filename);
  /// Parse XML
  void parseXML();
  /// Convert value to data
  void parseComponent(std::string valuetext, bool tomask);
  /// Convert value to detector ids
  void parseDetectorIDs(std::string inputstr, bool tomask);
  /// Convert value to spectrum ids
  void parseSpectrumIDs(std::string inputstr, bool tomask);
  /// Split a string
  void splitString(std::string inputstr, std::vector<std::string> &strings,
                   std::string sep);
  /// Split and convert string
  void parseRangeText(std::string inputstr, std::vector<int32_t> &singles,
                      std::vector<int32_t> &pairs);
  /// Initialize a Mask Workspace
  void intializeMaskWorkspace();
  /// Convert component to detectors
  void componentToDetectors(std::vector<std::string> componentnames,
                            std::vector<int32_t> &detectors);
  /// Convert bank to detector
  void bankToDetectors(std::vector<std::string> singlebanks,
                       std::vector<int32_t> &detectors,
                       std::vector<int32_t> &detectorpairslow,
                       std::vector<int32_t> &detectorpairsup);
  /// Convert input detector to detector
  void detectorToDetectors(std::vector<int32_t> singles,
                           std::vector<int32_t> pairslow,
                           std::vector<int32_t> pairsup,
                           std::vector<int32_t> &detectors,
                           std::vector<int32_t> &detectorpairslow,
                           std::vector<int32_t> &detectorpairsup);
  void processMaskOnDetectors(bool tomask, std::vector<int32_t> singledetids,
                              std::vector<int32_t> pairdetids_low,
                              std::vector<int32_t> pairdetids_up);
  /// Convert spectrum to detector
  void processMaskOnWorkspaceIndex(bool mask, std::vector<int32_t> pairslow,
                                   std::vector<int32_t> pairsup);
  void initDetectors();

  void loadISISMaskFile(std::string isisfilename);
  void parseISISStringToVector(std::string ins, std::vector<int> &rangestartvec,
                               std::vector<int> &rangeendvec);

  /// Mask Workspace
  DataObjects::MaskWorkspace_sptr m_MaskWS;
  /// Instrument name
  std::string m_instrumentPropValue;
  /// XML document loaded
  Poco::XML::Document *m_pDoc;
  /// Root element of the parsed XML
  Poco::XML::Element *m_pRootElem;

  /// Default setup.  If true, not masking, but use the pixel
  bool m_DefaultToUse;

  std::vector<int32_t> mask_detid_single;
  std::vector<int32_t> mask_specid_single;
  std::vector<int32_t> mask_detid_pair_low;
  std::vector<int32_t> mask_specid_pair_low;
  std::vector<int32_t> mask_detid_pair_up;
  std::vector<int32_t> mask_specid_pair_up;
  std::vector<std::string> mask_bankid_single;

  std::vector<int32_t> unmask_detid_single;
  std::vector<int32_t> unmask_specid_single;
  std::vector<int32_t> unmask_detid_pair_low;
  std::vector<int32_t> unmask_specid_pair_low;
  std::vector<int32_t> unmask_detid_pair_up;
  std::vector<int32_t> unmask_specid_pair_up;
  std::vector<std::string> unmask_bankid_single;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADMASK_H_ */
