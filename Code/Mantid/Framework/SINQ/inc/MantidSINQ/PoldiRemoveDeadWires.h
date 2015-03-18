#ifndef MANTID_POLDI_PoldiRemoveDeadWires_H_
#define MANTID_POLDI_PoldiRemoveDeadWires_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

namespace Mantid {

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

namespace Poldi {
// N.B. PoldiRemoveDeadWires is used to detecte and remove dead wire from Poldi
// raw data
/** @class PoldiRemoveDeadWires PoldiRemoveDeadWires.h
   Poldi/PoldiRemoveDeadWires.h

    Part of the Poldi scripts set, used to detecte and remove dead wire from
   Poldi raw data

    @author Christophe Le Bourlot, Paul Scherrer Institut - SINQ
    @date 05/06/2013

    Copyright © 2013 PSI-MSS

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

    File change history is stored at:
   <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at <http://doxygen.mantidproject.org>
*/
class MANTID_SINQ_DLL PoldiRemoveDeadWires : public API::Algorithm {
public:
  /// Default constructor
  PoldiRemoveDeadWires();
  /// Destructor
  virtual ~PoldiRemoveDeadWires() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "PoldiRemoveDeadWires"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Remove dead wires from Poldi data.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "SINQ\\Poldi\\Obsolete"; }

protected:
  /// Overwrites Algorithm method
  void exec();

  /// The name and path of the input file
  std::string m_filename;
  /// Should we remove the declare dead wires?
  bool m_runDeadWires;
  /// Should we auto detecte dead wires?
  bool m_runAutoDetectDW;
  /// threshold for dead wires auto detection
  double m_defautDWThreshold;

  /// The number of spectra in the raw file
  size_t m_numberOfSpectra;
  /// The number of periods in the raw file
  size_t m_channelsPerSpectrum;

private:
  /// Overwrites Algorithm method.
  void init();

  // remove the declared dead wires and set the corresponding intensity to zero
  void runExcludWires3(DataObjects::Workspace2D_sptr &localWorkspace,
                       API::ITableWorkspace_sptr &outputws);
  // auto detecte and remove dead wires by average intensity comparison with
  // neighbors
  void autoRemoveDeadWires(DataObjects::Workspace2D_sptr &localWorkspace,
                           API::ITableWorkspace_sptr &outputws);
};

} // namespace Poldi
} // namespace Mantid

#endif /*MANTID_POLDI_PoldiRemoveDeadWires_H_*/
