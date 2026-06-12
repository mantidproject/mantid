# Error Reporter Resources

This directory contains a Qt resource file, `errorreports.qrc`, and a
checked-in `resources.py` module generated from it with `pyrcc5` (with its
`PyQt5` import rewritten to `qtpy`).

A single module serves every Qt binding: the rcc data format it embeds and
the `qRegisterResourceData` API it calls are supported by both PyQt5 and
PyQt6, and PyQt6 ships no resource compiler of its own. The module is
imported from `main.py` before the `QApplication` is created.

If `errorreports.qrc` changes, regenerate the module from a PyQt5
environment by running the following in this directory and rewriting the
`from PyQt5 import QtCore` line to `from qtpy import QtCore`:

```
python3 -m PyQt5.pyrcc_main -o resources.py errorreports.qrc
```
