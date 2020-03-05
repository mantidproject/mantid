// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef FIELDDATA_TO_METADATA_H_
#define FIELDDATA_TO_METADATA_H_

#include "MantidKernel/System.h"
#include <functional>
#include <string>

class vtkFieldData;
namespace Mantid {
namespace VATES {

/**
 * Functor Converts fielddata of type vtkFieldData to metadata (std::string).
 *
 @author Owen Arnold, Tessella plc
 @date 09/02/2011
 */

class DLLExport FieldDataToMetadata {
public:
  /// Act as Functor.
  std::string operator()(vtkFieldData *fieldData, const std::string &id) const;

  /// Explicit call to Functor execution.
  std::string execute(vtkFieldData *fieldData, const std::string &id) const;
};
} // namespace VATES
} // namespace Mantid

#endif
