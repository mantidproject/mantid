# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, too-many-public-methods
from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.kernel import DateAndTime
from mantid.api import EventType
from mantid.dataobjects import EventList

class EventListTest(unittest.TestCase):

    def createRandomEventList(self, length):
        el = EventList()
        for i in range(length):
            el.addEventQuickly(float(i), DateAndTime(i))
        return el

    def test_event_list_constructor(self):
        el = EventList()
        self.assertEquals(el.getNumberEvents(), 0)
        self.assertEquals(el.getEventType(), EventType.TOF)

    def test_event_list_addEventQuickly(self):
        el = EventList()
        el.addEventQuickly(float(0.123), DateAndTime(42))
        self.assertEquals(el.getNumberEvents(), 1)
        self.assertEquals(el.getEventType(), EventType.TOF)
        self.assertEquals(el.getTofs()[0], float(0.123))
        self.assertEquals(el.getPulseTimes()[0], DateAndTime(42))


    def test_event_list_iadd(self):
        left = self.createRandomEventList(10)
        rght = self.createRandomEventList(20)

        left += rght

        self.assertEquals(left.getEventType(), EventType.TOF)
        self.assertEquals(rght.getEventType(), EventType.TOF)

        self.assertEquals(left.getNumberEvents(), 30)
        self.assertEquals(rght.getNumberEvents(), 20)

    def test_event_list_isub(self):
        left = self.createRandomEventList(10)
        rght = self.createRandomEventList(20)

        left -= rght

        self.assertEquals(left.getEventType(), EventType.WEIGHTED)
        self.assertEquals(rght.getEventType(), EventType.TOF)

        self.assertEquals(left.getNumberEvents(), 30)
        self.assertEquals(rght.getNumberEvents(), 20)

        self.assertEquals(left.integrate(-1.,31., True), -10.)

    def test_mask_condition(self):
        evl = self.createRandomEventList(20)

        tof = evl.getTofs()
        mask = (tof < 10)
        evl.maskCondition(mask)

        self.assertEquals(evl.getNumberEvents(), 10)
        self.assertEquals(evl.getTofMax(), float(9.0))

if __name__ == '__main__':
    unittest.main()
