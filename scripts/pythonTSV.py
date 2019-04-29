# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)

"""
Some simple helpers for dealing with the TSV
"""


def saveToTSV(TSV, value):
    """
    This will record the value based on its type,
    will only record default TSV data types
    """
    if isinstance(value, int):
        TSV.storeInt(value)
    elif isinstance(value, float):
        TSV.storeDouble(value)
    elif isinstance(value, bool):
        TSV.storeBool(value)
    elif isinstance(value, str):
        TSV.storeString(value)
    else:
        raise TypeError("Value is not recognised by TSVSerialiser")


def loadFromTSV(TSV, key, value):
    """
    Will return the stored data from a TSV
    with from the line called key and with
    a data type of value
    """
    safeKey = makeLineNameSafe(key)
    TSV.selectLine(safeKey)
    if isinstance(value, int):
        return TSV.readInt()
    elif isinstance(value, float):
        return TSV.readDouble()
    elif isinstance(value, bool):
        return TSV.readBool()
    elif isinstance(value, str):
        return TSV.readString()
    else:
        raise TypeError("Value is not recognised by TSVSerialiser")

"""
The line name cannot contain:
spaces, underscores or dashes
"""


def makeLineNameSafe(oldName):
    newName = removeUnsafeCharacter(oldName, " ")
    newName = removeUnsafeCharacter(newName, "_")
    newName = removeUnsafeCharacter(newName, "-")
    return newName


"""
write a lines and make sure it is safe
"""


def writeLine(TSV, name):
    newName = makeLineNameSafe(name)
    TSV.writeLine(newName)


def removeUnsafeCharacter(oldName, character):
    tmp = oldName.split(character)
    newName = ""
    for word in tmp:
        newName += word[0].upper() + word[1:]
    return newName
