/****************************************************************************
** Meta object code from reading C++ file 'qwt_plot_picker.h'
**
** Created: Tue 29. Apr 13:25:39 2008
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../qwt_plot_picker.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qwt_plot_picker.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_QwtPlotPicker[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      19,   15,   14,   14, 0x05,
      49,   44,   14,   14, 0x05,
      76,   73,   14,   14, 0x05,
     111,   15,   14,   14, 0x05,
     136,   15,   14,   14, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_QwtPlotPicker[] = {
    "QwtPlotPicker\0\0pos\0selected(QwtDoublePoint)\0"
    "rect\0selected(QwtDoubleRect)\0pa\0"
    "selected(QwtArray<QwtDoublePoint>)\0"
    "appended(QwtDoublePoint)\0moved(QwtDoublePoint)\0"
};

const QMetaObject QwtPlotPicker::staticMetaObject = {
    { &QwtPicker::staticMetaObject, qt_meta_stringdata_QwtPlotPicker,
      qt_meta_data_QwtPlotPicker, 0 }
};

const QMetaObject *QwtPlotPicker::metaObject() const
{
    return &staticMetaObject;
}

void *QwtPlotPicker::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QwtPlotPicker))
	return static_cast<void*>(const_cast< QwtPlotPicker*>(this));
    return QwtPicker::qt_metacast(_clname);
}

int QwtPlotPicker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QwtPicker::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: selected((*reinterpret_cast< const QwtDoublePoint(*)>(_a[1]))); break;
        case 1: selected((*reinterpret_cast< const QwtDoubleRect(*)>(_a[1]))); break;
        case 2: selected((*reinterpret_cast< const QwtArray<QwtDoublePoint>(*)>(_a[1]))); break;
        case 3: appended((*reinterpret_cast< const QwtDoublePoint(*)>(_a[1]))); break;
        case 4: moved((*reinterpret_cast< const QwtDoublePoint(*)>(_a[1]))); break;
        }
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void QwtPlotPicker::selected(const QwtDoublePoint & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void QwtPlotPicker::selected(const QwtDoubleRect & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void QwtPlotPicker::selected(const QwtArray<QwtDoublePoint> & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void QwtPlotPicker::appended(const QwtDoublePoint & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void QwtPlotPicker::moved(const QwtDoublePoint & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
