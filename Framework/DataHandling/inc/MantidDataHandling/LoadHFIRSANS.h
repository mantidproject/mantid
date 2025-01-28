// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/XmlHandler.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/FileDescriptor.h"

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
/** @class LoadHFIRSANS  DataHandling/LoadHFIRSANS.h

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

class MANTID_DATAHANDLING_DLL LoadHFIRSANS : public API::IFileLoader<Kernel::FileDescriptor> {

public:
  /// Constructor
  LoadHFIRSANS();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadHFIRSANS"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a SANS data file produce by the HFIR instruments at ORNL. "
           "The instrument geometry is also loaded. The center of the detector "
           "is placed at (0,0,D), where D is the sample-to-detector distance.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadSpiceAscii", "LoadSpiceXML2DDet"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Text;SANS\\DataHandling"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

  int getNumberOfMonitors() const { return m_nMonitors; }

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;

  void setInputFileAsHandler();
  void setSansSpiceXmlFormatVersion();
  void setTimes();
  void setWavelength();

  std::pair<int, int> parseDetectorDimensions(const std::string &dims_str);
  std::vector<int> readData(const std::string &dataXpath = "//Data");
  void permuteTubes(std::vector<int> &data);

  void storeValue(int specID, double value, double error, double wavelength, double dwavelength);
  void createWorkspace();
  template <class T> void addRunProperty(const std::string &name, const T &value, const std::string &units = "");
  template <class T> void addRunTimeSeriesProperty(const std::string &name, const T &value);
  void setBeamTrapRunProperty();
  void storeMetaDataIntoWS();
  void runLoadInstrument();
  void rotateDetector();
  void setDetectorDistance();
  void moveDetector();
  std::string getInstrumentStringParameter(const std::string &parameter);
  double getInstrumentDoubleParameter(const std::string &parameter);
  double getSourceToSampleDistance();
  void setBeamDiameter();

  /* constants */
  /// Number of monitors
  static const int m_nMonitors = 2;
  // when parsing the metadata ignore those tags
  const std::vector<std::string> m_tags_to_ignore{"Detector", "DetectorWing"};

  Mantid::DataHandling::XmlHandler m_xmlHandler;
  DataObjects::Workspace2D_sptr m_workspace;
  std::map<std::string, std::string> m_metadata;

  double m_sansSpiceXmlFormatVersion{0.0};
  double m_wavelength;
  double m_dwavelength;
  double m_sampleDetectorDistance;
  Mantid::Types::Core::DateAndTime m_startTime;
  Mantid::Types::Core::DateAndTime m_endTime;
};
} // namespace DataHandling
} // namespace Mantid
