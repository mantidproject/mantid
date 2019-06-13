// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_PLOTTING_MPL_CONTOURPREVIEWPLOT_H_
#define MANTIDQT_PLOTTING_MPL_CONTOURPREVIEWPLOT_H_

#include "ui_ContourPreviewPlot.h"

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_PLOTTING ContourPreviewPlot : public QWidget {
  Q_OBJECT

public:
  ContourPreviewPlot(QWidget *parent = nullptr);
  ~ContourPreviewPlot() override;

  Ui::ContourPreviewPlot m_uiForm;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* MANTIDQT_PLOTTING_MPL_CONTOURPREVIEWPLOT_H_ */
