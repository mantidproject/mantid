// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_MEASUREMENTITEM_H_
#define MANTIDQT_CUSTOMINTERFACES_MEASUREMENTITEM_H_

#include "DllConfig.h"
#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/** Measurement : Immutable measurement type
 */
class MANTIDQT_ISISREFLECTOMETRY_DLL MeasurementItem {

public:
  using IDType = std::string;

  /// Constructor
  MeasurementItem(const IDType &measurementItemId, const IDType &subId,
                  const std::string &label, const std::string &type,
                  const double angle, const std::string &run,
                  const std::string &title);

  /// Constructional method
  static MeasurementItem InvalidMeasurementItem(const std::string &why);

  /// Copy constructor
  MeasurementItem(const MeasurementItem &other);

  /// Destructor
  ~MeasurementItem();

  bool isUseable() const;
  std::string whyUnuseable() const;
  IDType id() const;
  IDType subId() const;
  std::string run() const;
  std::string type() const;
  std::string title() const;
  std::string label() const;
  double angle() const;
  std::string angleStr() const;
  MeasurementItem &operator=(const MeasurementItem & /*other*/);

private:
  /// Constructor
  MeasurementItem(const std::string &why);
  IDType m_measurementItemId;
  IDType m_subId;
  std::string m_label;
  std::string m_type;
  double m_angle;
  std::string m_run;
  std::string m_title;
  std::string m_whyUnuseable;
  /// Not assignable
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQT_CUSTOMINTERFACES_MEASUREMENTITEM_H_ */
