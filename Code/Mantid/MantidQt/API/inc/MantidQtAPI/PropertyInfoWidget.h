#ifndef MANTID_API_PROPERTYINFOWIDGET_H
#define MANTID_API_PROPERTYINFOWIDGET_H

#include <QFrame>
#include <QMap>
#include <QLabel>

namespace MantidQt
{
namespace API
{
  /**
   * A widget used in dialogs to display various information about a property.
   */
  class PropertyInfoWidget : public QFrame
  {
    Q_OBJECT;

  public:
    /// Enumerating the info that can be displayed by this widget.
    enum Info { INVALID, REPLACE, RESTORE };

    /// Constructor.
    PropertyInfoWidget(QWidget *parent = 0);

    /// Set the given icon's visibility.
    void setInfoVisible( Info info, bool visible );
    /// Set the given icon's tool tip.
    void setInfoToolTip( Info info, const QString & toolTip );

  private:
    /// Map enum to labels.
    QMap<Info, QLabel *> m_labels;
  };
}
}

#endif /* MANTID_API_PROPERTYINFOWIDGET_H */
