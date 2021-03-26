# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
from mantid.kernel import logger
from mantidqt.usersubwindowfactory import UserSubWindowFactory


def default_encoder_compatability_check(obj, encoder_cls):
    for tag in encoder_cls.tags():
        if tag == obj.__class__.__name__:
            return True
    return False


class EncoderFactory(object):
    encoder_dict = dict()  # cls name: (encoder, compatible check)

    @classmethod
    def find_encoder(cls, obj):
        """
        This assumes that obj is of a class that has an encode else it returns None
        :param obj: The object for encoding
        :return: Encoder or None; Returns the Encoder of the obj or None.
        """
        obj_encoders = [encoder for (encoder, compatible) in cls.encoder_dict.values() if compatible(obj, encoder)]

        if len(obj_encoders) > 1:
            raise RuntimeError("EncoderFactory: One or more encoder type claims to work with the passed obj: " +
                               obj.__class__.__name__)
        elif len(obj_encoders) == 1:
            return obj_encoders[0]()
        else:
            # attempt C++!
            return UserSubWindowFactory.Instance().findEncoder(obj)

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
        # check that an encoder of this class is not already registered
        if cls.__name__ in cls.encoder_dict:
            logger.debug("Overriding existing encoder")
        cls.encoder_dict[cls.__name__] = (encoder, compatible_check)
