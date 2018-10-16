// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// fake qmessagebox.h

class QMessageBox
{
public:
    enum Icon { Information, Warning, Critical };
    static void *standardIcon( Icon ) { return 0; }
};
