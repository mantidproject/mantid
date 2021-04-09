// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidSINQ/DllConfig.h"

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
*/

class MANTID_SINQ_DLL PoldiAutoCorrelation5 : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "PoldiAutoCorrelation"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Performs correlation analysis of POLDI 2D-data."; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 5; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "SINQ\\Poldi"; }

protected:
  /// Overwrites Algorithm method
  void exec() override;

  void logConfigurationInformation(const std::shared_ptr<PoldiDeadWireDecorator> &cleanDetector,
                                   const PoldiAbstractChopper_sptr &chopper);

private:
  /// Overwrites Algorithm method.
  void init() override;

  std::shared_ptr<PoldiAutoCorrelationCore> m_core;
};

} // namespace Poldi
} // namespace Mantid
