.. _working_with_files:

==================
Working With Files 
==================

-  Reading/writing files in Python is made quite simple and both are
   controlled through strings.

Reading
=======

-  To be able to read a file we must first open a "handle" to it through
   the open command,

.. code:: python

   file = open('filename.txt', 'r')

-  where the first argument is the full path to the file and the second
   argument is one of mode flags:

   -  read - ``'r'``
   -  write - ``'w'``
   -  append - ``'a'``
   -  and binary - ``'b'``.

-  In our example we have opened the file in read mode and so only
   reading can take place. We will not discuss the binary mode flag, for
   further information please see one of the external references.

-  Once opened the file can be accessed in a variety of ways. Firstly,
   the entire file can be read at once into a string using the
   ``read()`` command,

.. code:: python

   contents = file.read()

but this is only advised for very small files as it is an inefficient
way of processing a file.

-  Secondly, the ``readline()`` can be used to read a single line up to
   a new line character ('\n'). Note that the newline is left at the end
   of the string and the returned string is only empty if the end of the
   file has been reached, e.g.

.. code:: python

   while True:
       line = file.readline()
       if line == "":
           break
       print(line)
   file.close()

-  The final way to read data from a file is to loop in a similar manner
   to looping over a sequence. This is by far the most efficient and has
   the cleanest syntax.

.. code:: python

   for line in file:
       print(line)
   file.close()

-  Note that you should always call the ``close()`` command on the file
   after it has been dealt with to ensure that its resources are freed.

-  In the above examples the lines that have been read from the file
   still contain a newline character as the last character on the line.
   To remove this character use the ``rstrip()`` function with no
   arguments which strips whitespace characters from the right-hand side
   of the string

.. testcode:: read_file1

   # First we will write a file to read in
   import os

   file = open('MyFile.txt', 'w')
   file.write('ID WIDTH THICK HEIGHT\n')
   file.write('a  1.0   2.0   3.0 \n')
   file.write('b  2.0   3.6   1.2 \n')
   file.close()

   # Now read it in
   file = open('MyFile.txt')
   for line in file:
       print(line)
   file.close()

   #Second try

   #Reading agiain, but with rstrip
   file = open('MyFile.txt')
   for line in file:
       line = line.rstrip()
       print(line)
   file.close()

This should give:

.. testoutput:: read_file1
   :options: +NORMALIZE_WHITESPACE

   ID WIDTH THICK HEIGHT
   a  1.0   2.0   3.0
   b  2.0   3.6   1.2

   ID WIDTH THICK HEIGHT
   a  1.0   2.0   3.0
   b  2.0   3.6   1.2

.. testcleanup:: reading_test1

    if os.path.exists('MyFile.txt'):
        os.remove('MyFile.txt')

Writing
=======

-  A string is written to a file using the ``write()`` command once a
   file has been opened in write mode, 'w'. Note that the user controls
   the line formatting and using write does not automatically include a
   new line,

.. testcode:: writing_test1

   import os
   file = open('NewFile.txt', 'w')
   file.write('1 2 3 4 5 6\n')
   file.write('7 8 9 10 11\n')
   file.close()
   file = open('NewFile.txt', 'r')
   print(file.read())


Produces a file with the numbers on 2 separate lines

.. testoutput:: writing_test1
   :options: +NORMALIZE_WHITESPACE

    1 2 3 4 5 6
    7 8 9 10 11

.. testcleanup:: writing_test1

    file = 'NewFile.txt'
    if os.path.exists(file):
        os.remove(file)

`Category:Tested Examples <Category:Tested_Examples>`__
