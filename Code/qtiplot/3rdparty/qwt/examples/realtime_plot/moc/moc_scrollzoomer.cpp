/****************************************************************************
** Meta object code from reading C++ file 'scrollzoomer.h'
**
** Created: Tue 29. Apr 13:26:08 2008
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../scrollzoomer.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scrollzoomer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_ScrollZoomer[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
      24,   14,   13,   13, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScrollZoomer[] = {
    "ScrollZoomer\0\0o,min,max\0"
    "scrollBarMoved(Qt::Orientation,double,double)\0"
};

const QMetaObject ScrollZoomer::staticMetaObject = {
    { &QwtPlotZoomer::staticMetaObject, qt_meta_stringdata_ScrollZoomer,
      qt_meta_data_ScrollZoomer, 0 }
};

const QMetaObject *ScrollZoomer::metaObject() const
{
    return &staticMetaObject;
}

void *ScrollZoomer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScrollZoomer))
	return static_cast<void*>(const_cast< ScrollZoomer*>(this));
    return QwtPlotZoomer::qt_metacast(_clname);
}

int ScrollZoomer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QwtPlotZoomer::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: scrollBarMoved((*reinterpret_cast< Qt::Orientation(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2])),(*reinterpret_cast< double(*)>(_a[3]))); break;
        }
        _id -= 1;
    }
    return _id;
}
