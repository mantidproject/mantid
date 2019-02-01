// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// fake QApplication

class QWidget;

class QApplication
{
public:
    QApplication( int &, char ** ) {}
    void exec() {}
    void setMainWidget( void * ) {}
    void processEvents() {}
    static QWidget *desktop() { return 0; }
    void *activeWindow() { return 0; }
};
