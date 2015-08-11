:orphan:

.. testcode:: mwTest_Extending_Mantid_With_Python_Exercise_1_Solution[1]

   # Extending Mantid With Python: Exercise 1
   #
   # Write a Detector class contains two attributes id and name:
   #   - The attributes should be able to be set by a call like: Detector(1,"bank1_1").
   # Write an an Instrument class with 2 attributes: name and list of detectors
   #   - These attributes should also be able to be set by a call like: Instrument('MyInst',dets)
   # Add a method to the Instrument class called printTree that will print the names & ids of the detectors contained within it
   #
   # Write a class called Component that stores a single name attribute
   # Reorganise the Instrument & Detector classes to use Component as a super class and share its name attribute
   # Check the printTree method still functions correctly.
   
   
   class Component(object):
      name = ""
      
      def __init__(self, name):
           self.name = name
   
   class Detector(Component):
       id = None
   
       def __init__(self,id,name):
           super(Detector,self).__init__(name)
           self.id = id
   
   
   class Instrument(Component):
       detectors = None
       
       def __init__(self,name,detectors):
           super(Instrument,self).__init__(name)
           self.detectors = detectors
   
   
       def printTree(self):
           print "Instrument:",self.name
           for detector in self.detectors:
               print "Name:", detector.name, "ID:",detector.id
   
   ##########
   det1 = Detector(1,"Bank1")
   det2 = Detector(2,"Bank2")
   inst =  Instrument("My Instrument",[det1,det2])
   
   inst.printTree()

.. testoutput:: mwTest_Extending_Mantid_With_Python_Exercise_1_Solution[1]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Instrument: My Instrument
   Name: Bank1 ID: 1
   Name: Bank2 ID: 2


