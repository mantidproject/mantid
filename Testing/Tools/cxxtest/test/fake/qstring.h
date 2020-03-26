// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
// fake qstring.h
#pragma once

class QString
{
public:
    QString() {}
    QString( const char * ) {}
    bool operator==( const QString & ) { return false; }

    static QString number( int ) { return QString(); }
};

inline QString operator+( const QString &, const QString & ) { return QString(); }
