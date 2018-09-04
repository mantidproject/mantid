class pair(object):
    def __init__(self):
        self._name = ""
        self._F_group = ""
        self._B_group = ""
        self._alpha = 1.0

    def setName(self,name):
         self._name = name

    def setFGroup(self, group):
         self._F_group = group

    def setBGroup(self, group):
         self._B_group = group

    def setAlpha(self,alpha):
         self._alpha = alpha

    def isValid(self):
         if self._name != "" and self._F_group != "" and self._B_group != "":
              return True

         return False
