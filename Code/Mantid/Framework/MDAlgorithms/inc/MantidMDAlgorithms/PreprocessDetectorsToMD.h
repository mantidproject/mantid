#ifndef MANTID_MDALGORITHMS_PREPROCESS_DETECTORS2MD_H
#define MANTID_MDALGORITHMS_PREPROCESS_DETECTORS2MD_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace MDAlgorithms {
/**
This is helper algorithm used to preprocess detector's positions namely
to perform generic part of the transformation from a matrix workspace of a real
instrument to
physical MD workspace of an experimental results (e.g Q-space).

Copyright &copy; 13/09/2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport PreprocessDetectorsToMD : public API::Algorithm {
public:
  PreprocessDetectorsToMD();

  /// Algorithm's name for identification
  virtual const std::string name() const { return "PreprocessDetectorsToMD"; };
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Its a helper algorithm, used to make common part of transformation "
           "from real to reciprocal space. "
           "It is used by :ref:`algm-ConvertToMD` and "
           ":ref:`algm-ConvertToMDMinMaxLocal` algorithms to save time spent "
           "on this transformation "
           "when the algorithm used multiple times for multiple measurements "
           "on the same instrument.";
  }

  /// Algorithm's version for identification
  virtual int version() const { return 1; };
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MDAlgorithms"; }

private:
  void init();
  void exec();
  /// the variable specifies if one needs to calculate efixed for detectors
  /// (make sense for indirect instruments)
  bool m_getEFixed;
  /// the variable specifies if one needs to return the state of detector mask
  /// e.g if the detector is masked
  bool m_getIsMasked;

protected: // for testing
  void processDetectorsPositions(const API::MatrixWorkspace_const_sptr &inputWS,
                                 DataObjects::TableWorkspace_sptr &targWS);
  void
  buildFakeDetectorsPositions(const API::MatrixWorkspace_const_sptr &inputWS,
                              DataObjects::TableWorkspace_sptr &targWS);
  void updateMasksState(const API::MatrixWorkspace_const_sptr &inputWS,
                        DataObjects::TableWorkspace_sptr &targWS);
  // build a table workspace corresponding to the input matrix workspace
  boost::shared_ptr<DataObjects::TableWorkspace>
  createTableWorkspace(const API::MatrixWorkspace_const_sptr &inputWS);
  bool isDetInfoLost(Mantid::API::MatrixWorkspace_const_sptr inWS2D) const;
  // helper function to get efixed if it is there or not;
  double getEi(const API::MatrixWorkspace_const_sptr &inputWS) const;
};

} // MDEvents
} // Mantid
#endif