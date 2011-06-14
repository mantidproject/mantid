#ifndef STANDARDVIEW_H_
#define STANDARDVIEW_H_

#include <QtGui/QWidget>
#include <QPointer>
#include "ViewBase.h"
#include "ui_StandardView.h"

class pqColorMapModel;
class pqPipelineRepresentation;
class pqPipelineSource;
class pqRenderView;
/**
 *
 This class represents the initial view for the main program. It is meant to
 be a view to play with the data in an unstructured manner.

 @author Michael Reuter
 @date 24/05/2011

 Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class StandardView : public ViewBase, public Ui::StandardView
{
  Q_OBJECT

public:
  /**
   * Default constructor.
   * @param parent the parent widget for the standard view
   */
  StandardView(QWidget *parent = 0);
  /// Default destructor.
  virtual ~StandardView();

  /**
   * ViewBase::getView
   */
  pqRenderView* getView();
  /**
   * ViewBase::render
   */
  void render();

signals:
  /**
   * Signal to get the range of the data.
   * @param min the minimum value of the data
   * @param max the maximum value of the data
   */
  void dataRange(double min, double max);

protected slots:
  /// Set the color scale back to the original bounds.
  void onAutoScale();
  /// Add a slice to the current dataset.
  void onCutButtonClicked();
  /**
   * Set the requested color map on the data.
   * @param model the color map to use
   */
  void onColorMapChange(const pqColorMapModel *model);
  /**
   * Set the data color scale range to the requested bounds.
   * @param min the minimum bound for the color scale
   * @param max the maximum bound for the color scale
   */
  void onColorScaleChange(double min, double max);
  /// Invoke the RebinnerCutter on the current dataset.
  void onRebinButtonClicked();


private:
  Q_DISABLE_COPY(StandardView);
  QPointer<pqPipelineSource> origSource; ///< The current source
  QPointer<pqPipelineRepresentation> originSourceRepr; ///< The current source representation
  QPointer<pqPipelineSource> rebinCut; ///< Holder for the RebinnerCutter
  QPointer<pqRenderView> view; ///< The main view
};

#endif // STANDARDVIEW_H_
