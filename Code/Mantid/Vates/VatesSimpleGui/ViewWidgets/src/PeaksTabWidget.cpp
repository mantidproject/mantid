#include "MantidVatesSimpleGuiViewWidgets/PeaksTabWidget.h"

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidQtSliceViewer/QPeaksTableModel.h"
#include "MantidVatesSimpleGuiViewWidgets/PeaksWidget.h"

#include <QWidget>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <vector>
#include <string>
#include <map>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
/**
Constructor

@param ws : Peaks Workspace (MODEL)
@param coordinateSystem : Name of coordinate system used
@param parent : parent widget
*/
PeaksTabWidget::PeaksTabWidget(std::vector<Mantid::API::IPeaksWorkspace_sptr> ws, const std::string &coordinateSystem, QWidget *parent)  : QWidget(parent), m_ws(ws), m_coordinateSystem(coordinateSystem){
  ui.setupUi(this);
}

/// Destructor 
PeaksTabWidget::~PeaksTabWidget(){
}

/**
 * Setup the Table model 
 * @param visiblePeaks : A vector of lists of visible peaks for each peak workspace
 */
void PeaksTabWidget::setupMvc(std::map<std::string, std::vector<bool>> visiblePeaks) {
  for (std::vector<Mantid::API::IPeaksWorkspace_sptr>::iterator it = m_ws.begin(); it != m_ws.end(); ++it) {
    // Create new tab
    std::string name((*it)->getName().c_str());

    // Get visible peaks
    if (visiblePeaks.count((*it)->getName()) > 0) {
      addNewTab(*it, name, visiblePeaks[(*it)->getName()]);
    }
  }
}

void PeaksTabWidget::addNewTab(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, std::string tabName, std::vector<bool> visiblePeaks) {
  PeaksWidget* widget = new PeaksWidget(peaksWorkspace, m_coordinateSystem, this);
  widget->setupMvc(visiblePeaks);
  
  // Connect to the output of the widget
  QObject::connect(widget, SIGNAL(zoomToPeak(Mantid::API::IPeaksWorkspace_sptr, int)),
                   this, SLOT(onZoomToPeak(Mantid::API::IPeaksWorkspace_sptr, int)));

  // Add as a new tab
  this->ui.tabWidget->addTab(widget, QString(tabName.c_str()));
}

/**
 * Zoom to the peak of interest
 * @param ws The workspace pointer.
 * @param row The row in the table.
 */
void PeaksTabWidget::onZoomToPeak(Mantid::API::IPeaksWorkspace_sptr ws, int row){
  emit zoomToPeak(ws, row);
}

/**
 * Update the models and remove the model if it is not required anymore.
 * @param visiblePeaks A map with visible peaks for each workspace.
 */
void PeaksTabWidget::updateTabs(std::map<std::string, std::vector<bool>> visiblePeaks) {
  // Iterate over all tabs
  for (int i = 0; i < this->ui.tabWidget->count(); i++) {
    QString label = this->ui.tabWidget->label(i);
    
    // Check if the peaks workspace still exists, if it does update, else delete the tab.
    if (visiblePeaks.count(label.toStdString()) > 0) {
      updateTab(visiblePeaks[label.toStdString()], i);
    }
    else
    {
      this->ui.tabWidget->removeTab(i);
    }
  }
}

/**
 * Update the tab 
 * @param visbiblePeaks Vector which determines which peaks are visible.
 * @param index The tab index.
 */
void PeaksTabWidget::updateTab(std::vector<bool> visiblePeaks, int index) {
   PeaksWidget* widget = qobject_cast<PeaksWidget*>(this->ui.tabWidget->widget(index));
   widget->updateModel(visiblePeaks);
}

/**
 * Add a new tabs widget
 * @param peaksWorkspace A pointer to a peaksWorkspace
 */
void PeaksTabWidget::addNewPeaksWorkspace(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace, std::vector<bool> visiblePeaks) {
  m_ws.push_back(peaksWorkspace);     
  addNewTab(peaksWorkspace, peaksWorkspace->getName(), visiblePeaks);
}
}
} // namespace
}