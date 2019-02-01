// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// fake QLabel
#include <qstring.h>
#include <qwidget.h>

class QLabel
{
public:
    QLabel( void * ) {}
    void setText( const QString & ) {}
};
