#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Inspect and extract the PyQt configuration.

There is a commandline argument to select the version of PyQt.
"""
import argparse
import os
import pprint
import re
import site
import sys

# Regular expression to extract the Qt version tag
QT_TAG_RE = re.compile(r'Qt_\d+_\d+_\d+')


class PyQtConfig:
    """Inspects the installed PyQt version and extracts
    information about it.
    """

    version_hex: str
    version_str: str
    qt_tag: str
    sip_dir: str
    sip_flags: str
    pyuic_path: str

    def __init__(self, major_version):
        """Inspects the named PyQt package given the major version number
        :param major_version: The major version number of the library
        """
        pyqt_name = f'PyQt{major_version}'
        qtcore = __import__(f'{pyqt_name}.QtCore', globals(), locals(), ['QtCore'], 0)
        self.version_hex = qtcore.PYQT_VERSION
        self.version_str = qtcore.PYQT_VERSION_STR
        self.sip_flags = qtcore.PYQT_CONFIGURATION['sip_flags']
        self.qt_tag = self._get_qt_tag(self.sip_flags)

        conda_activated, conda_env = find_conda_env()
        if conda_activated:
            self.get_pyqt_conda_dirs(conda_env, pyqt_name)
        else:
            self.get_pyqt_dirs(pyqt_name)

        # Assume uic script is in uic submodule
        uic = __import__(pyqt_name + '.uic', globals(), locals(), ['uic'], 0)
        self.pyuic_path = os.path.join(os.path.dirname(uic.__file__), 'pyuic.py')

    def _get_qt_tag(self, sip_flags):
        match = QT_TAG_RE.search(sip_flags)
        if match:
            return match.group(0)
        else:
            return None

    def __str__(self):
        lines = [
            'pyqt_version:%06.x' % self.version_hex,
            'pyqt_version_str:%s' % self.version_str,
            'pyqt_version_tag:%s' % self.qt_tag,
            'pyqt_sip_dir:%s' % self.sip_dir,
            'pyqt_sip_flags:%s' % self.sip_flags,
            'pyqt_pyuic:%s' % self.pyuic_path
        ]
        return '\n'.join(lines)

    def get_pyqt_dirs(self, pyqt_name):
        # This is based on QScintilla's configure.py, and only works for the
        # default case where installation paths have not been changed in PyQt's
        # configuration process.
        if sys.platform == 'win32':
            self.sip_dir = os.path.join(sys.prefix, 'share', 'sip', pyqt_name)
        else:
            qt_maj_version = self.version_str[0]
            if sys.platform == 'darwin':
                prefix = '/usr/local/opt'  # brew Cellar
                possible_sip_dirs = []
                if qt_maj_version == '5':
                    possible_sip_dirs.append(os.path.join(site.getsitepackages()[0], 'PyQt5', 'bindings'))
                    possible_sip_dirs.append(os.path.join('pyqt', 'share', 'sip', 'Qt5'))
                    possible_sip_dirs.append(os.path.join('mantid-pyqt5', 'share', 'sip', 'Qt5'))
                else:
                    raise RuntimeError("Unknown Qt version ({}) found. Unable to determine location of PyQt sip files."
                                       "Please update FindPyQt accordingly.".format(self.version_str[0]))
            else:
                prefix = os.path.join(sys.prefix, 'share')
                possible_sip_dirs = (f'python{sys.version_info.major}{sys.version_info.minor}-sip/{pyqt_name}',
                                     f'python{sys.version_info.major}-sip/{pyqt_name}', f'sip/{pyqt_name}')
            for pyqt_sip_dir in possible_sip_dirs:
                if not os.path.isabs(pyqt_sip_dir):
                    pyqt_sip_dir = os.path.join(prefix, pyqt_sip_dir)
                if os.path.exists(pyqt_sip_dir):
                    self.sip_dir = pyqt_sip_dir
                    break
            if self.sip_dir is None:
                possible_sip_dirs = list(map(lambda p: os.path.join(prefix, p), possible_sip_dirs))
                raise RuntimeError(f"Unable to find {pyqt_name}.\n" + f"Tried following locations: {pprint.pformat(possible_sip_dirs)}")

    def get_pyqt_conda_dirs(self, conda_env_path, pyqt_name):
        if sys.platform != 'win32':
            self.sip_dir = os.path.join(conda_env_path, "share", "sip", pyqt_name)
        else:
            self.sip_dir = os.path.join(conda_env_path, 'sip', pyqt_name)


def find_conda_env():
    if 'CONDA_PREFIX' in os.environ:
        return True, os.environ['CONDA_PREFIX']
    return False, None


def main():
    # parse command line
    args = get_options()
    print(PyQtConfig(major_version=args.version))
    return 0


def get_options():
    parser = argparse.ArgumentParser(description='Extract PyQt config information')
    parser.add_argument('version', type=int, help="PyQt major version")
    return parser.parse_args()


if __name__ == "__main__":
    sys.exit(main())
