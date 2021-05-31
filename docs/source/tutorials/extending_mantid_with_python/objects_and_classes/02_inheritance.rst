.. _02_inheritance:

===========
Inheritance
===========

Inheritance offers a mechanism to achieve two aims:

#. Share behaviour/data that is common between classes.
#. Define common interface to allow as yet unknown extensions to be plugged in.

Consider the example of a modelling a university system that contains two types
of person: *Lecturer* and *Student*.

These two types would share common attributes such as *name* & *age* so
duplicating these fields in each class is not ideal.

Instead the ``Person`` class can be "shared" between the ``Lecturer`` and
``Student`` classes by setting it as super class:

.. code-block:: python

    class Person(object):

        name = None
        age = None

        def __init__(self, name, age):
            self.name = name
            self.age = age

    #-----------------------------------

    class Student(Person): # Person becomes a super class of Student

        lectures = None

        def __init__(self, name, age, lectures):
            # Basically calls Person.__init__(self, name, age)
            super(Student, self).__init__(name, age)
            self.lectures = lectures

    #-----------------------------------

    class Lecturer(Person):

        salary = None

        def __init__(self, name, age, salary):
            super(Lecturer, self).__init__(name, age)
            self.salary = salary

    #-----------------------------------

    # Demo
    person1 = Student('Bob', 20, ['science', 'maths'])
    person2 = Lecturer('Alice', 40, 28000)

    # No parentheses required if accessing attribute
    print('Student name: ' + person1.name)
    print( 'Lecturer name: ' + person2.name)

    print('Student lectures: ' + str(person1.lectures))
    #print('Lecturer lectures: ' + str(person2.lectures)) => causes AttributeError

    print('Lecturer salary: ' + str(person2.salary))
    #print('Student salary: ' + str(person1.salary)) => causes AttributeError
