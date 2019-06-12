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
        self.assertEqual(el.getNumberEvents(), 0)
        self.assertEqual(el.getEventType(), EventType.TOF)

    def test_event_list_addEventQuickly(self):
        el = EventList()
        el.addEventQuickly(float(0.123), DateAndTime(42))
        self.assertEqual(el.getNumberEvents(), 1)
        self.assertEqual(el.getEventType(), EventType.TOF)
        self.assertEqual(el.getTofs()[0], float(0.123))
        self.assertEqual(el.getPulseTimes()[0], DateAndTime(42))


    def test_event_list_iadd(self):
        left = self.createRandomEventList(10)
        rght = self.createRandomEventList(20)

        left += rght

        self.assertEqual(left.getEventType(), EventType.TOF)
        self.assertEqual(rght.getEventType(), EventType.TOF)

        self.assertEqual(left.getNumberEvents(), 30)
        self.assertEqual(rght.getNumberEvents(), 20)

    def test_event_list_isub(self):
        left = self.createRandomEventList(10)
        rght = self.createRandomEventList(20)

        left -= rght

        self.assertEqual(left.getEventType(), EventType.WEIGHTED)
        self.assertEqual(rght.getEventType(), EventType.TOF)

        self.assertEqual(left.getNumberEvents(), 30)
        self.assertEqual(rght.getNumberEvents(), 20)

        self.assertEqual(left.integrate(-1.,31., True), -10.)

    def test_mask_condition(self):
        evl = self.createRandomEventList(20)

        tof = evl.getTofs()
        mask = (tof < 10)
        evl.maskCondition(mask)

        self.assertEqual(evl.getNumberEvents(), 10)
        self.assertEqual(evl.getTofMax(), float(9.0))

if __name__ == '__main__':
    unittest.main()
