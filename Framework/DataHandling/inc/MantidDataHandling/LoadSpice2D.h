#ifndef MANTID_DATAHANDLING_LoadSpice2D_H
#define MANTID_DATAHANDLING_LoadSpice2D_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/XmlHandler.h"
#include <map>
#include <string>
#include <vector>
//----------------------------------------------------------------------

namespace Poco {
namespace XML {
class Element;
}
}

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
  /// default constructor
  LoadSpice2D();
  /// destructor
  ~LoadSpice2D();
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LoadSpice2D"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Loads a SANS data file produce by the HFIR instruments at ORNL. "
           "The instrument geometry is also loaded. The center of the detector "
           "is placed at (0,0,D), where D is the sample-to-detector distance.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const {
    return "DataHandling\\Text;SANS\\DataHandling";
  }
  /// Number of monitors
  static const int nMonitors = 2;

  /// Returns a confidence value that this algorithm can load a file
  virtual int confidence(Kernel::FileDescriptor &descriptor) const;

private:
  /// Overwrites Algorithm method.
  void init();
  /// Overwrites Algorithm method
  void exec();

  /// This method throws not found error if a element is not found in the xml
  /// file
  void throwException(Poco::XML::Element *elem, const std::string &name,
                      const std::string &fileName);
  /// Run LoadInstrument Child Algorithm
  void runLoadInstrument(const std::string &inst_name,
                         DataObjects::Workspace2D_sptr localWorkspace);
  /// Run the LoadMappingTable Child Algorithm to fill the SpectraToDetectorMap
  void runLoadMappingTable(DataObjects::Workspace2D_sptr localWorkspace,
                           int nxbins, int nybins);

  void setInputPropertiesAsMemberProperties();

  void addMetadataAsRunProperties(const std::map<std::string, std::string> &);
  void parseDetectorDimensions(const std::string &);
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
  void moveDetector(double sample_detector_distance);
  double detectorDistance(std::map<std::string, std::string> &metadata);
  void setMetadataAsRunProperties(std::map<std::string, std::string> &metadata);

  // Member variables:
  DataObjects::Workspace2D_sptr m_workspace;
  double m_wavelength_input;
  double m_wavelength_spread_input;
  Mantid::DataHandling::XmlHandler m_xmlHandler;
  int m_numberXPixels;
  int m_numberYPixels;
  double m_wavelength;
  double m_dwavelength;
};
} // namespace DataHandling
} // namespace Mantid

#endif
