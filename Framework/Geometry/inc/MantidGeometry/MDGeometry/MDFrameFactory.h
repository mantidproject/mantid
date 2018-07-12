#ifndef MANTID_GEOMETRY_MDFRAMEFACTORY_H_
#define MANTID_GEOMETRY_MDFRAMEFACTORY_H_

#include "MantidKernel/ChainableFactory.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidGeometry/MDGeometry/GeneralFrame.h"
#include "MantidGeometry/MDGeometry/QLab.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidGeometry/MDGeometry/UnknownFrame.h"
#include "MantidGeometry/DllConfig.h"
#include <memory>

namespace Mantid {
namespace Geometry {

/// Input argument type for MDFrameFactory chainable factory
class MANTID_GEOMETRY_DLL MDFrameArgument {
public:
  const std::string unitString;
  const std::string frameString;
  MDFrameArgument(const std::string &_frameString,
                  const std::string &_unitString)
      : unitString(_unitString), frameString(_frameString) {}
  MDFrameArgument(const std::string &_frameString)
      : unitString(""), frameString(_frameString) {}
};

/** MDFrameFactory.h : Chain of repsonsibility factory for the MDFrameFactory

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
class MANTID_GEOMETRY_DLL MDFrameFactory
    : public Kernel::ChainableFactory<MDFrameFactory, MDFrame,
                                      MDFrameArgument> {};

/// Helper typedef
using MDFrameFactory_uptr = std::unique_ptr<MDFrameFactory>;

//-----------------------------------------------------------------------
// Derived MDFrameFactory declarations
//-----------------------------------------------------------------------

/// GeneralFrameFactory derived MDFrameFactory type
class MANTID_GEOMETRY_DLL GeneralFrameFactory : public MDFrameFactory {
private:
  GeneralFrame *createRaw(const MDFrameArgument &argument) const override;

public:
  bool canInterpret(const MDFrameArgument &) const override;
};

/// QLabFrameFactory derived MDFrameFactory type
class MANTID_GEOMETRY_DLL QLabFrameFactory : public MDFrameFactory {
private:
  QLab *createRaw(const MDFrameArgument &argument) const override;

public:
  bool canInterpret(const MDFrameArgument &argument) const override;
};

/// QSampleFrameFactory derived MDFrameFactory type
class MANTID_GEOMETRY_DLL QSampleFrameFactory : public MDFrameFactory {
private:
  QSample *createRaw(const MDFrameArgument &argument) const override;

public:
  bool canInterpret(const MDFrameArgument &argument) const override;
};

/// HKLFrame derived MDFrameFactory type
class MANTID_GEOMETRY_DLL HKLFrameFactory : public MDFrameFactory {
private:
  HKL *createRaw(const MDFrameArgument &argument) const override;

public:
  bool canInterpret(const MDFrameArgument &argument) const override;
};

/// Unknown Frame derived MDFrameFactory type
class MANTID_GEOMETRY_DLL UnknownFrameFactory : public MDFrameFactory {
private:
  UnknownFrame *createRaw(const MDFrameArgument &argument) const override;

public:
  bool canInterpret(const MDFrameArgument &argument) const override;
};

/// Make a complete factory chain
MDFrameFactory_uptr MANTID_GEOMETRY_DLL makeMDFrameFactoryChain();

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_MDFRAMEFACTORY_H_ */
