#ifndef PLUGINCOLLECTIONINTERFACE_H_
#define PLUGINCOLLECTIONINTERFACE_H_

#include <QtDesigner>
#include <QtPlugin>

/** 
The PluginCollectionInterface implements the interface for the plugin library and holds a 
list of plugins defined by the library.

@author Martyn Gigg, Tessella plc
@date 10/08/2009

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
*/
class PluginCollectionInterface : public QObject, public QDesignerCustomWidgetCollectionInterface
{
  Q_OBJECT
  Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

public:
  /// Default constructor
  PluginCollectionInterface(QObject *parent = 0);
  /// Returns a list of the custom widgets within this library
  virtual QList<QDesignerCustomWidgetInterface*> customWidgets() const;

private:
  QList<QDesignerCustomWidgetInterface*> m_widgets;
};

#endif