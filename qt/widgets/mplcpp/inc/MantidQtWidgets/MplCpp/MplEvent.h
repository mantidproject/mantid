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
 * The MplLocationEvent class is a rough counterpart to
 * matplotlib.backend_bases.MouseEvent
 */
class EXPORT_OPT_MANTIDQT_MPLCPP MplMouseEvent {
public:
  // Q_DECLARE_METATYPE requires a default constructor
  MplMouseEvent() = default;
  MplMouseEvent(QPoint pos, QPointF dataPos, Qt::MouseButton button);

  inline QPoint pos() const { return m_pos; }
  inline QPointF dataPos() const { return m_dataPos; }
  inline Qt::MouseButton button() const { return m_button; }

private:
  QPoint m_pos;
  QPointF m_dataPos;
  Qt::MouseButton m_button;
};

//
// Declare these types to the Qt metatype system. The specializations must
// be in the same namespace that QMetaType is declared.
//
Q_DECLARE_METATYPE(MplMouseEvent)

#endif // MPLEVENT_H
