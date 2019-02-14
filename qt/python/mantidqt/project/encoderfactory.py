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
    encoder_list = set([])

    @classmethod
    def find_encoder(cls, obj):
        """
        This assumes that obj is of a class that has an encode else it returns None
        :param obj: The object for encoding
        :return: Encoder or None; Returns the Encoder of the obj or None.
        """
        for encoder, compatible_check in cls.encoder_list:

            # Perform check if optional_cb is present if it is, then check whether the the optional_cb returns true for
            # this object.
            if compatible_check is not None and compatible_check(obj):
                return encoder()

            if encoder().has_tag(obj.__class__.__name__):
                return encoder()
        return None

    @classmethod
    def register_encoder(cls, encoder, compatible_check=None):
        """
        This adds the passed encoder's class to the available encoders in the Factory
        :param encoder: Class of Encoder; The class of the encoder to be added to the list.
        :param compatible_check: An optional function reference that will be used instead of comparing the encoder to
        potential widget candidates. Function should return True if compatible else False.
        """
        cls.encoder_list.add((encoder, compatible_check))
