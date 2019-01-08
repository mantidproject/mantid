// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INSTRUMENTWIDGETDECODER_H_
#define INSTRUMENTWIDGETDECODER_H_

#include <QMap>
#include <QList>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW InstrumentWidgetDecoder {
  Q_OBJECT
public:
  InstrumentWidgetDecoder();
  void decoder(const QMap<QString, QVariant> &map, const InstrumentWidget &obj,
               const QString &projectPath);

private:
  QString m_projectPath;
  QString m_workspaceName;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /*INSTRUMENTWIDGETDECODER_H_*/