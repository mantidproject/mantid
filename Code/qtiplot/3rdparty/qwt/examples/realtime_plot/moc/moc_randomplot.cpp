/****************************************************************************
** Meta object code from reading C++ file 'randomplot.h'
**
** Created: Tue 29. Apr 13:26:08 2008
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../randomplot.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'randomplot.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_RandomPlot[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      12,   11,   11,   11, 0x05,

 // slots: signature, parameters, type, tag, flags
      26,   11,   11,   11, 0x0a,
      34,   11,   11,   11, 0x0a,
      55,   41,   11,   11, 0x0a,
      71,   11,   11,   11, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_RandomPlot[] = {
    "RandomPlot\0\0running(bool)\0clear()\0"
    "stop()\0timeout,count\0append(int,int)\0"
    "appendPoint()\0"
};

const QMetaObject RandomPlot::staticMetaObject = {
    { &IncrementalPlot::staticMetaObject, qt_meta_stringdata_RandomPlot,
      qt_meta_data_RandomPlot, 0 }
};

const QMetaObject *RandomPlot::metaObject() const
{
    return &staticMetaObject;
}

void *RandomPlot::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_RandomPlot))
	return static_cast<void*>(const_cast< RandomPlot*>(this));
    return IncrementalPlot::qt_metacast(_clname);
}

int RandomPlot::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = IncrementalPlot::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: running((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: clear(); break;
        case 2: stop(); break;
        case 3: append((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: appendPoint(); break;
        }
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void RandomPlot::running(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
