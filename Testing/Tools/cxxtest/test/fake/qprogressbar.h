// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// fake qprogressbar.h

class QColorGroup
{
public:
    enum { Highlight };
};

class QColor
{
public:
    QColor( int, int, int ) {}
};

class QPalette
{
public:
    void setColor( int, const QColor & ) {}
};

class QProgressBar
{
public:
    QProgressBar( int, void * ) {}
    void setProgress( int )  {}
    int progress()  { return 0; }
    QPalette palette() { return QPalette(); }
    void setPalette( const QPalette & ) {}
};
