#ifndef MANTID_DATAHANDLING_LOADMASK_H_
#define MANTID_DATAHANDLING_LOADMASK_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
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
  ~LoadMask() override;

  /// Algorithm's name for identification
  const std::string name() const override { return "LoadMask"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load file containing masking information to a SpecialWorkspace2D "
           "(masking workspace).";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\Masking;Transforms\\Masking";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
  /// Initialize XML parser
  void initializeXMLParser(const std::string &filename);
  /// Parse XML
  void parseXML();
  /// Convert value to data
  void parseComponent(const std::string &valuetext, bool tomask);
  /// Convert value to detector ids
  void parseDetectorIDs(const std::string &inputstr, bool tomask);
  /// Convert value to spectrum Nos
  void parseSpectrumNos(const std::string &inputstr,
                        std::vector<int32_t> &targetMask);
  /// Split a string
  void splitString(const std::string &inputstr,
                   std::vector<std::string> &strings, std::string sep);
  /// Split and convert string
  void parseRangeText(const std::string &inputstr,
                      std::vector<int32_t> &singles,
                      std::vector<int32_t> &pairs);
  /// Initialize a Mask Workspace
  void intializeMaskWorkspace();
  /// Convert component to detectors
  void componentToDetectors(const std::vector<std::string> &componentnames,
                            std::vector<int32_t> &detectors);
  /// Convert bank to detector
  void bankToDetectors(const std::vector<std::string> &singlebanks,
                       std::vector<int32_t> &detectors,
                       std::vector<int32_t> &detectorpairslow,
                       std::vector<int32_t> &detectorpairsup);
  /// Convert input detector to detector
  void detectorToDetectors(const std::vector<int32_t> &singles,
                           const std::vector<int32_t> &pairslow,
                           const std::vector<int32_t> &pairsup,
                           std::vector<int32_t> &detectors,
                           std::vector<int32_t> &detectorpairslow,
                           std::vector<int32_t> &detectorpairsup);
  void processMaskOnDetectors(const detid2index_map &indexmap, bool tomask,
                              std::vector<int32_t> singledetids,
                              std::vector<int32_t> pairdetids_low,
                              std::vector<int32_t> pairdetids_up);
  /// Convert spectrum to detector
  void processMaskOnWorkspaceIndex(bool mask,
                                   std::vector<int32_t> &maskedSpecID,
                                   std::vector<int32_t> &singleDetIds);

  void initDetectors();

  void loadISISMaskFile(const std::string &isisfilename,
                        std::vector<int32_t> &spectraMasks);
  void parseISISStringToVector(const std::string &ins,
                               std::vector<int> &rangestartvec,
                               std::vector<int> &rangeendvec);

  std::map<std::string, std::string> validateInputs() override;

  void convertSpMasksToDetIDs(const API::MatrixWorkspace_sptr &SourceWS,
                              const std::vector<int32_t> &maskedSpecID,
                              std::vector<int32_t> &singleDetIds);

  /// Mask Workspace
  DataObjects::MaskWorkspace_sptr m_maskWS;
  /// Instrument name
  std::string m_instrumentPropValue;
  /// optional source workspace, containing spectra-detector mapping
  API::MatrixWorkspace_sptr m_sourceMapWS;
  /// XML document loaded
  Poco::XML::Document *m_pDoc;
  /// Root element of the parsed XML
  Poco::XML::Element *m_pRootElem;

  /// Default setup.  If true, not masking, but use the pixel
  bool m_defaultToUse;

  std::vector<int32_t> mask_detid_single;
  std::vector<int32_t> mask_detid_pair_low;
  std::vector<int32_t> mask_detid_pair_up;

  std::vector<int32_t> unmask_detid_single;
  std::vector<int32_t> unmask_detid_pair_low;
  std::vector<int32_t> unmask_detid_pair_up;

  // spectra mask provided
  std::vector<int32_t> m_MaskSpecID;
  // spectra unmask provided
  std::vector<int32_t> m_unMaskSpecID;

  std::vector<std::string> mask_bankid_single;
  std::vector<std::string> unmask_bankid_single;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADMASK_H_ */
