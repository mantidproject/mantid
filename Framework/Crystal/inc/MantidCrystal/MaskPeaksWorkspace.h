#ifndef MANTID_DATAHANDLING_MaskPeaksWorkspace_H_
#define MANTID_DATAHANDLING_MaskPeaksWorkspace_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"

namespace Mantid {
namespace Crystal {
/**
 Find the offsets for each detector

 @author Vickie Lynch, SNS, ORNL
 @date 02/08/2011

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
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport MaskPeaksWorkspace : public API::Algorithm {
public:
  /// Default constructor
  MaskPeaksWorkspace();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "MaskPeaksWorkspace"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"CreatePeaksWorkspace"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Crystal\\Peaks"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Masks a peaks workspace.";
  }

private:
  API::MatrixWorkspace_sptr m_inputW; ///< A pointer to the input workspace

  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  std::size_t getWkspIndex(const detid2index_map &pixel_to_wi,
                           Geometry::IComponent_const_sptr comp, const int x,
                           const int y);
  void getTofRange(double &tofMin, double &tofMax, const double tofPeak,
                   const HistogramData::HistogramX &tof);
  int findPixelID(std::string bankName, int col, int row);

  /// Read in all the input parameters
  void retrieveProperties();
  int m_xMin;      ///< The start of the X range for fitting
  int m_xMax;      ///< The end of the X range for fitting
  int m_yMin;      ///< The start of the Y range for fitting
  int m_yMax;      ///< The end of the Y range for fitting
  double m_tofMin; ///< The start of the box around the peak in tof
  double m_tofMax; ///< The end of the box around the peak in tof
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_MaskPeaksWorkspace_H_*/
