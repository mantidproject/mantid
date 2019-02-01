// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// fake qstatusbar.h

class QStatusBar
{
public:
    QStatusBar( void * ) {}
    void setProgress()  {}
    void addWidget( void *, int ) {}
    void removeWidget( void * ) {}
};
