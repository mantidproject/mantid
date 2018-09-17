class group(object):

    def __init__(self, name="", dets=[]):
        self._name = name
        self._dets = dets

    @property
    def name(self):
        return self._name

    def setName(self, name):
        self._name = name

    def setDets(self, dets):
        self._dets = dets

    def isValid(self):
        if self._name != "" and len(self._dets):
            return True

        return False
