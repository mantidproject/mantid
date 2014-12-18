#ifndef MANTID_ALGORITHMS_MASKBINSFROMTABLE_H_
#define MANTID_ALGORITHMS_MASKBINSFROMTABLE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid {
namespace Algorithms {

/** MaskBinsFromTable : TODO: DESCRIPTION

  @date 2012-06-04

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport MaskBinsFromTable : public API::Algorithm {
public:
  MaskBinsFromTable();
  virtual ~MaskBinsFromTable();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "MaskBinsFromTable"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Mask bins from a table workspace. ";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; };
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Transforms\\Masking"; }

private:
  // Implement abstract Algorithm methods
  void init();
  // Implement abstract Algorithm methods
  void exec();

  /// Process input Mask bin TableWorkspace.
  void processMaskBinWorkspace(DataObjects::TableWorkspace_sptr masktblws,
                               API::MatrixWorkspace_sptr dataws);
  /// Call MaskBins
  void maskBins(API::MatrixWorkspace_sptr dataws);
  /// Convert a list of detector IDs list (string) to a list of
  /// spectra/workspace indexes list
  std::string convertToSpectraList(API::MatrixWorkspace_sptr dataws,
                                   std::string detidliststr);

  /// Column indexes of XMin, XMax, SpectraList, DetectorIDsList
  int id_xmin, id_xmax, id_spec, id_dets;
  bool m_useDetectorID;
  bool m_useSpectrumID;

  /// Vector to store XMin, XMax and SpectraList
  std::vector<double> m_xminVec, m_xmaxVec;
  std::vector<std::string> m_spectraVec;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MASKBINSFROMTABLE_H_ */
