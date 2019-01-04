.. _sequence_data_types:

===================
Sequence Data Types 
===================

-  Python supports a range of types to store sequences. Here we will
   explore lists, sets, tuples and dictionaries. The string type is also
   considered a sequence but for our purposes here we shall consider it
   as a simple list of characters.

-  A list is essentially an ordered collection of elements where the
   ordering is defined by the creator.

-  Lists are created using square brackets to enclose the sequence of
   elements

-  Elements can be added using the ``append()`` function

-  Access to a specific element is done again by the square bracket
   operator by providing the required index of the element. **Note that
   in Python the first index is 0**

-  Removing an element can be done by the ``del`` command or the
   ``.remove()`` function

-  Replacement is also done using the square-bracket operator

.. testcode:: lists1

   lottery_numbers = [1,2,3,4,5,6]

   bonus = 7
   lottery_numbers.append(bonus)

   # print the first element
   print(lottery_numbers[0])  
   # print the last element
   print(lottery_numbers[6])  
   print(lottery_numbers) 
   lottery_numbers.remove(5) # Removes first occurrence of value 5 in list
   del lottery_numbers[0]  # Removes the 0th element of the list
   print(lottery_numbers) 

   lottery_numbers[3] = 42
   print(lottery_numbers)

Givest the output:

.. testoutput:: lists1

    1
    7
    [1, 2, 3, 4, 5, 6, 7]
    [2, 3, 4, 6, 7]
    [2, 3, 4, 42, 7]

-  The square bracket operator also provides an operation known as
   *slicing*.

-  Slicing allows contiguous portions of lists to be sectioned out by
   using ``[i:j]`` syntax where i and j are indexes.

-  In this case it is helpful to think of the indices of the sequence
   slightly differently. Instead of thinking of each index as being
   assigned to a specific element within the list, think of them as
   being assigned to the boundaries of the elements, e.g.

.. figure:: /images/Pyslice.png
   :alt: Pyslice.png

-  The output of a slice operation is then much more obvious as it
   simple slices out the items within the boxes contained by the given
   range.

-  This also works with strings

.. testcode:: slice1

   my_list = ['M','A', 'N', 'T', 'I', 'D']
   print(my_list[1:4])  

   my_string = 'MANTID'
   print(my_string[1:4])  

Gives the output:

.. testoutput:: slice1

    ['A', 'N', 'T']
    ANT

-  Lists can be sorted using the ``sort()`` function which modifies the
   list in place.

.. testcode:: sort1

   my_list = [5,4,3,2,7]
   print(my_list)   
   my_list.sort()
   print(my_list)  

Gives the output:

.. testoutput:: sort1

    [5, 4, 3, 2, 7]
    [2, 3, 4, 5, 7]

-  The default sorting criterion is less-than where items lower in the
   list are "less-than" items higher in the list. You can reverse this
   with,

.. testcode:: sort2

   l = [5,4,3,2,7]
   l.sort(reverse=True)
   print(l)  #prints list in descending order 

Gives the output:

.. testoutput:: sort2

    [7, 5, 4, 3, 2]

`Category:Tested Examples <Category:Tested_Examples>`__
