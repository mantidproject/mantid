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
    decoder_list = []

    @classmethod
    def find_decoder(cls, tag):
        for decoder in cls.decoder_list:
            if tag in decoder.tags:
                return decoder
        return False

    @classmethod
    def register_decoder(cls, decoder):
        for decoder_ in cls.decoder_list:
            if decoder is decoder_:
                # It's a duplicate
                return
        cls.decoder_list.append(decoder)
