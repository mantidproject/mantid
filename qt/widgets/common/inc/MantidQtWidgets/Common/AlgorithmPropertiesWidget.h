// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"
#include "MantidQtWidgets/Common/PropertyWidget.h"
#include <QWidget>

class QGridLayout;
class QGroupBox;
class QScrollArea;

namespace Mantid {
namespace Kernel {
class Property;
}
} // namespace Mantid

namespace MantidQt {
namespace API {
class PropertyWidget;

/** Widget that contains dynamically generated
 * PropertyWidget's for each property of an algorithm,
 * contained in a scroll area.

  @date 2012-03-09
*/
class EXPORT_OPT_MANTIDQT_COMMON AlgorithmPropertiesWidget : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QString algorithmName READ getAlgorithmName WRITE setAlgorithmName)

public:
  AlgorithmPropertiesWidget(QWidget *parent = nullptr);
  ~AlgorithmPropertiesWidget() override;

  void setInputHistory(MantidQt::API::AbstractAlgorithmInputHistory *inputHistory);

  void initLayout();

  Mantid::API::IAlgorithm_sptr getAlgorithm();
  void setAlgorithm(const Mantid::API::IAlgorithm_sptr &algo);

  const QString &getAlgorithmName() const;
  void setAlgorithmName(QString name);

  void addEnabledAndDisableLists(const QStringList &enabled, const QStringList &disabled);

  void hideOrDisableProperties(const QString &changedPropName = "");

  void saveInput();

  /// Each dynamically created PropertyWidget
  QHash<QString, PropertyWidget *> m_propWidgets;

  /// Mapping between group and it's dynamically created widget
  QHash<QString, QGroupBox *> m_groupWidgets;

  /// Viewport containing the grid of property widgets
  QWidget *m_viewport;

  /// Scroll area containing the viewport
  QScrollArea *m_scroll;

public slots:
  /// Any property changed
  void propertyChanged(const QString &changedPropName);

  /// Replace WS button was clicked
  void replaceWSClicked(const QString &propName);

private:
  bool isWidgetEnabled(Mantid::Kernel::Property *property, const QString &propName) const;

  /// Chosen algorithm name
  QString m_algoName;

  /// Pointer to the algorithm to view
  Mantid::API::IAlgorithm_sptr m_algo;

  /// The grid widget containing the input boxes
  QGridLayout *m_inputGrid;

  /// The current grid widget for sub-boxes
  QGridLayout *m_currentGrid;

  /// A map where key = property name; value = the error for this property (i.e.
  /// it is not valid).
  QHash<QString, QString> m_errors;

  /// A list of property names that are FORCED to stay enabled.
  QStringList m_enabled;

  /// A list of property names that are FORCED to stay disabled.
  /// e.g. when callid AlgorithmNameDialog()
  QStringList m_disabled;

  /// History of inputs to the algorithm
  MantidQt::API::AbstractAlgorithmInputHistory *m_inputHistory;
};

} // namespace API
} // namespace MantidQt
