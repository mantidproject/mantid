#ifndef MANTID_DATAHANDLING_PoldiAutoCorrelation5_H_
#define MANTID_DATAHANDLING_PoldiAutoCorrelation5_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"

#include "MantidKernel/PhysicalConstants.h"
#include "MantidSINQ/PoldiUtilities/PoldiAutoCorrelationCore.h"
#include "MantidSINQ/PoldiUtilities/PoldiDeadWireDecorator.h"

namespace Mantid {
namespace Poldi {

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

// N.B. PoldiAutoCorrelation5 is used to create the autocorrelation
// function from POLDI raw data
/** @class PoldiAutoCorrelation5 PoldiAutoCorrelation5.h
   Poldi/PoldiAutoCorrelation5.h

    Part of the Poldi scripts set, used to analyse Poldi data

    @author Christophe Le Bourlot, Paul Scherrer Institut - SINQ
    @date 05/06/2013

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 18/02/2014

    Copyright © 2013, 2014 PSI-MSS

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

class MANTID_SINQ_DLL PoldiAutoCorrelation5 : public API::Algorithm {
public:
  /// Default constructor
  PoldiAutoCorrelation5() {}
  /// Destructor
  virtual ~PoldiAutoCorrelation5() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "PoldiAutoCorrelation"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Performs correlation analysis of POLDI 2D-data.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 5; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "SINQ\\Poldi"; }

protected:
  /// Overwrites Algorithm method
  void exec();

  void logConfigurationInformation(
      boost::shared_ptr<PoldiDeadWireDecorator> cleanDetector,
      PoldiAbstractChopper_sptr chopper);

private:
  /// Overwrites Algorithm method.
  void init();

  boost::shared_ptr<PoldiAutoCorrelationCore> m_core;
};

} // namespace Poldi
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_PoldiAutoCorrelation5_H_*/
