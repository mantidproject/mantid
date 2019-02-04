// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef IFUNCTONBUILDER_H_
#define IFUNCTONBUILDER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "ImplicitFunctionParameter.h"
#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"
#include <vector>

namespace Mantid {

namespace API {
/** This class is the abstract type for building IImplicitFunctions

@author Owen Arnold, Tessella plc
@date 01/10/2010
*/

class MANTID_API_DLL ImplicitFunctionBuilder {
public:
  virtual Mantid::Geometry::MDImplicitFunction *create() const = 0;
  virtual ~ImplicitFunctionBuilder() = default;
};
} // namespace API
} // namespace Mantid

#endif
