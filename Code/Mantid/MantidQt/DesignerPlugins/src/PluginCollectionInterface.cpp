//-------------------------------------------------------
// Includes
//-------------------------------------------------------
#include "MantidQtDesignerPlugins/PluginCollectionInterface.h"

#include "MantidQtDesignerPlugins/FileFinderPlugin.h"
#include "MantidQtDesignerPlugins/InstrumentSelectorPlugin.h"
#include "MantidQtDesignerPlugins/WorkspaceSelectorPlugin.h"
#include "MantidQtDesignerPlugins/FitBrowserPlugin.h"
#include "MantidQtDesignerPlugins/MuonFitBrowserPlugin.h"


Q_EXPORT_PLUGIN2(LIBRARY_NAME, PluginCollectionInterface)

/**
 * Default constructor
 * @param parent :: The parent widget
 */
PluginCollectionInterface::PluginCollectionInterface(QObject *parent) : QObject(parent)
{
  m_widgets.append(new FileFinderPlugin(this));
  m_widgets.append(new InstrumentSelectorPlugin(this));
  m_widgets.append(new WorkspaceSelectorPlugin(this));
  // for now adding the fit browser to qt-designer is in 
  // development stage - only uncomment once they plugin
  // can be can be dragged in without breaking designer
  m_widgets.append(new FitBrowserPlugin(this));
  m_widgets.append(new MuonFitBrowserPlugin(this));
}

/**
 * Return the custom widgets exported by this library
 * @returns :: a list of custom widget interfaces contained within this library
 */
QList<QDesignerCustomWidgetInterface*> PluginCollectionInterface::customWidgets() const
{
  return m_widgets;
}
