:orphan:

.. testsetup:: mwTest_Working_With_Files[5]

   import os
   with open('filename.txt', 'w') as outfile:
      outfile.write("Some Sample Text\nover 2 lines")

.. testcode:: mwTest_Working_With_Files[5]

   file = open('filename.txt', 'r')

.. testcleanup:: mwTest_Working_With_Files[5]

   file.close()
   os.remove('filename.txt')


.. testsetup:: mwTest_Working_With_Files[28]

   import os
   with open('filename.txt', 'w') as outfile:
      outfile.write("Some Sample Text\nover 2 lines")
   file = open('filename.txt', 'r')

.. testcode:: mwTest_Working_With_Files[28]

   contents = file.read()

.. testcleanup:: mwTest_Working_With_Files[28]

   file.close()
   os.remove('filename.txt')


.. testsetup:: mwTest_Working_With_Files[45]

   import os
   with open('filename.txt', 'w') as outfile:
      outfile.write("Some Sample Text\nover 2 lines")
   file = open('filename.txt', 'r')

.. testcode:: mwTest_Working_With_Files[45]

   while True:
       line = file.readline()
       if line == "":
           break
       print line
   file.close()

.. testcleanup:: mwTest_Working_With_Files[45]

   os.remove('filename.txt')

.. testoutput:: mwTest_Working_With_Files[45]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Some Sample Text
   over 2 lines


.. testsetup:: mwTest_Working_With_Files[68]

   import os
   with open('filename.txt', 'w') as outfile:
      outfile.write("Some Sample Text\nover 2 lines")
   file = open('filename.txt', 'r')

.. testcode:: mwTest_Working_With_Files[68]

   for line in file:
       print line
   file.close()

.. testcleanup:: mwTest_Working_With_Files[68]

   os.remove('filename.txt')

.. testoutput:: mwTest_Working_With_Files[68]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Some Sample Text
   over 2 lines


.. testsetup:: mwTest_Working_With_Files[92]

   with open('MyFile.txt', 'w') as outfile:
      outfile.write("ID WIDTH THICK HEIGHT\n")
      outfile.write("a  1.0   2.0   3.0\n")
      outfile.write("b  2.0   3.6   1.2\n")

.. testcode:: mwTest_Working_With_Files[92]

   # MyFile.txt
   #ID WIDTH THICK HEIGHT
   #a  1.0   2.0   3.0
   #b  2.0   3.6   1.2 
   #...
   
   # Python code
   file = open('MyFile.txt')
   for line in file:
       print line
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
       print line
   file.close()
   
   # Prints 
   #ID WIDTH THICK HEIGHT
   #a  1.0   2.0   3.0
   #b  2.0   3.6   1.2

.. testcleanup:: mwTest_Working_With_Files[92]

   import os
   os.remove('MyFile.txt')

.. testoutput:: mwTest_Working_With_Files[92]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   ID WIDTH THICK HEIGHT
   
   a  1.0   2.0   3.0
   
   b  2.0   3.6   1.2
   
   ID WIDTH THICK HEIGHT
   a  1.0   2.0   3.0
   b  2.0   3.6   1.2


.. testcode:: mwTest_Working_With_Files[150]

   file = open('NewFile.txt', 'w')
   file.write('1 2 3 4 5 6\n')
   file.write('7 8 9 10 11\n')
   file.close()
   
   # Produces a file with the numbers on 2 separate lines

.. testcleanup:: mwTest_Working_With_Files[150]

   import os
   os.remove('NewFile.txt')


