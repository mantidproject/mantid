#ifndef MANTIDQT_API_SCROLLABLEDIALOG_H_
#define MANTIDQT_API_SCROLLABLEDIALOG_H_

//----------------------------------
// Includes
//----------------------------------
#include "WidgetScrollbarFeature.h"
#include <QDialog>

namespace MantidQt {
namespace API {

/**
 * @brief The ScrollableDialog class
 */
class ScrollableDialog : public QDialog {
  Q_OBJECT

public:
  explicit ScrollableDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);
  virtual ~ScrollableDialog();

  /// Check whether this dialog is currently scrollable
  bool scrollable() const;

  /// Enable or disable scrollable behaviour
  void setScrollable(bool enable);

private:
  WidgetScrollbarFeature m_scrollbars;
};

} // namespace API
} // namespace MantidQt

#endif // MANTIDQT_API_SCROLLABLEDIALOG_H_
