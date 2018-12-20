#ifndef MANTID_DATAHANDLING_LOADSPICEXML2DDET_H_
#define MANTID_DATAHANDLING_LOADSPICEXML2DDET_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataHandling {

class SpiceXMLNode {
public:
  SpiceXMLNode(const std::string &nodename);
  void setParameters(const std::string &nodetype, const std::string &nodeunit,
                     const std::string &nodedescription);
  void setValue(const std::string &strvalue);

  bool hasUnit() const;
  bool hasValue() const;

  bool isString() const;
  bool isInteger() const;
  bool isDouble() const;

  const std::string getName() const;
  const std::string getUnit() const;
  const std::string getDescription() const;
  const std::string getValue() const;

  std::string m_name;
  std::string m_value;
  std::string m_unit;
  char m_typechar;
  std::string m_typefullname;
  std::string m_description;
};

/** LoadSpiceXML2DDet : Load 2D detector data in XML format form SPICE

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport LoadSpiceXML2DDet : public API::Algorithm {
public:
  LoadSpiceXML2DDet();
  ~LoadSpiceXML2DDet() override;

  /// Algoriothm name
  const std::string name() const override;

  /// Algorithm version
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"LoadSpice2D"};
  }

  /// Category
  const std::string category() const override;

  /// Summary
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  /// Process inputs
  void processInputs();

  /// Parse SPICE XML file
  std::vector<SpiceXMLNode> parseSpiceXML(const std::string &xmlfilename);

  /// Create output MatrixWorkspace
  API::MatrixWorkspace_sptr
  createMatrixWorkspace(const std::vector<SpiceXMLNode> &vecxmlnode,
                        const size_t &numpixelx, const size_t &numpixely,
                        const std::string &detnodename,
                        const bool &loadinstrument);

  /// Create output MatrixWorkspace
  API::MatrixWorkspace_sptr
  createMatrixWorkspaceVersion2(const std::vector<SpiceXMLNode> &vecxmlnode,
                                const std::string &detnodename,
                                const bool &loadinstrument);

  API::MatrixWorkspace_sptr parseDetectorNode(const std::string &detvaluestr,
                                              bool loadinstrument,
                                              double &max_counts);

  /// Set up sample logs from table workspace loaded where SPICE data file is
  /// loaded
  void setupSampleLogFromSpiceTable(API::MatrixWorkspace_sptr matrixws,
                                    API::ITableWorkspace_sptr spicetablews,
                                    int ptnumber);

  /// Set up sample logs in the output workspace
  bool setupSampleLogs(API::MatrixWorkspace_sptr outws);

  /// Load instrument
  void loadInstrument(API::MatrixWorkspace_sptr matrixws,
                      const std::string &idffilename);

  /// Get wavelength from workspace
  bool getHB3AWavelength(API::MatrixWorkspace_sptr dataws, double &wavelength);

  /// Set output workspace's X-axs as lab-frame Q space
  void setXtoLabQ(API::MatrixWorkspace_sptr dataws, const double &wavelength);

  /// SPICE detector XML file
  std::string m_detXMLFileName;
  /// XML node name in detector counts file
  std::string m_detXMLNodeName;
  /// Pixel size at X direction
  size_t m_numPixelX;
  /// Pixel size at Y direction
  size_t m_numPixelY;
  /// Flag to show whether instrument is required to load
  bool m_loadInstrument;
  /// shift distance from sample to detector center
  double m_detSampleDistanceShift;
  /// Flag to show whether the SPICE scan table workspace is given
  bool m_hasScanTable;
  /// Pt number for the sample logs to load with presense of Spice scan table
  /// workspace
  int m_ptNumber4Log;
  /// IDF file name to override Mantid's
  std::string m_idfFileName;
  /// User specified wave length
  double m_userSpecifiedWaveLength;
  /// shift of detector on X and Y direction
  double m_detXShift;
  double m_detYShift;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADSPICEXML2DDET_H_ */
