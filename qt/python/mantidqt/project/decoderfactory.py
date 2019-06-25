# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from __future__ import (absolute_import, division, print_function, unicode_literals)


class DecoderFactory(object):
    decoder_list = set([])

    @classmethod
    def find_decoder(cls, tag):
        """
        This assumes that tag is present in a decoder and if it cannot find a decoder with the tag it raises ValueError
        :param tag: String; The tag used for saving and thus to be reused for loading that encoder back.
        :return: Decoder object; The object of a decoder for the tag that was passed.
        """
        for decoder in cls.decoder_list:
            if tag in decoder.tags:
                return decoder()
        raise ValueError("Unable to find decoder for tag: " + tag + " Please add the decoder for: " + tag)

    @classmethod
    def register_decoder(cls, decoder):
        """
        Will register the decoder class with the factory
        :param decoder: The Decoder class; to be registered with the Factory
        """
        cls.decoder_list.add(decoder)
