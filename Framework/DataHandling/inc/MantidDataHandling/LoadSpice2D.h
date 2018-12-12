// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LoadSpice2D_H
#define MANTID_DATAHANDLING_LoadSpice2D_H

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/XmlHandler.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/DateAndTime.h"
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Poco {
namespace XML {
class Element;
}
} // namespace Poco

namespace Mantid {
namespace DataHandling {
/** @class LoadSpice2D  DataHandling/LoadSpice2D.h

 This algorithm loads a SPICE2D file for HFIR SANS into a workspace.

 Required properties:
 <UL>
 <LI> OutputWorkspace - The name of workspace to be created.</LI>
 <LI> Filename - Name of the file to load</LI>
 </UL>

 @author Mathieu Doucet, Oak Ridge National Laboratory
 @author Ricardo Leal, Oak Ridge National Laboratory

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

 File change history is stored at: <https://github.com/mantidproject/mantid>.
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport LoadSpice2D : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadSpice2D"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a SANS data file produce by the HFIR instruments at ORNL. "
           "The instrument geometry is also loaded. The center of the detector "
           "is placed at (0,0,D), where D is the sample-to-detector distance.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadSpiceAscii", "LoadSpiceXML2DDet"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Text;SANS\\DataHandling";
  }
  /// Number of monitors
  static const int nMonitors = 2;

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;

  /// This method throws not found error if a element is not found in the xml
  /// file
  void throwException(Poco::XML::Element *elem, const std::string &name,
                      const std::string &fileName);
  /// Run LoadInstrument Child Algorithm
  void runLoadInstrument(const std::string &inst_name,
                         DataObjects::Workspace2D_sptr localWorkspace);

  void setInputPropertiesAsMemberProperties();

  void addMetadataAsRunProperties(const std::map<std::string, std::string> &);
  std::pair<int, int> parseDetectorDimensions(const std::string &);
  void createWorkspace();
  std::vector<int> getData(const std::string &);
  void createWorkspace(const std::vector<int> &data, const std::string &title,
                       double monitor1_counts, double monitor2_counts);
  void setWavelength(std::map<std::string, std::string> &metadata);
  template <class T>
  T addRunProperty(std::map<std::string, std::string> &metadata,
                   const std::string &oldName, const std::string &newName,
                   const std::string &units = "");
  template <class T>
  void addRunProperty(const std::string &name, const T &value,
                      const std::string &units = "");
  void setBeamTrapRunProperty(std::map<std::string, std::string> &metadata);
  void detectorDistance(std::map<std::string, std::string> &metadata);
  void detectorTranslation(std::map<std::string, std::string> &metadata);
  void setMetadataAsRunProperties(std::map<std::string, std::string> &metadata);
  void rotateDetector(const double &);
  void setTimes();
  void
  setSansSpiceXmlFormatVersion(std::map<std::string, std::string> &metadata);

  // Member variables:
  DataObjects::Workspace2D_sptr m_workspace;
  double m_wavelength_input{0.0};
  double m_wavelength_spread_input{0.0};
  Mantid::DataHandling::XmlHandler m_xmlHandler;
  double m_wavelength{0.0};
  double m_dwavelength{0.0};
  double m_sansSpiceXmlFormatVersion{0.0};
  Mantid::Types::Core::DateAndTime m_startTime;
  Mantid::Types::Core::DateAndTime m_endTime;
};
} // namespace DataHandling
} // namespace Mantid

#endif
