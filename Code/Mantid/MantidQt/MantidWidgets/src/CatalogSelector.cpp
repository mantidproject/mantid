#include "MantidQtMantidWidgets/CatalogSelector.h"
#include "MantidAPI/CatalogManager.h"

#include <QDesktopWidget>

namespace MantidQt
{
  namespace MantidWidgets
  {
    /**
     * Constructor
     */
    CatalogSelector::CatalogSelector(QWidget* parent) : QWidget(parent), m_uiForm()
    {
      initLayout();
    }

    /**
     * Obtain the session information for the facilities selected.
     * @return A vector holding the sessions ids of the selected facilities to search.
     */
    std::vector<std::string> CatalogSelector::getSelectedCatalogSessions()
    {
      QModelIndexList indexes = m_uiForm.selectedCatalogs->selectionModel()->selectedRows();

      std::vector<std::string> selectedSessions;
      for (int i = 0; i < indexes.count(); ++i)
      {
        selectedSessions.push_back(m_uiForm.selectedCatalogs->item(i)->data(Qt::UserRole).toString().toStdString());
      }
      return selectedSessions;
    }


    /**
     * Populate the ListWidget with the facilities of the catalogs the user is logged in to.
     */
    void CatalogSelector::populateFacilitySelection()
    {
      auto session = Mantid::API::CatalogManager::Instance().getActiveSessions();

      for (unsigned row = 0; row < session.size(); ++row)
      {
        // This prevents the same items being appended (again) to the list widget.
        if (!m_uiForm.selectedCatalogs->item(row))
        {
          QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(session.at(row)->getFacility()));
          // Set sessionID to user specific meta-data to easily obtain it later.
          item->setData(Qt::UserRole,QVariant(QString::fromStdString(session.at(row)->getSessionId())));
          item->setCheckState(Qt::Unchecked);
          m_uiForm.selectedCatalogs->insertItem(row,item);
        }
      }
      // Select first facility by default (as the user must be logged into at least one catalog).
      m_uiForm.selectedCatalogs->item(0)->setSelected(true);
      m_uiForm.selectedCatalogs->item(0)->setCheckState(Qt::Checked);
      // Set the list widget as focus to better show the selected facilities.
      m_uiForm.selectedCatalogs->setFocus();
    }

    /**
     * Initialise the  default layout.
     */
    void CatalogSelector::initLayout()
    {
      m_uiForm.setupUi(this);

      connect(m_uiForm.updateBtn,SIGNAL(clicked()),this,SLOT(close()));
      connect(m_uiForm.cancelBtn,SIGNAL(clicked()),this,SLOT(close()));

      // Check/un-check the checkbox when an item is clicked or selected.
      connect(m_uiForm.selectedCatalogs,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(checkSelectedFacility(QListWidgetItem*)));

      // Centre the GUI on screen.
      this->setGeometry(QStyle::alignedRect(Qt::LeftToRight,Qt::AlignCenter,
          this->window()->size(),QDesktopWidget().availableGeometry()));
    }

    /**
     * SLOT: Checks the checkbox of the list item selected.
     */
    void CatalogSelector::checkSelectedFacility(QListWidgetItem* item)
    {
      if (item->isSelected()) item->setCheckState(Qt::Checked);
      if (!item->isSelected()) item->setCheckState(Qt::Unchecked);
    }

  } // namespace MantidWidgets
} // namespace MantidQt
