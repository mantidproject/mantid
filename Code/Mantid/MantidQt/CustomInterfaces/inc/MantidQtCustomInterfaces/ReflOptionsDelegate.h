#ifndef MANTID_CUSTOMINTERFACES_REFLOPTIONSDELEGATE_H
#define MANTID_CUSTOMINTERFACES_REFLOPTIONSDELEGATE_H

#include <QStyledItemDelegate>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    class ReflOptionsDelegate : public QStyledItemDelegate
    {
    public:
      ReflOptionsDelegate() {};
      virtual ~ReflOptionsDelegate() {};
      virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
      {
        Q_UNUSED(option);
        Q_UNUSED(index);
        return new QLineEdit(parent);
      }
    };
  }
}

#endif /* MANTID_CUSTOMINTERFACES_REFLOPTIONSDELEGATE_H */
