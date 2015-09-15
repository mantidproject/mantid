#ifndef MANTID_ALGORITHMS_GETALLEI_H_
#define MANTID_ALGORITHMS_GETALLEI_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"


namespace Mantid {

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
  virtual ~GetAllEi(){};

  /// Algorithms name for identification. @see Algorithm::name
  virtual const std::string name() const { return "GetAllEi"; };
  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  virtual const std::string summary() const{
    return "Analyze the chopper logs and estimate all incident energies\n"
           "registered by given monitor of a direct inelastic instrument\n"
           "and available in given experiment.";
  }
  /// Algorithm's version for identification. @see Algorithm::version
  virtual int version() const{ return 1; } ;
  /// Algorithm's category for identification. @see Algorithm::category
  virtual const std::string category()const { return "Direct\\Inelastic"; };
  /// Cross-check properties with each other @see IAlgorithm::validateInputs
  virtual std::map<std::string, std::string> validateInputs();
private:
  // Implement abstract Algorithm methods
  void init();
  void exec();
protected: // for testing, private otherwise.
  // prepare working workspace with appropriate monitor spectra for fitting 
 API::MatrixWorkspace_sptr 
    GetAllEi::buildWorkspaceToFit(const API::MatrixWorkspace_sptr &inputWS,
    size_t &wsIndex0);

   /**Return average time series log value for the appropriately filtered log*/
   double getAvrgLogValue(const API::MatrixWorkspace_sptr &inputWS, const std::string &propertyName,
          std::vector<Kernel::SplittingInterval> &splitter);
  /**process logs and retrieve chopper speed and chopper delay*/
  void findChopSpeedAndDelay(const API::MatrixWorkspace_sptr &inputWS,
       double &chop_speed,double &chop_delay);
  void findGuessOpeningTimes(const std::pair<double,double> &TOF_range,
      double ChopDelay,double Period,std::vector<double > & guess_opening_times);

  // if log, which identifies that instrument is running is available on workspace.
  // The log should be positive when instrument is running and negative otherwise.
  bool m_useFilterLog; 

};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_GETALLEI_H_ */
