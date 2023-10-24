.. _01_basic_algorithm_structure:

=========================
Basic Algorithm Structure
=========================

Each Python algorithm within Mantid must contain a few elements in order for it to be recognised as an algorithm.

The basic layout should look like:

.. code-block:: python
   :linenos:

   from mantid.api import AlgorithmFactory, PythonAlgorithm

   class HelloWorld(PythonAlgorithm):

       def PyInit(self):
           # Declare properties
           pass

       def PyExec(self):
           # Run the algorithm
           pass

   # Register algorithm with Mantid
   AlgorithmFactory.subscribe(HelloWorld)

The super class is ``PythonAlgorithm`` (alias to :class:`Algorithm <mantid.api.Algorithm>`), which provides a hook to allow Mantid to interact with it without knowing about it beforehand.

The functions ``PyInit`` and ``PyExec`` are called by the framework to setup properties and run the algorithm respectively and must be included.

The final line, which should be outside the class definition, registers the new algorithm class with Mantid's :class:`AlgorithmFactory <mantid.api.AlgorithmFactoryImpl>`.


Each algorithm needs to belong to a category.
By default a Python algorithm belongs to the "PythonAlgorithms",
but Mantid will emit a warning on registration if you leave it using the default value.
To change this include the ``category()`` function in the definition, i.e.

.. code-block:: python
   :linenos:

   from mantid.api import AlgorithmFactory, PythonAlgorithm

   class HelloWorld(PythonAlgorithm):

       def category(self):
           return 'MyTools'

       # The rest is the same as above

Subcategories can be defined using a **"\\\\\\\\"** (you need two **\\\\**'s as the first is the escape character) to separate the categories.
You can also state that your algorithm should appear in multiple categories by separating them with
a semi-colon **";"**.

i.e. The following code defines that the algorithm should be stored in Useful->Tools and MyTools.

.. code-block:: python
   :linenos:

   from mantid.api import AlgorithmFactory, PythonAlgorithm

   class HelloWorld(PythonAlgorithm):

       def category(self):
          return 'Useful\\Tools;MyTools'

       # The rest is the same as above


You are highly encouraged to stay to the list of :ref:`existing categories <Algorithms List>`.
