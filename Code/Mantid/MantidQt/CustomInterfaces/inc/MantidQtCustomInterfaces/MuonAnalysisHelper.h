#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISHELPER_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISHELPER_H_

#include "MantidKernel/System.h"

#include <QSettings>
#include <QVector>
#include <QDoubleValidator>
#include <QLineEdit>

namespace MantidQt
{
namespace CustomInterfaces
{
namespace MuonAnalysisHelper
{

/// Sets double validator for specified field
DLLExport void setDoubleValidator(QLineEdit* field);

class DLLExport WidgetAutoSaver : QObject
{

public:
  WidgetAutoSaver(const QString& groupName);

  void registerWidget(QWidget* widget, const QString& name, QVariant defaultValue);

  void beginGroup(const QString& name);
  void endGroup();

private slots:
  void saveWidgetValue();

private:
  void connectWidget(QWidget* widget);

  QVector<QWidget*> registeredWidgets;

  QMap<QWidget*, QString> widgetNames;
  QMap<QWidget*, QVariant> widgetDefaultValues;
  QMap<QWidget*, QString> widgetGroups;

  QSettings m_settings;
};

} // namespace MuonAnalysisHelper
} // namespace CustomInterfaces
} // namespace Mantid

#endif  /* MANTID_CUSTOMINTERFACES_MUONANALYSISHELPER_H_ */
