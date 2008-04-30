/****************************************************************************
** Meta object code from reading C++ file 'qwt_legend_item.h'
**
** Created: Tue 29. Apr 13:25:38 2008
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../qwt_legend_item.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qwt_legend_item.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_QwtLegendItem[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // signals: signature, parameters, type, tag, flags
      15,   14,   14,   14, 0x05,
      25,   14,   14,   14, 0x05,
      35,   14,   14,   14, 0x05,
      46,   14,   14,   14, 0x05,

 // slots: signature, parameters, type, tag, flags
      63,   60,   14,   14, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_QwtLegendItem[] = {
    "QwtLegendItem\0\0clicked()\0pressed()\0"
    "released()\0checked(bool)\0on\0"
    "setChecked(bool)\0"
};

const QMetaObject QwtLegendItem::staticMetaObject = {
    { &QwtTextLabel::staticMetaObject, qt_meta_stringdata_QwtLegendItem,
      qt_meta_data_QwtLegendItem, 0 }
};

const QMetaObject *QwtLegendItem::metaObject() const
{
    return &staticMetaObject;
}

void *QwtLegendItem::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QwtLegendItem))
	return static_cast<void*>(const_cast< QwtLegendItem*>(this));
    return QwtTextLabel::qt_metacast(_clname);
}

int QwtLegendItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QwtTextLabel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: clicked(); break;
        case 1: pressed(); break;
        case 2: released(); break;
        case 3: checked((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: setChecked((*reinterpret_cast< bool(*)>(_a[1]))); break;
        }
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void QwtLegendItem::clicked()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void QwtLegendItem::pressed()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void QwtLegendItem::released()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void QwtLegendItem::checked(bool _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
