/****************************************************************************
** Meta object code from reading C++ file 'qwt_panner.h'
**
** Created: Tue 29. Apr 13:25:38 2008
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../qwt_panner.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qwt_panner.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_QwtPanner[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      17,   11,   10,   10, 0x05,
      33,   11,   10,   10, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_QwtPanner[] = {
    "QwtPanner\0\0dx,dy\0panned(int,int)\0"
    "moved(int,int)\0"
};

const QMetaObject QwtPanner::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_QwtPanner,
      qt_meta_data_QwtPanner, 0 }
};

const QMetaObject *QwtPanner::metaObject() const
{
    return &staticMetaObject;
}

void *QwtPanner::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QwtPanner))
	return static_cast<void*>(const_cast< QwtPanner*>(this));
    return QWidget::qt_metacast(_clname);
}

int QwtPanner::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: panned((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 1: moved((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        }
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void QwtPanner::panned(int _t1, int _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void QwtPanner::moved(int _t1, int _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
