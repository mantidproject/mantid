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
    def find_encoder(cls, obj):
        """
        This assumes that obj is of a class that has an encode else it returns None
        :param obj: The object for encoding
        :return: Encoder or None; Returns the Encoder of the obj
        """
        for encoder in cls.encoder_list:
            if encoder().has_tag(obj.__class__.__name__):
                return encoder()
        return None

    @classmethod
    def register_encoder(cls, encoder):
        for encoder_ in cls.encoder_list:
            if encoder is encoder_:
                # It's a duplicate
                return
        cls.encoder_list.append(encoder)
