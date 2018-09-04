class group(object):
    def __init__(self):
        self._name = ""
        self._dets = []

    def setName(self,name):
         self._name = name

    def setDets(self, dets):
         self._dets = dets

    def isValid(self):
         if self._name != "" and len(self._dets):
              return True

         return False
