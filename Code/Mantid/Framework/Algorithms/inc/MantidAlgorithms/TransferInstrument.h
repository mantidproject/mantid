#ifndef MANTID_ALGORITHMS_TRANSFERINSTRUMENT_H_
#define MANTID_ALGORITHMS_TRANSFERINSTRUMENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"

#include <climits>

namespace Mantid
{
namespace Algorithms
{
/** Transfers an instrument from one workspace to another workspace with the same base instrument.
    This enables one to manipulate the instrument in one workspace (e.g. calibrate it) and
    then transfer it to another workspace, which can then take advantage of the manipulations
    already done

    Required Properties:
    <UL>
    <LI> GivingWorkspace - The name of the Matrix Workspace containing the instrument to transfer. </LI>
    <LI> RecievingWorkspace - The name of the Matrix workspace for same instrument to receive the tranferred instrument. </LI>
    </UL>

    The instrument in RecievingWorkspace has its parameters replaced by those in GivingWorkspace.

    @author Karl Palmen STFC
    @date 16/08/2012

    Copyright &copy; 2008-2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport TransferInstrument : public API::Algorithm
{
public:
  TransferInstrument();
  virtual ~TransferInstrument();
  /// Algorithm's name
  virtual const std::string name() const { return "TransferInstrument"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return ""; } // TODO decide upon sensible category

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  ///Execution code
  void exec();

  void checkProperties();

  /// The giving workspace
  API::MatrixWorkspace_sptr m_givingWorkspace;
  /// The receiving workspace
  API::MatrixWorkspace_sptr m_receivingWorkspace;
  /// Constant reference to parameters of instrument in giving workspace
  //const Geometry::ParameterMap& m_givParams;
  /// Mutable reference to parameters of instrument in receiving workspace
  //Geometry::ParameterMap& m_recParams;

  /// Static reference to the logger class
  static Mantid::Kernel::Logger& g_log;

  
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_TRANSFERINSTRUMENT_H_*/
