#ifndef MPLEVENT_H
#define MPLEVENT_H
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
#include "MantidQtWidgets/MplCpp/DllOption.h"

#include <QMetaType>
#include <QPoint>

//
// A collection of classes to mirror the matplotlib events in
// matplotlib.backend_bases.
//
// They are delibarately not defined within a namespace as this complicates
// the signal/slot connection syntax
//

/**
 * The MplMouseEvent class is designed to stores additional information on top
 * of the standard QMouseEvent. It is generally dispatched alongside
 * the QMouseEvent to avoid copying any data
 */
class EXPORT_OPT_MANTIDQT_MPLCPP MplMouseEvent {
public:
  MplMouseEvent(QPointF dataPos);

  inline QPointF dataPos() const { return m_dataPos; }

private:
  QPointF m_dataPos;
};
#endif // MPLEVENT_H
