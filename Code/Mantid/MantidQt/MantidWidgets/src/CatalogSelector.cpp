#include "MantidQtMantidWidgets/CatalogSelector.h"

#include <QDesktopWidget>
#include <QListWidgetItem>

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

    /**
     * Populate the ListWidget with the facilities of the catalogs the user is logged in to.
     */
    void CatalogSelector::populateTable()
    {
      auto session = Mantid::API::CatalogManager::Instance().getActiveSessions();

      for (unsigned row = 0; row < session.size(); ++row)
      {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(session.at(row)->getFacility()));
        // Set sessionID to user specific meta-data to easily obtain it later.
        item->setData(Qt::UserRole,QVariant(QString::fromStdString(session.at(row)->getSessionId())));
        m_uiForm.selectedCatalogs->insertItem(row,item);
      }
    }

  } // namespace MantidWidgets
} // namespace MantidQt
