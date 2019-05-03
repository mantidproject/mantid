#!/usr/bin/env python
from __future__ import (absolute_import, division, print_function, unicode_literals)
import os
import sys


def parse_old_style_connect(qt_old_line, linenum):
    step1 = qt_old_line.split('connect(')[1]
    step1 = step1.replace(' ', '')
    terms = step1.split(',')
    if len(terms) != 3:
        raise RuntimeError('This is not right! L{1} {0}'.format(qt_old_line,
                                                                linenum))

    # leading whitespace
    whitespace = ' ' * (len(qt_old_line) - len(qt_old_line.lstrip(' ')))
    # widget name
    widget_name = terms[0]
    # event signal
    event_signal = terms[1]
    # method to handle event
    hander_method = terms[2].replace('(', '').replace(')', '').strip()

    return whitespace, widget_name, event_signal, hander_method


def convert_signal_connect(cmd, linenum):
    """
    Convert (very) old style signal connection to newer style
    """
    try:
        if cmd.strip().startswith('#'):
            return cmd
        if cmd.count('self.connect') != 1:
            return cmd
    except UnicodeDecodeError:
        print('L{} of source file encountered UnicodeDecodeError - not changing line'.format(linenum))
        return cmd

    whitespace, widget_name, event_signal, handler_method = parse_old_style_connect(cmd, linenum)
    if event_signal.count('accepted') > 0:
        signal_call = 'accepted'
    elif event_signal.count('activated') > 0:
        signal_call = 'activated'
    elif event_signal.count('clicked') > 0:
        signal_call = 'clicked'
    elif event_signal.count('currentIndexChanged') > 0:
        signal_call = 'currentIndexChanged'
    elif event_signal.count('indexChanged') > 0:
        # must follow currentIndexChanged
        signal_call = 'indexChanged'
    elif event_signal.count('stateChanged') > 0:
        signal_call = 'stateChanged'
    elif event_signal.count('toggled') > 0:
        signal_call = 'toggled'
    elif event_signal.count('itemSelectionChanged') > 0:
        signal_call = 'itemSelectionChanged'
    elif event_signal.count('rejected') > 0:
        signal_call = 'rejected'
    elif event_signal.count('returnPressed') > 0:
        signal_call = 'returnPressed'
    elif event_signal.count('textChanged') > 0:
        signal_call = 'textChanged'
    elif event_signal.count('triggered') > 0:
        signal_call = 'triggered'
    elif event_signal.count('valueChanged') > 0:
        signal_call = 'valueChanged'
    else:
        sys.stderr.write('L{1} signal {0} is not supported - skipping line\n'.format(event_signal, linenum))
        return cmd

    return '{0}{1}.{2}.connect({3})\n'.format(whitespace, widget_name, signal_call, handler_method)


