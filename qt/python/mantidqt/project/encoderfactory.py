# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)


def default_encoder_compatability_check(obj, encoder_cls):
    return encoder_cls.has_tag(obj.__class__.__name__)


class EncoderFactory(object):
    encoder_list = set([])

    @classmethod
    def find_encoder(cls, obj):
        """
        This assumes that obj is of a class that has an encode else it returns None
        :param obj: The object for encoding
        :return: Encoder or None; Returns the Encoder of the obj or None.
        """
        obj_encoders = [encoder for encoder, compatible in cls.encoder_list if compatible(obj, encoder)]
        if len(obj_encoders) > 1:
            raise RuntimeError("EncoderFactory: One or more encoder type claims to work with the passed obj: "
                               + obj.__class__.__name__)
        elif len(obj_encoders) == 1:
            return obj_encoders[0]()
        else:
            return None

    @classmethod
    def register_encoder(cls, encoder, compatible_check=None):
        """
        This adds the passed encoder's class to the available encoders in the Factory
        :param encoder: Class of Encoder; The class of the encoder to be added to the list.
        :param compatible_check: An optional function reference that will be used instead of comparing the encoder to
        potential widget candidates. Function should return True if compatible else False.
        """
        if compatible_check is None:
            compatible_check = default_encoder_compatability_check
        cls.encoder_list.add((encoder, compatible_check))
