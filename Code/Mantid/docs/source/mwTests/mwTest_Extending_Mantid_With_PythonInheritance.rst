:orphan:

.. testcode:: mwTest_Extending_Mantid_With_PythonInheritance[12]

   class Person(object):
       
       name = None
       age = None
   
       def __init__(self, name, age):
           self.name = name
           self.age = age
   
   #-----------------------------------
   
   class Student(Person): # Person becomes a super class of Student
   
       def __init__(self, name, age):
           # Basically calls Person.__init__(self, name, age)
           super(Student, self).__init__(name,age)
   
   #-----------------------------------
   
   class Lecturer(Person):
   
       def __init__(self, name, age):
           super(Lecturer, self).__init__(name,age)
   
   #-----------------------------------
   
   # Demo
   person1 = Student('Bob', 20)
   person2 = Lecturer('Alice', 40)
   
   # No parentheses required if accessing attribute
   print 'Student name:',person1.name 
   print 'Lecturer name:',person2.name

.. testoutput:: mwTest_Extending_Mantid_With_PythonInheritance[12]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Student name: Bob
   Lecturer name: Alice


