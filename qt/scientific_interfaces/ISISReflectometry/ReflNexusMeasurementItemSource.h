// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_REFLNEXUSMEASUREMENTITEMSOURCE_H_
#define MANTIDQT_CUSTOMINTERFACES_REFLNEXUSMEASUREMENTITEMSOURCE_H_

#include "DllConfig.h"
#include "MeasurementItem.h"
#include "ReflMeasurementItemSource.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** ReflNexusMeasurementSource : ReflMeasurementSource repository realization
  that
  fetches data out off disk using load algorithms and Nexus formats.
*/
class MANTIDQT_ISISREFLECTOMETRY_DLL ReflNexusMeasurementItemSource
    : public ReflMeasurementItemSource {
public:
  ReflNexusMeasurementItemSource();
  MeasurementItem obtain(const std::string &definedPath,
                         const std::string &fuzzyName) const override;
  /// Virtual destructor
  ReflNexusMeasurementItemSource *clone() const override;
  ~ReflNexusMeasurementItemSource() override;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_REFLNEXUSMEASUREMENTSOURCE_H_ */
