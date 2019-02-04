// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef METADATATOFIELDDATA_H_
#define METADATATOFIELDDATA_H_

#include "MantidKernel/System.h"
#include <functional>
#include <string>
class vtkFieldData;
namespace Mantid {
namespace VATES {

/**
 * Functor converts metadata (in std::string) to vtkFieldData.
 *
 @author Owen Arnold, Tessella plc
 @date 09/02/2011
 */

class DLLExport MetadataToFieldData {
public:
  /// Act as Functor.
  void operator()(vtkFieldData *fieldData, const std::string &metaData,
                  const std::string &id) const;

  /// Explicit call to Functor execution.
  void execute(vtkFieldData *fieldData, const std::string &metaData,
               const std::string &id) const;
};
} // namespace VATES
} // namespace Mantid

#endif
