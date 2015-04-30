#ifndef MANTID_DATAHANDLING_LOADSPICEXML2DDET_H_
#define MANTID_DATAHANDLING_LOADSPICEXML2DDET_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace DataHandling {

class SpiceXMLNode {
public:
  SpiceXMLNode(const std::string &nodename);
  ~SpiceXMLNode();

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
  virtual ~LoadSpiceXML2DDet();

  /// Algoriothm name
  virtual const std::string name() const;

  /// Algorithm version
  virtual int version() const;

  /// Category
  virtual const std::string category() const;

  /// Summary
  virtual const std::string summary() const;

private:
  void init();
  void exec();

  /// Parse SPICE XML file
  void parseSpiceXML(const std::string &xmlfilename,
                     std::vector<SpiceXMLNode> &vecspicenode);

  /// Create output MatrixWorkspace
  API::MatrixWorkspace_sptr
  createMatrixWorkspace(const std::vector<SpiceXMLNode> &vecxmlnode,
                        const size_t &numpixelx, const size_t &numpixely,
                        const std::string &detnodename);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADSPICEXML2DDET_H_ */
