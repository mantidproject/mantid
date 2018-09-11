#ifndef MANTID_ALGORITHMS_BINPDEVENTS2D_H_
#define MANTID_ALGORITHMS_BINPDEVENTS2D_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace Algorithms {

/** Bin2DPowderDiffraction :

  This algorithms performs binning of TOF powder diffraction event data
  in 2D d-Spacing (d, d_perp) as described in
  J. Appl. Cryst. (2015) 48, 1627-1636 and
  J. Appl. Cryst. (2017) 50, 866-875

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_ALGORITHMS_DLL Bin2DPowderDiffraction : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"Rebin2D"};
  }
  const std::string category() const override;
  const std::string summary() const override;
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;

protected:
  boost::shared_ptr<API::Progress> m_progress;

private:
  void init() override;
  void exec() override;
  /// Setup the output workspace
  API::MatrixWorkspace_sptr createOutputWorkspace();
  void ReadBinsFromFile(std::vector<double> &Ybins,
                        std::vector<std::vector<double>> &Xbins) const;
  size_t UnifyXBins(std::vector<std::vector<double>> &Xbins) const;

  DataObjects::EventWorkspace_sptr
      m_inputWS;         ///< Pointer to the input event workspace
  int m_numberOfSpectra; ///< The number of spectra in the workspace
  void normalizeToBinArea(API::MatrixWorkspace_sptr outWS);
};

double calcD(double wavelength, double sintheta);
double calcDPerp(double wavelength, double logcostheta);

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_BINPDEVENTS2D_H_ */
