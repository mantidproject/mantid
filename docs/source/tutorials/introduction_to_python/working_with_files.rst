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

.. code:: python

   # MyFile.txt
   #ID WIDTH THICK HEIGHT
   #a  1.0   2.0   3.0
   #b  2.0   3.6   1.2 
   #...

   # Python code
   file = open('MyFile.txt')
   for line in file:
       print(line)
   file.close()
   # Prints 
   #ID WIDTH THICK HEIGHT
   #
   #a  1.0   2.0   3.0
   #
   #b  2.0   3.6   1.2
   #

   #Second try

   file = open('MyFile.txt')
   for line in file:
       line = line.rstrip()
       print(line)
   file.close()

   # Prints 
   #ID WIDTH THICK HEIGHT
   #a  1.0   2.0   3.0
   #b  2.0   3.6   1.2

Writing
=======

-  A string is written to a file using the ``write()`` command once a
   file has been opened in write mode, 'w'. Note that the user controls
   the line formatting and using write does not automatically include a
   new line,

.. code:: python

   file = open('NewFile.txt', 'w')
   file.write('1 2 3 4 5 6\n')
   file.write('7 8 9 10 11\n')
   file.close()

   # Produces a file with the numbers on 2 separate lines

`Category:Tested Examples <Category:Tested_Examples>`__
