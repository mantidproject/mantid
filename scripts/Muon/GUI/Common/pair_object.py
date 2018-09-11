class pair(object):
    def __init__(self,name="",F_group="",B_group="",alpha=1.0):
        self._name = name
        self._F_group = F_group
        self._B_group = B_group
        self._alpha = alpha

    def setName(self,name):
         self._name = name

    def setFGroup(self, group):
         self._F_group = group

    def setBGroup(self, group):
         self._B_group = group

    def setAlpha(self,alpha):
         self._alpha = alpha

    def getName(self):
         return self._name

    def getFGroup(self):
         return self._F_group

    def getBGroup(self):
        return self._B_group

    def getAlpha(self):
        return self._alpha

    def isValid(self):
         if self._name != "" and self._F_group != "" and self._B_group != "":
              return True

         return False
