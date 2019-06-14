.. _solutions_to_exercises:

======================
Solutions To Exercises 
======================

Exercise 1
==========

-  Write a program that prints out the square of the first 20 integers
   in a block such that the block is a rectangle of size 4x5. (Hint: One
   method uses the ``%`` (modulo) operator to test if a number is a
   multiple of another number. The answer is the remainder of the
   division of the lhs into whole parts, e.g. 4 % 1 = 0, 4 % 2 = 0, 4 %
   3 = 1, 4 % 4 = 0)

.. code:: python

   # Write a program that prints out the square of the first
   # 20 integers a block such that the block has a dimension of 4x5.

   # range(i,j) produces a list of numbers from i -> j-1
   #python 2
   for i in range(1,21):
       print str(i*i).center(3), # center is a function for strings that centers the contents to the given width
       if i % 4 == 0:
           print

   #Python 3
   #for i in range(1,21):
   #    print(str(i*i).center(3),end=' ') # center is a function for strings that centers the contents to the given width
   #    if i % 4 == 0:
   #        print()

   # ------------ Produces -------------
   # 1   4   9   16
   # 25  36  49  64
   # 81 100 121 144
   #169 196 225 256
   #289 324 361 400

Exercise 2
==========

-  Write a program that prints out the first 25 Fibonacci numbers. (The
   Fibonacci sequence starts as with ``0,1`` and next number is the sum
   of the two previous numbers)

   -  Extend the program to also print the ratio of successive numbers
      of the sequence.

.. code:: python

   # prev_2 - One before previous number
   # prev_1 - Previous number
   prev_2, prev_1 = 0, 1

   # Already have first 2 terms above
   print(prev_2)
   print(prev_1)

   # Now the next 23 terms (range(0,23) will run the loop 23 times)
   for i in range(0,23):
       current = prev_2 + prev_1
       # Ratio to previous
       ratio = float(current)/prev_1
       print(str(current) + " ratio to previous= " +str(ratio))
       # Move the previous markers along one for the next time around
       prev_2 = prev_1
       prev_1 = current

Exercise 3
==========

-  Starting with your solution to exercise 2, rewrite your code so that
   you have a function called ``fib`` that accepts a number. The
   function should compute a fibonacci sequence containing this many
   elements and return them as a list.

.. code:: python

   # Write a program that builds a list of the first 20 Fibonacci numbers, then
   ## Use the list to print out the value of the ratio of successive numbers of the sequence,
   ##   printing out the final value.
   ## Extend the program so that the Fibonacci list is calculated in a function that takes the
   ##   number of required values as a parameter and returns the list. 

   # Function to calculate the first n fibonacci numbers
   # and return them as a list
   def fib(nfibs):
       if nfibs == 0:
           return []
       elif nfibs == 1:
           return [0]
       else:
           pass
       # First two numbers
       fibs = [0,1]
       if nfibs == 2:
           return fibs
       for i in range(2, nfibs):
           fibs.append(fibs[i-2] + fibs[i-1])
       return fibs

   #### fib ends here ####

   # Print out successive ratio remembering that the first number is a zero
   nfibs = 20
   fib_nums = fib(nfibs)
   for i in range(1,nfibs):
       try:
           numerator = fib_nums[i]
           denominator = fib_nums[i-1]
           ratio = float(numerator)/denominator
       except ZeroDivisionError:
           print('Warning: Invalid ratio: ' + str(numerator) + '/' + str(denominator))
       else:
           print('Ratio ' + str(numerator) + '/' + str(denominator) + ': ' + str(ratio))

   ##### Produces #####
   #Warning: Invalid ratio: 1/0
   #Ratio 1/1:  1.0
   #Ratio 2/1:  2.0
   #Ratio 3/2:  1.5
   #Ratio 5/3:  1.66666666667
   #Ratio 8/5:  1.6
   #Ratio 13/8:  1.625
   #Ratio 21/13:  1.61538461538
   #Ratio 34/21:  1.61904761905
   #Ratio 55/34:  1.61764705882
   #Ratio 89/55:  1.61818181818
   #Ratio 144/89:  1.61797752809
   #Ratio 233/144:  1.61805555556
   #Ratio 377/233:  1.61802575107
   #Ratio 610/377:  1.61803713528
   #Ratio 987/610:  1.61803278689
   #Ratio 1597/987:  1.61803444782
   #Ratio 2584/1597:  1.6180338134
   #Ratio 4181/2584:  1.61803405573

Exercise 4
==========

