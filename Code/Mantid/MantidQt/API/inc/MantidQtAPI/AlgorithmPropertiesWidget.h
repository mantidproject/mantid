#ifndef MANTID_API_ALGORITHMPROPERTIESWIDGET_H_
#define MANTID_API_ALGORITHMPROPERTIESWIDGET_H_

#include "MantidKernel/System.h"
#include <QtGui/qwidget.h>
#include <qgridlayout.h>
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtAPI/PropertyWidget.h"
#include "MantidAPI/Algorithm.h"
#include <QtGui>


namespace MantidQt
{
namespace API
{


  /** Widget that contains dynamically generated
   * PropertyWidget's for each property of an algorithm,
   * contained in a scroll area.
    
    @date 2012-03-09

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport AlgorithmPropertiesWidget : public QWidget
  {
    Q_OBJECT
    Q_PROPERTY( QString algorithmName READ getAlgorithmName WRITE setAlgorithmName )

  public:
    AlgorithmPropertiesWidget(QWidget * parent = NULL);
    virtual ~AlgorithmPropertiesWidget();
    
    void initLayout();

    Mantid::API::IAlgorithm * getAlgorithm();

    QString getAlgorithmName() const;
    void setAlgorithmName(QString name);

  private:
    /// Chosen algorithm name
    QString m_algoName;

    /// Shared pointer to the algorithm to view
    Mantid::API::Algorithm_sptr m_algo;

    /// The grid widget containing the input boxes
    QGridLayout *m_inputGrid;

    /// The current grid widget for sub-boxes
    QGridLayout *m_currentGrid;

    /// Viewport containing the grid of property widgets
    QWidget * m_viewport;

    /// Scroll area containing the viewport
    QScrollArea * m_scroll;

    /// Each dynamically created PropertyWidget
    QVector<PropertyWidget*> m_propWidgets;

  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_ALGORITHMPROPERTIESWIDGET_H_ */
