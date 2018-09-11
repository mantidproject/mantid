.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a very simple algorithm that does nothing for a given number of
seconds.

This can be used during debugging, for example, to slow down the
execution of a fast script.


Usage
-----

**Example - Pausing for a time:**  

.. testcode:: ExPauseString

   import time
	
   start_time = time.clock()
   Pause(0.05)
   end_time = time.clock()
   print("The algorithm paused for {:.2f} seconds.".format(end_time-start_time))
	
Output:

.. testoutput:: ExPauseString
   :options: +ELLIPSIS
   
   The algorithm paused for ... seconds.

.. categories::

.. sourcelink::
