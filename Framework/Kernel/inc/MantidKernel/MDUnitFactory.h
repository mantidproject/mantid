#ifndef MANTID_KERNEL_MDUNITFACTORY_H_
#define MANTID_KERNEL_MDUNITFACTORY_H_

#include "MantidKernel/ChainableFactory.h"
#include "MantidKernel/System.h"
#include "MantidKernel/MDUnit.h"
#include "MantidKernel/DllConfig.h"
#include <memory>

namespace Mantid {
namespace Kernel {

/** MDUnitFactory : Abstract type. Factory method with chain of reponsibility
  succession for creating MDUnits.

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_KERNEL_DLL MDUnitFactory
    : public ChainableFactory<MDUnitFactory, MDUnit, std::string> {

private:
  /// Create the product
  MDUnit *createRaw(const std::string &unitString) const override = 0;

  /// Indicate an ability to intepret the string
  bool canInterpret(const std::string &unitString) const override = 0;
};

//-----------------------------------------------------------------------
// Derived MDUnitFactory declarations
//-----------------------------------------------------------------------

class MANTID_KERNEL_DLL LabelUnitFactory : public MDUnitFactory {
  LabelUnit *createRaw(const std::string &unitString) const override;
  bool canInterpret(const std::string &unitString) const override;
};

class MANTID_KERNEL_DLL InverseAngstromsUnitFactory : public MDUnitFactory {
  InverseAngstromsUnit *createRaw(const std::string &unitString) const override;
  bool canInterpret(const std::string &unitString) const override;
};

class MANTID_KERNEL_DLL ReciprocalLatticeUnitFactory : public MDUnitFactory {
  ReciprocalLatticeUnit *
  createRaw(const std::string &unitString) const override;
  bool canInterpret(const std::string &unitString) const override;
};

using MDUnitFactory_uptr = std::unique_ptr<MDUnitFactory>;

using MDUnitFactory_const_uptr = std::unique_ptr<const MDUnitFactory>;

/// Convience method. Pre-constructed builder chain.
MDUnitFactory_uptr MANTID_KERNEL_DLL makeMDUnitFactoryChain();

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_MDUNITFACTORY_H_ */
