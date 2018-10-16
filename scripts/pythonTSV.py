# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)


def saveToTSV(TSV,value):
    if isinstance(value,int):
        TSV.storeInt(value)
    elif isinstance(value, float):
        TSV.storeDouble(value)
    elif isinstance(value,bool):
        TSV.storeBool(value)
    elif isinstance(value,str):
        TSV.storeString(value)
    else:
        raise TypeError

def loadFromTSV(TSV,key,value):
#    print(value, "waa", key)
    TSV.selectLine(key)
    if isinstance(value,int):
        tmp= TSV.readInt()
    elif isinstance(value, float):
        tmp= TSV.readDouble()
    elif isinstance(value,bool):
        tmp= TSV.readBool()
    elif isinstance(value,str):
        tmp= TSV.readString()
    else:
        raise TypeError
    return tmp


