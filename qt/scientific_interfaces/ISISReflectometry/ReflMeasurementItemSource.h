// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_REFLMEASUREMENTITEMSOURCE_H_
#define MANTIDQT_CUSTOMINTERFACES_REFLMEASUREMENTITEMSOURCE_H_

#include "DllConfig.h"
#include "MeasurementItem.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** ReflMeasurementSource : Repository pattern abstracting data mapping from
 domain. Specifically for accessing
 * measurement information from some data map/repository.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflMeasurementItemSource {
public:
  /// Get the measurement somehow using location, or fuzzy path
  virtual MeasurementItem obtain(const std::string &definedPath,
                                 const std::string &fuzzyName) const = 0;
  /// Virtual destructor
  virtual ReflMeasurementItemSource *clone() const = 0;
  /// Destructor
  virtual ~ReflMeasurementItemSource(){};
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_REFLMEASUREMENTITEMSOURCE_H_ */
