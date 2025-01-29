// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {

class SpiceXMLNode {
public:
  SpiceXMLNode(std::string nodename);
  void setParameters(const std::string &nodetype, const std::string &nodeunit, const std::string &nodedescription);
  void setValue(const std::string &strvalue);

  bool hasUnit() const;
  bool hasValue() const;

  bool isString() const;
  bool isInteger() const;
  bool isDouble() const;

  const std::string &getName() const;
  const std::string &getUnit() const;
  const std::string &getDescription() const;
  const std::string &getValue() const;

  std::string m_name;
  std::string m_value;
  std::string m_unit;
  char m_typechar;
  std::string m_typefullname;
  std::string m_description;
};

/** LoadSpiceXML2DDet : Load 2D detector data in XML format form SPICE
 */
class MANTID_DATAHANDLING_DLL LoadSpiceXML2DDet final : public API::Algorithm {
public:
  LoadSpiceXML2DDet();
  ~LoadSpiceXML2DDet() override;

  /// Algoriothm name
  const std::string name() const override;

  /// Algorithm version
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"LoadSpice2D"}; }

  /// Category
  const std::string category() const override;

  /// Summary
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  /// Process inputs
  void processInputs();

  /// create workspace (good to load instrument) from vector of counts
  API::MatrixWorkspace_sptr createMatrixWorkspace(const std::vector<unsigned int> &vec_counts);
  /// parse binary integer file
  std::vector<unsigned int> binaryParseIntegers(std::string &binary_file_name);

  /// Parse SPICE XML file
  std::vector<SpiceXMLNode> xmlParseSpice(const std::string &xmlfilename);

  /// Create output MatrixWorkspace
  API::MatrixWorkspace_sptr xmlCreateMatrixWorkspaceKnownGeometry(const std::vector<SpiceXMLNode> &vecxmlnode,
                                                                  const size_t &numpixelx, const size_t &numpixely,
                                                                  const std::string &detnodename,
                                                                  const bool &loadinstrument);

  /// Create output MatrixWorkspace
  API::MatrixWorkspace_sptr xmlCreateMatrixWorkspaceUnknowGeometry(const std::vector<SpiceXMLNode> &vecxmlnode,
                                                                   const std::string &detnodename,
                                                                   const bool &loadinstrument);

  API::MatrixWorkspace_sptr xmlParseDetectorNode(const std::string &detvaluestr, bool loadinstrument,
                                                 double &max_counts);

  /// Set up sample logs from table workspace loaded where SPICE data file is
  /// loaded
  void setupSampleLogFromSpiceTable(const API::MatrixWorkspace_sptr &matrixws,
                                    const API::ITableWorkspace_sptr &spicetablews, int ptnumber);

  /// Set up sample logs in the output workspace
  bool setupSampleLogs(const API::MatrixWorkspace_sptr &outws);

  /// Load instrument
  void loadInstrument(const API::MatrixWorkspace_sptr &matrixws, const std::string &idffilename);

  /// Get wavelength from workspace
  bool getHB3AWavelength(const API::MatrixWorkspace_sptr &dataws, double &wavelength);

  /// Set output workspace's X-axs as lab-frame Q space
  void setXtoLabQ(const API::MatrixWorkspace_sptr &dataws, const double &wavelength);

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
