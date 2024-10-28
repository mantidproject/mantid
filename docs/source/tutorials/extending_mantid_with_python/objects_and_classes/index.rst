.. _emwp_obj_and_classes:

===================
Objects And Classes
===================

.. toctree::
   :hidden:
   :glob:
   :maxdepth: 1

   01_methods
   02_inheritance
   03_exercise_1

Central concepts are the ideas of classes and objects, which form the basis of
Object-Oriented (OO) programming. Not too much in-depth knowledge of these
topics is required but it is useful to understand the basics.

Object Orientation
==================

Any program is essentially a collection of actions that are applied to some
data.

In an OO design, the data and actions are broken down and combined to form
objects that model parts of the "system".

The objects are then combined together to form the program as a whole.

Classes
=======

A ``class`` is a blueprint that is used to construct an object, i.e. tells it
what data it knows about and what actions can be applied to it.

In Python this is done using the ``class`` keyword, for example, a ``Person``
class with a single piece of data (attribute) that stores their name would
look like:

.. code-block:: python

    class Person(object):
        name = "Bob"

The class is used by constructing an object that is defined by the given class
and then accessing the attributes:

.. code-block:: python

    person_object = Person()
    print(person_object.name)

The attributes can be changed but the changes apply to that object and not
the class, i.e. the blueprint is not affected:

.. code-block:: python

    person_object1 = Person()
    person_object1.name = 'Alice'

    person_object2 = Person()
    print("Person 1 name: " + person_object1.name)  # => prints Alice
    print("Person 2 name: " + person_object2.name)  # => prints Bob

**Contents**

* :ref:`01_methods`
* :ref:`02_inheritance`
* :ref:`03_exercise_1`
