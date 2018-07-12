#ifndef MANTID_ALGORITHMS_ALIGNDETECTORS_H_
#define MANTID_ALGORITHMS_ALIGNDETECTORS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {

namespace DataObjects {
class EventWorkspace;
}

namespace Algorithms {

namespace {
class ConversionFactors;
}

/** Performs a unit change from TOF to dSpacing, correcting the X values to
   account for small
    errors in the detector positions.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the Workspace whose detectors are to be
   aligned </LI>
    <LI> OutputWorkspace - The name of the Workspace in which the result of the
   algorithm will be stored </LI>
    <LI> CalibrationFile - The file containing the detector offsets </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 18/08/2008

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport AlignDetectors : public API::Algorithm {
public:
  AlignDetectors();
  ~AlignDetectors() override;

  /// Algorithms name for identification. @see Algorithm::name
  const std::string name() const override;
  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string summary() const override;

  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"DiffractionFocussing", "AlignAndFocusPowder"};
  }
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override;
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override;

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;

  void align(const ConversionFactors &converter, API::Progress &progress,
             API::MatrixWorkspace &outputWS);
  void align(const ConversionFactors &converter, API::Progress &progress,
             DataObjects::EventWorkspace &outputWS);

  void loadCalFile(API::MatrixWorkspace_sptr inputWS,
                   const std::string &filename);
  void getCalibrationWS(API::MatrixWorkspace_sptr inputWS);

  Mantid::API::ITableWorkspace_sptr m_calibrationWS;

  /// number of spectra in input workspace
  int64_t m_numberOfSpectra;

  /// Map of conversion factors for TOF to d-Spacing conversion
  std::map<detid_t, double> *tofToDmap;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_ALIGNDETECTORS_H_ */
