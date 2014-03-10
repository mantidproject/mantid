#include "MantidQtMantidWidgets/CatalogSelector.h"

#include <QDesktopWidget>

namespace MantidQt
{
  namespace MantidWidgets
  {
    CatalogSelector::CatalogSelector(QWidget* parent) : QWidget(parent), m_uiForm()
    {
      initLayout();
    }

    void CatalogSelector::initLayout()
    {
      m_uiForm.setupUi(this);

      // Centre the GUI on screen.
      this->setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,
          this->window()->size(),QDesktopWidget().availableGeometry()));
    }

  } // namespace MantidWidgets
} // namespace MantidQt
