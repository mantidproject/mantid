#ifndef MANTID_CRYSTAL_LoadIsawSpectrum_H_
#define MANTID_CRYSTAL_LoadIsawSpectrum_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {

/**
Load incident spectrum and detector efficiency correction file.

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class DLLExport LoadIsawSpectrum : public API::Algorithm {
public:
  LoadIsawSpectrum();
  ~LoadIsawSpectrum();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "LoadIsawSpectrum"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Load incident spectrum and detector efficiency correction file.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Crystal;DataHandling\\Text";
  }

private:
  /// Initialise the properties
  void init();
  /// Run the algorithm
  void exec();

  double spectrumCalc(double TOF, int iSpec,
                      std::vector<std::vector<double>> time,
                      std::vector<std::vector<double>> spectra, size_t id);
  void getInstrument3WaysInit(Algorithm *alg);
  Geometry::Instrument_const_sptr getInstrument3Ways(Algorithm *alg);
};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_LoadIsawSpectrum_H_ */
