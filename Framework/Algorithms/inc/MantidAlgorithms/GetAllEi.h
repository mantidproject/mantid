#ifndef MANTID_ALGORITHMS_GETALLEI_H_
#define MANTID_ALGORITHMS_GETALLEI_H_

#include "MantidKernel/System.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
//#include "MantidAPI/IAlgorithm.h"

namespace Mantid {

namespace Geometry {
class IComponent;
}

namespace Kernel {
class Unit;
}

namespace Algorithms {

/** Estimate all incident energies, used by chopper instrument.

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport GetAllEi : public API::Algorithm {
public:
  GetAllEi();
  /// Algorithms name for identification. @see Algorithm::name
  const std::string name() const override { return "GetAllEi"; };
  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string summary() const override {
    return "Analyze the chopper logs and the signal registered by the monitors "
           "to identify energies used as incident energies in an inelastic "
           "experiment.";
  }
  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"GetEi"}; }
  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override { return "Inelastic\\Ei"; };
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  std::map<std::string, std::string> validateInputs() override;

private:
  // Implement abstract Algorithm methods
  void init() override;
  void exec() override;
  Kernel::Property *getPLogForProperty(const API::MatrixWorkspace_sptr &inputWS,
                                       const std::string &propertyName);
  void setFilterLog(const API::MatrixWorkspace_sptr &inputWS);
  // former lambda function exposed as not evry compiler support this yet
  bool peakGuess(const API::MatrixWorkspace_sptr &inputWS, size_t index,
                 double Ei, const std::vector<size_t> &monsRangeMin,
                 const std::vector<size_t> &monsRangeMax, double &peakPos,
                 double &peakHeight, double &peakTwoSigma);

protected: // for testing, private otherwise.
  // prepare working workspace with appropriate monitor spectra for fitting
  API::MatrixWorkspace_sptr
      // prepare matrix workspace to analyze monitor signal
      buildWorkspaceToFit(const API::MatrixWorkspace_sptr &inputWS,
                          size_t &wsIndex0);

  /**Return average time series log value for the appropriately filtered log*/
  double getAvrgLogValue(const API::MatrixWorkspace_sptr &inputWS,
                         const std::string &propertyName,
                         std::vector<Kernel::SplittingInterval> &splitter);
  /**process logs and retrieve chopper speed and chopper delay*/
  void findChopSpeedAndDelay(const API::MatrixWorkspace_sptr &inputWS,
                             double &chop_speed, double &chop_delay);
  void findGuessOpeningTimes(const std::pair<double, double> &TOF_range,
                             double ChopDelay, double Period,
                             std::vector<double> &guess_opening_times);
  /**Get energy of monitor peak if one is present*/
  bool findMonitorPeak(const API::MatrixWorkspace_sptr &inputWS, double Ei,
                       const std::vector<size_t> &monsRangeMin,
                       const std::vector<size_t> &monsRangeMax,
                       double &position, double &height, double &twoSigma);
  /**Find indexes of each expected peak intervals */
  void findBinRanges(const HistogramData::HistogramX &eBins,
                     const HistogramData::HistogramY &signal,
                     const std::vector<double> &guess_energy,
                     double eResolution, std::vector<size_t> &irangeMin,
                     std::vector<size_t> &irangeMax,
                     std::vector<bool> &guessValid);

  size_t calcDerivativeAndCountZeros(const std::vector<double> &bins,
                                     const std::vector<double> &signal,
                                     std::vector<double> &deriv,
                                     std::vector<double> &zeros);

  /**Auxiliary method to print guess chopper energies in debug mode*/
  void printDebugModeInfo(const std::vector<double> &guess_opening,
                          const std::pair<double, double> &TOF_range,
                          boost::shared_ptr<Kernel::Unit> &destUnit);
  /// if true, take derivate of the filter log to identify interval when
  /// instrument is running.
  bool m_FilterWithDerivative;
  /// maximal relative peak width to consider acceptable. Defined by minimal
  /// instrument resolution
  /// and does not exceed 0.08
  double m_min_Eresolution;
  // set as half max LET resolution at 20mev at 5e-4
  double m_max_Eresolution;
  double m_peakEnergyRatio2reject;
  // the value of constant phase shift on the chopper used to calculate
  // tof at chopper from recorded delay.
  double m_phase;
  // internal pointer to access to chopper
  boost::shared_ptr<const Geometry::IComponent> m_chopper;
  // internal pointer to access log, used for filtering
  Kernel::TimeSeriesProperty<double> *m_pFilterLog;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_GETALLEI_H_ */
