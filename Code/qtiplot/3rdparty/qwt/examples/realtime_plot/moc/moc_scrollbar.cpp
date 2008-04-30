/****************************************************************************
** Meta object code from reading C++ file 'scrollbar.h'
**
** Created: Tue 29. Apr 13:26:08 2008
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../scrollbar.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scrollbar.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_ScrollBar[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      14,   11,   10,   10, 0x05,
      57,   11,   10,   10, 0x05,

 // slots: signature, parameters, type, tag, flags
     109,  101,   10,   10, 0x0a,
     132,  101,   10,   10, 0x0a,
     164,  158,   10,   10, 0x08,
     187,  158,   10,   10, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ScrollBar[] = {
    "ScrollBar\0\0,,\0sliderMoved(Qt::Orientation,double,double)\0"
    "valueChanged(Qt::Orientation,double,double)\0"
    "min,max\0setBase(double,double)\0"
    "moveSlider(double,double)\0value\0"
    "catchValueChanged(int)\0catchSliderMoved(int)\0"
};

const QMetaObject ScrollBar::staticMetaObject = {
    { &QScrollBar::staticMetaObject, qt_meta_stringdata_ScrollBar,
      qt_meta_data_ScrollBar, 0 }
};

const QMetaObject *ScrollBar::metaObject() const
{
    return &staticMetaObject;
}

void *ScrollBar::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ScrollBar))
	return static_cast<void*>(const_cast< ScrollBar*>(this));
    return QScrollBar::qt_metacast(_clname);
}

int ScrollBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QScrollBar::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: sliderMoved((*reinterpret_cast< Qt::Orientation(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2])),(*reinterpret_cast< double(*)>(_a[3]))); break;
        case 1: valueChanged((*reinterpret_cast< Qt::Orientation(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2])),(*reinterpret_cast< double(*)>(_a[3]))); break;
        case 2: setBase((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 3: moveSlider((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 4: catchValueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: catchSliderMoved((*reinterpret_cast< int(*)>(_a[1]))); break;
        }
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void ScrollBar::sliderMoved(Qt::Orientation _t1, double _t2, double _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ScrollBar::valueChanged(Qt::Orientation _t1, double _t2, double _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