QT4_TO_QTPY_FIXES = {'QtCore.QEventLoop': ('qtpy.QtCore', 'QEventLoop'),
                     'QtCore.QFile': ('qtpy.QtCore', 'QFile'),
                     'QtCore.QFileInfo': ('qtpy.QtCore', 'QFileInfo'),
                     'QtCore.QProcess': ('qtpy.QtCore', 'QProcess'),
                     'QtCore.QRegExp': ('qtpy.QtCore', 'QRegExp'),
                     'QtCore.QSize':  ('qtpy.QtCore', 'QSize'),
                     'QtCore.QSettings': ('qtpy.QtCore', 'QSettings'),
                     'QtCore.QThread':  ('qtpy.QtCore', 'QThread'),
                     'QtCore.QUrl': ('qtpy.QtCore', 'QUrl'),
                     'QtGui.QAction': ('qtpy.QtWidgets', 'QAction'),
                     'QtGui.QAbstractItemView': ('qtpy.QtWidgets', 'QAbstractItemView'),
                     'QtGui.QApplication': ('qtpy.QtWidgets', 'QApplication'),
                     'QtGui.QBrush': ('qtpy.QtGui', 'QBrush'),
                     'QtGui.QButtonGroup': ('qtpy.QtWidgets', 'QButtonGroup'),
                     'QtGui.QCheckBox': ('qtpy.QtWidgets', 'QCheckBox'),
                     'QtGui.QColor': ('qtpy.QtGui', 'QColor'),
                     'QtGui.QComboBox': ('qtpy.QtWidgets', 'QComboBox'),
                     'QtGui.QCursor': ('qtpy.QtGui', 'QCursor'),
                     'QtGui.QDesktopServices': ('qtpy.QGui', 'QDesktopServices'),
                     'QtGui.QDialog': ('qtpy.QtWidgets', 'QDialog'),
                     'QtGui.QDoubleValidator': ('qtpy.QtGui', 'QDoubleValidator'),
                     'QtGui.QDoubleSpinBox': ('qtpy.QtWidgets', 'QDoubleSpinBox'),
                     'QtGui.QFileDialog': ('qtpy.QtWidgets', 'QFileDialog'),
                     'QtGui.QFont': ('qtpy.QtGui', 'QFont'),
                     'QtGui.QFrame': ('qtpy.QtWidgets', 'QFrame'),
                     'QtGui.QGridLayout': ('qtpy.QtWidgets', 'QGridLayout'),
                     'QtGui.QGroupBox': ('qtpy.QtWidgets', 'QGroupBox'),
                     'QtGui.QHeaderView': ('qtpy.QtWidgets', 'QHeaderView'),
                     'QtGui.QHBoxLayout': ('qtpy.QtWidgets', 'QHBoxLayout'),
                     'QtGui.QIntValidator': ('qtpy.QtGui', 'QIntValidator'),
                     'QtGui.QMenu': ('qtpy.QtWidgets', 'QMenu'),
                     'QtGui.QLabel': ('qtpy.QtWidgets', 'QLabel'),
                     'QtGui.QLineEdit': ('qtpy.QtWidgets', 'QLineEdit'),
                     'QtGui.QMainWindow': ('qtpy.QtWidgets', 'QMainWindow'),
                     'QtGui.QPalette': ('qtpy.QtGui', 'QPalette'),
                     'QtGui.QMessageBox': ('qtpy.QtWidgets', 'QMessageBox'),
                     'QtGui.QPushButton': ('qtpy.QtWidgets', 'QPushButton'),
                     'QtGui.QRegExpValidator': ('qtpy.QtGui', 'QRegExpValidator'),
                     'QtGui.QRadioButton': ('qtpy.QtWidgets', 'QRadioButton'),
                     'QtGui.QScrollBar': ('qtpy.QtWidgets', 'QScrollBar'),
                     'QtGui.QStandardItem': ('qtpy.QtGui', 'QStandardItem'),
                     'QtGui.QStatusBar': ('qtpy.QtWidgets', 'QStatusBar'),
                     'QtGui.QStyleFactory': ('qtpy.QtWidgets', 'QStyleFactory'),
                     'QtGui.QTableWidgetSelectionRange': ('qtpy.QtWidgets', 'QTableWidgetSelectionRange'),
                     'QtGui.QTreeView': ('qtpy.QtWidgets', 'QTreeView'),
                     'QtGui.QSizePolicy': ('qtpy.QtWidgets', 'QSizePolicy'),
                     'QtGui.QSpacerItem': ('qtpy.QtWidgets', 'QSpacerItem'),
                     'QtGui.QTableWidgetItem': ('qtpy.QtWidgets', 'QTableWidgetItem'),
                     'QtGui.QTabWidget': ('qtpy.QtWidgets', 'QTabWidget'),
                     'QtGui.QTextEdit': ('qtpy.QtWidgets', 'QTextEdit'),
                     'QtGui.QVBoxLayout': ('qtpy.QtWidgets', 'QVBoxLayout'),
                     'QtGui.QWidget': ('qtpy.QtWidgets', 'QWidget'),
                     }


def convertToQtPy(command, linenum):
    imports = dict()

    try:
        for old_txt, (import_mod, new_txt) in QT4_TO_QTPY_FIXES.items():
            if old_txt in command:
                items = imports.get(import_mod, set())
                items.add(new_txt)
                imports[import_mod] = items
                command = command.replace(old_txt, new_txt)
        if ('QtCore' in command or 'QtGui' in command) and 'import' not in command:
            sys.stderr.write('L{} Found unknown qt call: "{}"\n'.format(linenum,
                                                                        command.strip()))
    except UnicodeDecodeError:
        pass  # would have already been an issue
    return command, imports


def read_and_convert_pyqt4_functions(filename):
    # parse and fix file
    lines = list()
    imports = dict()
    with open(filename, 'r') as handle:
        for linenum, line in enumerate(handle):
            # editors count from 1
            line = convert_signal_connect(line, linenum+1)
            line, imports_for_line = convertToQtPy(line, linenum+1)
            for key, values in imports_for_line.items():
                items = imports.get(key, set())
                for item in values:
                    items.add(item)
                imports[key] = items
            lines.append(line)

    if len(imports) > 0:
        print('========== Change imports of PyQt4 to be ==========')
        for key, values in imports.items():
            values = list(values)
            values.sort()
            values = ', '.join(values)
            print('from {} import ({})  # noqa'.format(key, values))

    # overwrite with new version since there weren't errors
    with open(filename, 'w') as handle:
        for line in lines:
            handle.write(line)
    return lines


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Convert old qt4 signals to qt5-style')
    parser.add_argument('files', nargs='+', help='script to convert')
    options = parser.parse_args()

    for filename in options.files:
        filename = os.path.abspath(os.path.expanduser(filename))
        if not os.path.exists(filename):
            parser.error('file "{}" does not exist'.format(filename))

        read_and_convert_pyqt4_functions(filename)
