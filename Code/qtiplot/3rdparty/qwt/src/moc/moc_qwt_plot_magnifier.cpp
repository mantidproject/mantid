/****************************************************************************
** Meta object code from reading C++ file 'qwt_plot_magnifier.h'
**
** Created: Tue 29. Apr 13:25:39 2008
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../qwt_plot_magnifier.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'qwt_plot_magnifier.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

static const uint qt_meta_data_QwtPlotMagnifier[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets

       0        // eod
};

static const char qt_meta_stringdata_QwtPlotMagnifier[] = {
    "QwtPlotMagnifier\0"
};

const QMetaObject QwtPlotMagnifier::staticMetaObject = {
    { &QwtMagnifier::staticMetaObject, qt_meta_stringdata_QwtPlotMagnifier,
      qt_meta_data_QwtPlotMagnifier, 0 }
};

const QMetaObject *QwtPlotMagnifier::metaObject() const
{
    return &staticMetaObject;
}

void *QwtPlotMagnifier::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_QwtPlotMagnifier))
	return static_cast<void*>(const_cast< QwtPlotMagnifier*>(this));
    return QwtMagnifier::qt_metacast(_clname);
}

int QwtPlotMagnifier::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QwtMagnifier::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
