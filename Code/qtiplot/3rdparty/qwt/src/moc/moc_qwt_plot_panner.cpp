/****************************************************************************
** Meta object code from reading C++ file 'qwt_plot_panner.h'
**
** Created: Tue 29. Apr 13:25:38 2008
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../qwt_plot_panner.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qwt_plot_panner.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_QwtPlotPanner[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
      21,   15,   14,   14, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_QwtPlotPanner[] = {
    "QwtPlotPanner\0\0dx,dy\0moveCanvas(int,int)\0"
};

const QMetaObject QwtPlotPanner::staticMetaObject = {
    { &QwtPanner::staticMetaObject, qt_meta_stringdata_QwtPlotPanner,
      qt_meta_data_QwtPlotPanner, 0 }
};

const QMetaObject *QwtPlotPanner::metaObject() const
{
    return &staticMetaObject;
}

void *QwtPlotPanner::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QwtPlotPanner))
	return static_cast<void*>(const_cast< QwtPlotPanner*>(this));
    return QwtPanner::qt_metacast(_clname);
}

int QwtPlotPanner::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QwtPanner::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: moveCanvas((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        }
        _id -= 1;
    }
    return _id;
}
