.. _basic_python_exercises_5:

=======================
Basic Python Exercises  
=======================

-  This exercise aims to perform a moderately complicated set of
   operations using most of the topics covered over the course.

-  For this exercise you will require 5 text files, each containing 2
   columns of data: a timestamp and a value.

Exercise
========

#. Build a list containing the 5 filenames of the text files that are
   going to be used. (Hint: Can be done by hand or using the
   os.listdir('dirpath') function in the os module)
#. Add a bogus file name that doesn't exist to the list (so that we have
   to do some error handling)
#. Loop over the list and for each file (Remember here that we have a
   non existent file in the list and calling open on this will result in
   an IOError exception that needs to be dealt with)

   #. Open the file;
   #. Loop over each line;
   #. Split the line up into sections (Hint: The string has a
      ``.split()`` function that splits the string on whitespace and
      gives back a list with each section as an element of the list)
   #. Convert the second column value into an float
   #. Keep track of the values for each line and compute an average for
      the file.

#. Finally, print out a list of file,average-value pairs
