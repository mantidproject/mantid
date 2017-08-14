#ifndef MANTIDQT_WIDGETS_INSTRUMENTVIEW_MINIPLOT_H
#define MANTIDQT_WIDGETS_INSTRUMENTVIEW_MINIPLOT_H
/*
 Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
*/
#include "MantidQtWidgets/MplCpp/MplPlotWidget.h"

namespace MantidQt {
namespace MantidWidgets {

class MiniPlot : public QWidget {
  Q_OBJECT

public:
  MiniPlot(QWidget *parent = nullptr);

private:
  MantidQt::Widgets::MplCpp::MplFigureCanvas *m_canvas;
};
}
}

#endif // MANTIDQT_WIDGETS_INSTRUMENTVIEW_MINIPLOT_H
