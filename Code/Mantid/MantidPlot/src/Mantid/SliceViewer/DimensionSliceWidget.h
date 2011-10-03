#ifndef DIMENSIONSLICEWIDGET_H
#define DIMENSIONSLICEWIDGET_H

#include <QtGui/QWidget>
#include "ui_DimensionSliceWidget.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"


/** Widget for the 2D slice viewer. Select whether the dimension
 * is X or Y, or if it is not one to be shown, where is the
 * slice.
 * Shows the dimension name and units
 *
 * @author Janik Zikovsky
 * @date Oct 3, 2011
 */

class DimensionSliceWidget : public QWidget
{
    Q_OBJECT

public:
    DimensionSliceWidget(QWidget *parent = 0);
    ~DimensionSliceWidget();

    void setDimension(Mantid::Geometry::IMDDimension_sptr dim);

    double getSlicePoint() const
    { return m_slicePoint; }

private:
    /// Auto-gen UI class
    Ui::DimensionSliceWidgetClass ui;

    /// Sptr to the dimensions being displayed
    Mantid::Geometry::IMDDimension_sptr m_dim;

    /// Which dimension is being shown. -1 = None, 0 = X, 1 = Y. 2+ reserved for higher dimensions
    int m_shownDim;

    /// If the dimensions is not shown, where is the slice point?
    double m_slicePoint;
};

#endif // DIMENSIONSLICEWIDGET_H
