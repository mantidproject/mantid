# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +


def can_plot_beamcentrefinder():
    PYQT4 = False
    IN_MANTIDPLOT = False
    WITHOUT_GUI = False
    try:
        from qtpy import PYQT4
    except ImportError:
        pass  # it is already false
    if PYQT4:
        try:
            import mantidplot
            IN_MANTIDPLOT = True
        except (Exception, Warning):
            pass
    else:
        try:
            from mantidqt.plotting.functions import plot
        except ImportError:
            WITHOUT_GUI = True

    return PYQT4, IN_MANTIDPLOT, WITHOUT_GUI
