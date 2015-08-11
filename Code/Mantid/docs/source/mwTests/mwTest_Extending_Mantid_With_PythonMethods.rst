:orphan:

.. testcode:: mwTest_Extending_Mantid_With_PythonMethods[10]

   class Person(object):
   
       name = "Bob"
   
       def sleep(self):
           print self.name,'is sleeping'


.. testsetup:: mwTest_Extending_Mantid_With_PythonMethods[23]

   class Person(object):
   
       name = "Bob"
   
       def sleep(self):
           print self.name,'is sleeping'

.. testcode:: mwTest_Extending_Mantid_With_PythonMethods[23]

   person1 = Person()
   # Note: We call it with no arguments, Python inserts self automatically. 
   # We use parantheses because sleep is a function
   person1.sleep()

.. testoutput:: mwTest_Extending_Mantid_With_PythonMethods[23]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Bob is sleeping


.. testcode:: mwTest_Extending_Mantid_With_PythonMethods[45]

   class Person(object):
   
       name = "Bob"
   
       def sleep(self, nhours):
           print self.name,'is sleeping for ',str(nhours),'hours'
   ##################################################################
   person1 = Person()
   # Called with 1 argument, Python inserts the self automatically.
   person1.sleep(7)

.. testoutput:: mwTest_Extending_Mantid_With_PythonMethods[45]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Bob is sleeping for 7 hours


.. testcode:: mwTest_Extending_Mantid_With_PythonMethods[72]

   class Person(object):
   
       name = None
   
       def __init__(self, name):
           self.name = name
   
       def sleep(self):
           print self.name,'is sleeping'
   
   ###################################################
   
   # Usage:
   person1 = Person("Bob")
   person2 = Person("Alice")
   
   person1.sleep() # => prints "Bob is sleeping"
   person2.sleep() # => prints "Alice is sleeping"

.. testoutput:: mwTest_Extending_Mantid_With_PythonMethods[72]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Bob is sleeping
   Alice is sleeping


