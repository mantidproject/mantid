#ifndef MANTID_DATAHANDLING_LOADMASK_H_
#define MANTID_DATAHANDLING_LOADMASK_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/ParallelAlgorithm.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/System.h"

#include <Poco/AutoPtr.h>
#include <Poco/DOM/Document.h>

namespace Poco {
namespace XML {
class Document;
class Element;
} // namespace XML
} // namespace Poco

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
class DLLExport LoadMask : public API::ParallelAlgorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "LoadMask"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load file containing masking information to a SpecialWorkspace2D "
           "(masking workspace).";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"ExportSpectraMask", "LoadMask"};
  }
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
  /// Initialize a Mask Workspace
  void intializeMaskWorkspace();
  /// Convert component to detectors
  void componentToDetectors(const std::vector<std::string> &componentnames,
                            std::vector<detid_t> &detectors);
  /// Convert bank to detector
  void bankToDetectors(const std::vector<std::string> &singlebanks,
                       std::vector<detid_t> &detectors);

  void processMaskOnDetectors(const detid2index_map &indexmap, bool tomask,
                              const std::vector<detid_t> &singledetids);
  /// Convert spectrum to detector
  void processMaskOnWorkspaceIndex(bool mask,
                                   std::vector<specnum_t> &maskedSpecID,
                                   std::vector<detid_t> &singleDetIds);

  void initDetectors();

  std::map<std::string, std::string> validateInputs() override;

  void convertSpMasksToDetIDs(const API::MatrixWorkspace &sourceWS,
                              const std::vector<specnum_t> &maskedSpecID,
                              std::vector<detid_t> &singleDetIds);

  void reset();

  /// Mask Workspace
  DataObjects::MaskWorkspace_sptr m_maskWS;
  /// Instrument name
  std::string m_instrumentPropValue;
  /// optional source workspace, containing spectra-detector mapping
  API::MatrixWorkspace_sptr m_sourceMapWS;
  /// XML document loaded
  Poco::AutoPtr<Poco::XML::Document> m_pDoc;
  /// Root element of the parsed XML
  Poco::XML::Element *m_pRootElem{nullptr};

  /// Default setup.  If true, not masking, but use the pixel
  bool m_defaultToUse{true};

  // detector id-s to mask
  std::vector<detid_t> m_maskDetID;
  // spectrum id-s to unmask
  std::vector<detid_t> m_unMaskDetID;

  // spectra mask provided
  std::vector<specnum_t> m_maskSpecID;
  // spectra unmask provided NOT IMPLEMENTED
  // std::vector<specnum_t> m_unMaskSpecID;

  std::vector<std::string> m_maskCompIdSingle;
  std::vector<std::string> m_uMaskCompIdSingle;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADMASK_H_ */
