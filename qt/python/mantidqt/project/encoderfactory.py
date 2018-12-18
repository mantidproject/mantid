# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)


class EncoderFactory(object):
    encoder_list = []

    @classmethod
    def find_encoder(cls, tag):
        for encoder in cls.encoder_list:
            if encoder.has_tag(tag):
                return encoder

        return False

    @classmethod
    def register_encoder(cls, encoder):
        cls.encoder_list.append(encoder)
