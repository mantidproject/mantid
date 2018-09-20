#ifndef MASKBINSDATA_H_
#define MASKBINSDATA_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <string>
#include <vector>

#include <QList>
#include <QMap>

namespace MantidQt {
namespace MantidWidgets {
/**
Class for storing information on masked bins in a workspace.

Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
*/
class MaskBinsData {
public:
  void addXRange(double start, double end, const std::vector<size_t> &indices);
  void mask(const std::string &wsName) const;
  bool isEmpty() const;
  void subtractIntegratedSpectra(const Mantid::API::MatrixWorkspace &workspace,
                                 std::vector<double> &spectraIntgrs) const;
  void clear();
  /// Load the state of the bin masks from a Mantid project file.
  void loadFromProject(const std::string &lines);
  /// Save the state of the bin masks to a Mantid project file.
  std::string saveToProject() const;

private:
  /// Range of x values to mask in a spectrum. (Using MaskBins)
  struct BinMask {
    BinMask(double s = 0.0, double e = 0.0) : start(s), end(e) {}
    double start;
    double end;
    std::vector<size_t> spectra;
  };
  QList<BinMask> m_masks;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /*MASKBINSDATA_H_*/
