#ifndef MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_FOURIERTRANSFORM_H_
#define MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_FOURIERTRANSFORM_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
#include "ui_DPDFFourierTransform.h"
// Mantid headers from other projects
#include "DllConfig.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
// 3rd party library headers
#include <QWidget>
// System headers

// Forward declarations
class QtProperty;
class QtTreePropertyBrowser;
class QtDoublePropertyManager;
class QtBoolPropertyManager;
class QtEnumPropertyManager;
class QtGroupPropertyManager;
class QSettings;

namespace MantidQt {
namespace API {
class AlgorithmRunner;
}
namespace CustomInterfaces {
namespace DynamicPDF {
class BackgroundRemover;
class InputDataControl;
class FitControl;
} // namespace DynamicPDF
} // namespace CustomInterfaces
} // namespace MantidQt
// end of forward declarations

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

/** An interface to visualize the G(r,E) and set options for the
  Fourier transform from S(Q,E).

  @date 2016-04-04

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

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
class MANTIDQT_DYNAMICPDF_DLL FourierTransform : public QWidget {
  Q_OBJECT

  friend class BackgroundRemover;

public:
  FourierTransform(QWidget *parent = nullptr);
  ~FourierTransform();

signals:
  void signalExtractResidualsHistogramFinished();

private slots:
  void resetAfterSliceSelected();
  void extractResidualsHistogram(const QString &modelWorkspaceName);
  void transform();
  void finishTransform(bool error);
  void transformAfterPropertyChanged(QtProperty *property);
  void clearFourierPlot();

private:
  void initLayout();
  void createPropertyTree();
  void setConnections();
  void setupPlotDisplay();
  void setInputDataControl(InputDataControl *inputDataControl);
  void setFitControl(FitControl *fitControl);
  void setDefaultPropertyValues();
  void updatePlot();

  /// object handling all input slices
  InputDataControl *m_inputDataControl;
  /// object handling the fitting for removal background
  FitControl *m_fitControl;
  /// object generated from the Qt -designer form
  Ui::FourierTransform m_uiForm;
  /// widget displaying properties for algorithm PDFFourierTransform
  QtTreePropertyBrowser *m_propertyTree;
  /// handy map to get a pointer to any property
  QMap<QString, QtProperty *> m_properties;
  /// Precision of doubles in m_doubleManager
  int m_decimals;
  /// Name of the workspace with the residuals of the model evaluation
  const std::string m_residualsName;
  /// Name of the workspace with the fourier transform
  const std::string m_fourierName;
  /// Associate a color to each type of transform
  QMap<QString, QColor> m_colors;
  /// Manager for double properties
  QtDoublePropertyManager *m_doubleManager;
  /// Manager for bool properties
  QtBoolPropertyManager *m_boolManager;
  /// Manager for the string list properties
  QtEnumPropertyManager *m_enumManager;
  /// Manager for groups of properties
  QtGroupPropertyManager *m_groupManager;
  /// Fit algorithm runner
  std::unique_ptr<MantidQt::API::AlgorithmRunner> m_algorithmRunner;
}; // class FourierTransform
} // namespace DynamicPDF
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_FOURIERTRANSFORM_H_
