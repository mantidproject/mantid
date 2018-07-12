#ifndef MANTID_ALGORITHMS_COPYINSTRUMENTPARAMETERS_H_
#define MANTID_ALGORITHMS_COPYINSTRUMENTPARAMETERS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/Instrument.h"

#include <climits>

namespace Mantid {
namespace Algorithms {
/** Transfers an instrument from one workspace to another workspace with the
   same base instrument.
    This enables one to manipulate the instrument in one workspace (e.g.
   calibrate it) and
    then transfer it to another workspace, which can then take advantage of the
   manipulations
    already done

    Required Properties:
    <UL>
    <LI> GivingWorkspace - The name of the Matrix Workspace containing the
   instrument to transfer. </LI>
    <LI> RecievingWorkspace - The name of the Matrix workspace for same
   instrument to receive the tranferred instrument. </LI>
    </UL>

    The instrument in RecievingWorkspace has its parameters replaced by those in
   GivingWorkspace.

    @author Karl Palmen STFC
    @date 16/08/2012

    Copyright &copy; 2008-2012 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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
class DLLExport CopyInstrumentParameters : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "CopyInstrumentParameters"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Transfers an instrument from on workspace to another workspace "
           "with same base instrument.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"ClearInstrumentParameters"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "DataHandling\\Instrument";
  } // Needs to change
  /// method indicates that base source instrument is the same or different from
  /// base target instrument (mainly used in testing)
  bool isInstrumentDifferent() const { return m_different_instrument_sp; }

protected:
  Parallel::ExecutionMode getParallelExecutionMode(
      const std::map<std::string, Parallel::StorageMode> &storageModes)
      const override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  void checkProperties();

  /// The giving workspace
  API::MatrixWorkspace_sptr m_givingWorkspace;
  /// The receiving workspace
  API::MatrixWorkspace_sptr m_receivingWorkspace;
  /// indicates that source workspace instrument and target workspace instrument
  /// have different share pointers.
  bool m_different_instrument_sp = false;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_COPYINSTRUMENTPARAMETERS_H_ */
