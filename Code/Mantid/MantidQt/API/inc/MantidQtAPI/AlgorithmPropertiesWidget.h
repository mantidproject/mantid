#ifndef MANTID_API_ALGORITHMPROPERTIESWIDGET_H_
#define MANTID_API_ALGORITHMPROPERTIESWIDGET_H_

#include "DllOption.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/System.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/PropertyWidget.h"
#include <qgridlayout.h>
#include <QtCore/qvariant.h>
#include <QtGui/qwidget.h>
#include <QtGui>


namespace MantidQt
{
namespace API
{


  /** Widget that contains dynamically generated
   * PropertyWidget's for each property of an algorithm,
   * contained in a scroll area.
    
    @date 2012-03-09

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class EXPORT_OPT_MANTIDQT_API AlgorithmPropertiesWidget : public QWidget
  {
    Q_OBJECT
    Q_PROPERTY( QString algorithmName READ getAlgorithmName WRITE setAlgorithmName )

  public:
    AlgorithmPropertiesWidget(QWidget * parent = NULL);
    virtual ~AlgorithmPropertiesWidget();
    
    void setInputHistory(MantidQt::API::AbstractAlgorithmInputHistory * inputHistory);

    void initLayout();

    Mantid::API::IAlgorithm_sptr  getAlgorithm();
    void setAlgorithm(Mantid::API::IAlgorithm_sptr algo);

    QString getAlgorithmName() const;
    void setAlgorithmName(QString name);

    void addEnabledAndDisableLists(const QStringList & enabled, const QStringList & disabled);

    void hideOrDisableProperties();

    void saveInput();

    /// Each dynamically created PropertyWidget
    QHash<QString, PropertyWidget*> m_propWidgets;

    /// Mapping between group and it's dynamically created widget
    QHash<QString, QGroupBox*> m_groupWidgets;

    /// Viewport containing the grid of property widgets
    QWidget * m_viewport;

    /// Scroll area containing the viewport
    QScrollArea * m_scroll;

  public slots:
    /// Any property changed
    void propertyChanged(const QString & pName);

    /// Replace WS button was clicked
    void replaceWSClicked(const QString & propName);

  private:

    bool isWidgetEnabled(Mantid::Kernel::Property * property, const QString & propName) const;

    /// Chosen algorithm name
    QString m_algoName;

    /// Pointer to the algorithm to view
    Mantid::API::IAlgorithm_sptr m_algo;

    /// The grid widget containing the input boxes
    QGridLayout *m_inputGrid;

    /// The current grid widget for sub-boxes
    QGridLayout *m_currentGrid;

    /// A map where key = property name; value = the error for this property (i.e. it is not valid).
    QHash<QString, QString> m_errors;

    /// A list of property names that are FORCED to stay enabled.
    QStringList m_enabled;

    /// A list of property names that are FORCED to stay disabled.
    /// e.g. when callid AlgorithmNameDialog()
    QStringList m_disabled;

    /// History of inputs to the algorithm
    MantidQt::API::AbstractAlgorithmInputHistory * m_inputHistory;

  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_ALGORITHMPROPERTIESWIDGET_H_ */
