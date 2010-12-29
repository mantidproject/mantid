//-------------------------------------------------------
// Includes
//-------------------------------------------------------
#include "MantidQtDesignerPlugins/PluginCollectionInterface.h"

#include "MantidQtDesignerPlugins/FileFinderPlugin.h"
#include "MantidQtDesignerPlugins/InstrumentSelectorPlugin.h"

Q_EXPORT_PLUGIN2(LIBRARY_NAME, PluginCollectionInterface)

/**
 * Default constructor
 * @param parent The parent widget
 */
PluginCollectionInterface::PluginCollectionInterface(QObject *parent) : QObject(parent)
{
  m_widgets.append(new FileFinderPlugin(this));
  m_widgets.append(new InstrumentSelectorPlugin(this));
}

/**
 * Return the custom widgets exported by this library
 * @param Returns a list of custom widget interfaces contained within this library
 */
QList<QDesignerCustomWidgetInterface*> PluginCollectionInterface::customWidgets() const
{
  return m_widgets;
}
