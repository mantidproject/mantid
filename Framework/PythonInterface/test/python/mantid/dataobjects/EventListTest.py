# pylint: disable=invalid-name, too-many-public-methods
import unittest

from mantid.kernel import DateAndTime
from mantid.api import EventType
from mantid.dataobjects import EventList

class EventListTest(unittest.TestCase):

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


if __name__ == '__main__':
    unittest.main()
