#ifndef MANTID_DATAHANDLING_SAVESPE_H_
#define MANTID_DATAHANDLING_SAVESPE_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/cow_ptr.h"
#include <vector>

namespace Mantid {
namespace HistogramData {
class HistogramY;
class HistogramE;
} // namespace HistogramData
namespace DataHandling {
/**
     Saves a workspace into an ASCII SPE file.

     Required properties:
     <UL>
     <LI> InputWorkspace - The workspace name to save. </LI>
     <LI> Filename - The filename for output </LI>
     </UL>

     @author Stuart Campbell, NScD, Oak Ridge National Laboratory
     @date 08/09/2009

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
class DLLExport SaveSPE : public API::Algorithm {
public:
  /// Constructor
  SaveSPE();
  /// Algorithm's name
  const std::string name() const override { return "SaveSPE"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Writes a workspace into a file the spe format.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadSPE", "SavePAR", "SavePHX"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\SPE;Inelastic\\DataHandling";
  }

  /// the mask flag (=-1e30) from the SPE specification
  /// http://www.mantidproject.org/images/3/3d/Spe_file_format.pdf
  static const double MASK_FLAG;

private:
  /// Initialization code
  void init() override;
  /// Execution code
  void exec() override;

  void writeSPEFile(FILE *outSPEFile,
                    const API::MatrixWorkspace_const_sptr &inputWS);
  void writeHists(const API::MatrixWorkspace_const_sptr WS,
                  FILE *const outFile);
  void writeHist(const API::MatrixWorkspace_const_sptr WS, FILE *const outFile,
                 const int wsIn) const;
  void writeMaskFlags(FILE *const outFile) const;
  void writeBins(const std::vector<double> &Vs, FILE *const outFile) const;
  void writeValue(const double value, FILE *const outFile) const;
  void logMissingMasked(const std::vector<int> &inds, const size_t nonMasked,
                        const int masked) const;

  /// the SPE files have a constant number of numbers written on each line, but
  /// depending on the number of bins there will be some "spare" numbers at the
  /// end of the block, this holds that number of spares
  int m_remainder;
  /// the number of bins in each histogram, as the histogram must have common
  /// bins this shouldn't change
  size_t m_nBins;

  /// the error value (=0.0) for spectra whose detectors are all masked, from
  /// the SPE specification
  /// http://www.mantidproject.org/images/3/3d/Spe_file_format.pdf
  static const double MASK_ERROR;

  // temporary variable to keep verified signal values for current spectra
  mutable std::vector<double> m_tSignal;
  // temporary variable to keep verified error values for current spectra
  mutable std::vector<double> m_tError;

  // method verifies if a spectra contains any NaN or Inf values and replaces
  // these values with SPE-specified constants
  void check_and_copy_spectra(const HistogramData::HistogramY &inSignal,
                              const HistogramData::HistogramE &inErr,
                              std::vector<double> &Signal,
                              std::vector<double> &Error) const;
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_SAVESPE_H_
