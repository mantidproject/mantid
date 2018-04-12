#ifndef MANTID_ALGORITHMS_PEAKINTEGRATION_H_
#define MANTID_ALGORITHMS_PEAKINTEGRATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"

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
class DLLExport PeakIntegration : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "PeakIntegration"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"IntegratePeakTimeSlices", "CentroidPeaks"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Crystal\\Integration"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Integrate single crystal peaks using IkedaCarpenter fit TOF";
  }

private:
  API::MatrixWorkspace_sptr inputW;  ///< A pointer to the input workspace
  API::MatrixWorkspace_sptr outputW; ///< A pointer to the output workspace
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  /// Call Gaussian as a Child Algorithm to fit the peak in a spectrum
  void fitSpectra(const int s, double TOFPeakd, double &I, double &sigI);
  /// Read in all the input parameters
  void retrieveProperties();
  int fitneighbours(int ipeak, std::string det_name, int x0, int y0, int idet,
                    double qspan, DataObjects::PeaksWorkspace_sptr &Peaks,
                    const detid2index_map &pixel_to_wi);

  bool m_IC = false; ///< Ikeida Carpenter fit of TOF
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_PEAKINTEGRATION_H_*/
