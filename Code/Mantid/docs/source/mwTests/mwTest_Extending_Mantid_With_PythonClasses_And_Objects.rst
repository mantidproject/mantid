:orphan:

.. testcode:: mwTest_Extending_Mantid_With_PythonClasses_And_Objects[22]

   class Person(object):
       name = "Bob"


.. testsetup:: mwTest_Extending_Mantid_With_PythonClasses_And_Objects[31]

   class Person(object):
       name = "Bob"

.. testcode:: mwTest_Extending_Mantid_With_PythonClasses_And_Objects[31]

   person_object = Person()
   print person_object.name

.. testoutput:: mwTest_Extending_Mantid_With_PythonClasses_And_Objects[31]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Bob


.. testsetup:: mwTest_Extending_Mantid_With_PythonClasses_And_Objects[45]

   class Person(object):
       name = "Bob"

.. testcode:: mwTest_Extending_Mantid_With_PythonClasses_And_Objects[45]

   person_object1 = Person()
   person_object1.name = 'Alice'
   
   person_object2 = Person()
   print "Person 1 name:",person_object1.name  # => prints Alice
   print "Person 2 name:",person_object2.name  # => prints Bob

.. testoutput:: mwTest_Extending_Mantid_With_PythonClasses_And_Objects[45]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Person 1 name: Alice
   Person 2 name: Bob


