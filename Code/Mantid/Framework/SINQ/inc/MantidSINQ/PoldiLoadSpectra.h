#ifndef MANTID_DATAHANDLING_PoldiLoadChopperSlits_H_
#define MANTID_DATAHANDLING_PoldiLoadChopperSlits_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidSINQ/DllConfig.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/ITableWorkspace.h"

#include <napi.h>
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

namespace Mantid {
namespace DataHandling {
/**
 * Original contributor: Christophe Le Bourlot, Paul Scherrer Institut
 *
 * Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 * This file is part of Mantid.

 * Mantid is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mantid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.

 * File change history is stored at: <https://github.com/mantidproject/mantid>
 * Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_SINQ_DLL PoldiLoadSpectra : public API::Algorithm {
public:
  /// Default constructor
  PoldiLoadSpectra(){};
  /// Destructor
  virtual ~PoldiLoadSpectra() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "PoldiLoadSpectra"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const { return "Load Poldi data file."; }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "SINQ\\Poldi\\Obsolete"; }

protected:
  /// Overwrites Algorithm method
  void exec();

private:
  /// Overwrites Algorithm method.
  void init();
};

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LoadPoldiNexus_H_*/
