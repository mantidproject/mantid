#ifndef MANTID_ALGORITHMS_RefReduction_H_
#define MANTID_ALGORITHMS_RefReduction_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/**
    Data reduction for reflectometry

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

class DLLExport RefReduction : public API::Algorithm {
public:
  /// (Empty) Constructor
  RefReduction() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~RefReduction() {}
  /// Algorithm's name
  virtual const std::string name() const { return "RefReduction"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Data reduction for reflectometry.";
  }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Workflow\\Reflectometry";
  }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  static const std::string PolStateOffOff;
  static const std::string PolStateOnOff;
  static const std::string PolStateOffOn;
  static const std::string PolStateOnOn;
  static const std::string PolStateNone;

  static const int NX_PIXELS;
  static const int NY_PIXELS;
  static const double PIXEL_SIZE;

  std::string m_output_message;

  API::MatrixWorkspace_sptr processData(const std::string polarization);
  API::MatrixWorkspace_sptr processNormalization();
  API::IEventWorkspace_sptr loadData(const std::string dataRun,
                                     const std::string polarization);
  double calculateAngleREFM(API::MatrixWorkspace_sptr workspace);
  double calculateAngleREFL(API::MatrixWorkspace_sptr workspace);
  API::MatrixWorkspace_sptr subtractBackground(API::MatrixWorkspace_sptr dataWS,
                                               API::MatrixWorkspace_sptr rawWS,
                                               int peakMin, int peakMax,
                                               int bckMin, int bckMax,
                                               int lowResMin, int lowResMax);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_RefReduction_H_*/