-  Write a program that creates a dictionary and initializes it with 5
   names/ID pairs.

   -  Create a function that prints out the dictionary as a 2 columns:
      the first being the key and the second the value
   -  Update the dictionary with another 5 name/values and reprint the
      table, making sure you understand the ordering within the map

.. code:: python

   # Write a program that creates a dictionary and initializes it with 5 names/ID pairs.
   ## Create a function that prints out the dictionary in a nicely formatted table;
   ## Update the dictionary with another 5 name/values and reprint the table,
   ##   making sure you understand the ordering within the map. 

   def formatLine(cola, colb, width):
       return cola.center(width) + '|' + colb.center(width)

   # A simple two cloumn print out
   def outputStore(store):
       print('Phonebook contains {} entries:'.format(len(store))) 

       # Do a quick sweep to find out the longest name
       col_width = 0
       for k in store:
           if len(k) > col_width:
               col_width = len(k)
       col_width += 5

       # Header
       print('-'*col_width*2)
       print(formatLine('Name', 'Ext.', col_width))
       print('-'*col_width*2)
       for k ,v in store.items():
           print(formatLine(k, str(v), col_width))

   phone_book = {'Martyn Gigg' : 1234, 'Joe Bloggs' : 1233, 'Guido Van Rossum' : 4321, 'Bob' : 2314, 'Linus Torvalds' : 4132 }
   outputStore(phone_book)

   # Update Dictionary (replacing one person's phone number
   new_entries = {'Bjarne Strousoup' : 9876, 'Bill Gates' : 9898, 'Steve Jobs' : 7898, \
                 'Bob' : 9871, 'Dave' : 7098 }

   phone_book.update(new_entries)
   outputStore(phone_book)

   #------------- Produces --------------------

   #Phonebook contains 5 entries:
   #------------------------------------------
   #         Name        |         Ext.        
   #------------------------------------------
   #         Bob         |         2314        
   #      Joe Bloggs     |         1233        
   #    Linus Torvalds   |         4132        
   #   Guido Van Rossum  |         4321        
   #     Martyn Gigg     |         1234        
   #Phonebook contains 9 entries:
   #------------------------------------------
   #         Name        |         Ext.        
   #------------------------------------------
   #   Guido Van Rossum  |         4321        
   #     Martyn Gigg     |         1234        
   #      Steve Jobs     |         7898        
   #   Bjarne Strousoup  |         9876        
   #      Joe Bloggs     |         1233        
   #    Linus Torvalds   |         4132        
   #         Dave        |         7098        
   #      Bill Gates     |         9898        
   #         Bob         |         9871   

Exercise 5
==========

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

.. code:: python

   ## Exercise 5
   #   1.  Build a list containing the 5 filenames of the text files that are going to be used.
   #       (Hint: Can be done by hand or using the os.listdir() function in the os module)
   #   2. Add a bogus file name that doesn't exist to the list (so that we have to do some error handling)
   #   3. Loop over the list and for each file (Remember here that we have a non existent file in the list and calling open
   #      on this will result in an IOError exception that needs to be dealt with)
   #         1. Open the file;
   #         2. Loop over each line;
   #         3. Split the line up into sections (Hint: The string has a .split() function that splits the string on
   #            whitespace and gives back a list with each section as an element of the list)
   #         4. Convert the second column value into an integer
   #         5. Keep track of the values for each line and compute an average for the file. 
   #   4. Finally, print out a list of file,average-value pairs 
   import os

   file_dir = "C:\\MantidInstall\\data\\"
   file_names = os.listdir(file_dir)
   file_names.append('nonexistant.txt')
   average_store = {}
   print('Computing average for log files in directory "' + file_dir + '"')
   for name in file_names:
       # Skip all no text files
       if name.endswith('.txt') == False:
           continue
       try:
           file_handle = open(os.path.join(file_dir,name), 'r')
       except IOError:
           print('\tError: No such file: "' + name + '". Skipping file')
           continue
       print('\tReading file',name)
       # At this point we have an open file
       average = 0.0
       nvalues = 0
       line_counter = 1
       for line in file_handle:
           columns = line.split()
           if len(columns) == 2:
               average += float(columns[1])
               nvalues += 1
           else:
               print('\tWarning: Unexpected file format encountered in file {0}  on line {1}'.format(name,line_counter))
           line_counter += 1

       average /= nvalues
       average_store[name] = average
       file_handle.close()

   # Print out file averages
   column_width = 30
   print('')
   print('-'*column_width*2)
   print('File'.center(column_width) + '|' + 'Average'.center(column_width))
   print('-'*column_width*2)
   for key, value in average_store.iteritems():
       print(key.center(column_width) + '|' + str(value).center(column_width))
