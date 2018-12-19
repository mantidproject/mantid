# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)


class InstrumentViewAttributes(object):
    # WARNING: If you delete a tag from here instead of adding a new one, it will make old project files obsolete
    tags = ["InstrumentView"]


class Decoder(InstrumentViewAttributes):
    def __init__(self):
        super(Decoder, self).__init__()

    #def decode(self, obj):

    @classmethod
    def has_tag(cls, tag):
        return tag in cls.tags


class Encoder(InstrumentViewAttributes):
    def __init__(self):
        super(Encoder, self).__init__()

    #def encode(self, obj):

    @classmethod
    def has_tag(cls, tag):
        return tag in cls.tags
