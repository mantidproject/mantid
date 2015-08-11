:orphan:

.. testcode:: mwTest_Basic_Python_Exercises_4_Solutions[5]

   # Write a program that creates a dictionary and initializes it with 5 names/ID pairs.
   ## Create a function that prints out the dictionary in a nicely formatted table;
   ## Update the dictionary with another 5 name/values and reprint the table,
   ##   making sure you understand the ordering within the map. 
   
   def formatLine(cola, colb, width):
       return cola.center(width) + '|' + colb.center(width)
   
   # A simple two cloumn print out
   def outputStore(store):
       print 'Phonebook contains', len(store),'entries:' 
   
       # Do a quick sweep to find out the longest name
       col_width = 0
       for k in store:
           if len(k) > col_width:
               col_width = len(k)
   
       col_width += 5
       # Header
       print '-'*col_width*2
       print formatLine('Name', 'Ext.', col_width)
       print '-'*col_width*2
       for k ,v in store.iteritems():
           print formatLine(k, str(v), col_width)
   
   phone_book = {'Martyn Gigg' : 1234, 'Joe Bloggs' : 1233, 'Guido Van Rossum' : 4321, \
                 'Bob' : 2314, 'Linus Torvalds' : 4132 }
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

.. testoutput:: mwTest_Basic_Python_Exercises_4_Solutions[5]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   Phonebook contains 5 entries:
   ------------------------------------------
            Name        |         Ext.        
   ------------------------------------------
            Bob         |         2314        
         Joe Bloggs     |         1233        
       Linus Torvalds   |         4132        
      Guido Van Rossum  |         4321        
        Martyn Gigg     |         1234        
   Phonebook contains 9 entries:
   ------------------------------------------
            Name        |         Ext.        
   ------------------------------------------
      Guido Van Rossum  |         4321        
        Martyn Gigg     |         1234        
         Steve Jobs     |         7898        
      Bjarne Strousoup  |         9876        
         Joe Bloggs     |         1233        
       Linus Torvalds   |         4132        
            Dave        |         7098        
         Bill Gates     |         9898        
            Bob         |         9871


