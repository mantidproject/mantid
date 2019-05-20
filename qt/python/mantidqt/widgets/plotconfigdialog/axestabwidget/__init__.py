# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.


class AxProperties:
    """
    An object to store the properties that can be set in the Axes
    Tab. It can be constructed from a view or an Axes object.
    """

    def __init__(self):
        pass

    @classmethod
    def from_ax_object(cls, ax):
        cls.title = ax.get_title().encode('unicode_escape')
        cls.xlim = ax.get_xlim()
        cls.xlabel = ax.get_xlabel()
        cls.xscale = ax.get_xscale()
        cls.ylim = ax.get_ylim()
        cls.ylabel = ax.get_ylabel()
        cls.yscale = ax.get_yscale()
        return cls()

    @classmethod
    def from_view(cls, view):
        cls.title = view.get_title().decode('unicode_escape')
        cls.xlim = [view.get_xlower_limit(), view.get_xupper_limit()]
        cls.xlabel = view.get_xlabel()
        cls.xscale = view.get_xscale()
        cls.ylim = [view.get_ylower_limit(), view.get_yupper_limit()]
        cls.ylabel = view.get_ylabel()
        cls.yscale = view.get_yscale()
        return cls()

    def __str__(self):
        s = ""
        for attr, val in self.__dict__.items():
            s += "{}: {}\n".format(attr, val)
        return s
