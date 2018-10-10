========
Python 3
========

Python 2 has an `end-of-life date set for 2020 <http://legacy.python.org/dev/peps/pep-0373/>`_
and now that most third-party packages support both 2 and 3 we are starting to think about a
migration strategy for Mantid.

.. contents::
  :local:

Building Against Python 3
#########################

This is currently only possible on a Linux system with a pre-installed version of python 3. You need
to install some additional packages as shown below:

.. code-block:: sh

   apt-get install python3-sip-dev python3-pyqt4  python3-numpy  python3-scipy  python3-sphinx \
     python3-sphinx-bootstrap-theme  python3-dateutil python3-matplotlib ipython3-qtconsole \
     python3-h5py python3-yaml

or on fedora, with slightly different package names

.. code-block:: sh

   dnf install python3-sip-devel python3-PyQt4-devel python3-numpy python3-scipy python3-sphinx \
     python3-sphinx-theme-bootstrap python3-dateutil python3-matplotlib python3-ipython-gui \
     boost-python3-devel python3-h5py python3-yaml

then set ``-DPYTHON_EXECUTABLE=/usr/bin/python3`` when running cmake before building.

.. warning::

    If any of these packages are installed via pip, this could cause conflicts.
    Install as described here only.

Setting up the anaconda environment for developer (tested on ubuntu 18.04).

1. Creating environment and activating:

.. code-block:: sh

    conda create -n env3.5 python=3.5
    source activate env3.5

2. Installing needed packages:

.. code-block:: sh

    conda install numpy
    conda install pyqt=4.11
    conda install -c conda-forge qscintilla2
    conda install matplotlib
    pip install -U numpy

3. Build mantid with correct python:

.. code-block:: sh

    mkdir <your_path>/build
    cd <your_path>/build
    cmake -DPYTHON_EXECUTABLE=<anaconda_home>/bin/python3.5 -DCMAKE_BUILD_TYPE=Release
    make

4. Install and launch jupyter lab:

.. code-block:: sh

    conda install jupyterlab
    jupyter lab

Inside the jupyter you have to import API:

.. code-block:: python

    import sys
    sys.path.append('<your_path/build/bin>')
    from mantid.simpleapi import *
    import mantid


Supporting Python 2 and 3
#########################

Python 3 introduces many exciting new features. For a full description see the official Python 3
changes document. For a shorter overview see
`here <https://asmeurer.github.io/python3-presentation/slides.html#1>`__ or
`here <http://python3porting.com/differences.html>`__.

Some features of Python 3 have been backported to Python 2.x within the
`__future__ <https://docs.python.org/2.7/library/__future__.html?highlight=future#module-__future__>`_
module. These make it easier to write code that is compatible with both versions.

This cheat sheet provides helpful examples of how to write code in a 2/3 compatible manner. Where an
option is given to use either the `six <https://pythonhosted.org/six/>`_ or
`future <https://pypi.python.org/pypi/future>`_ (not to be confused with ``__future__``!) modules
then ``six`` is used.

All new code should be written to be compatible with Python 2 & 3 and as a minimum the first import
line of the module should be:

.. code-block:: python

   from __future__ import (absolute_import, division, print_function)

It is quite common to also see ``unicode_literals`` in the above import list, however, when running
under Python 2 ``Boost.Python`` will not automatically convert a Python ``str`` to C++ ``std::string``
automatically if the string is unicode. When running with Python 3 ``Boost.Python`` will do this
conversion automatically for unicode strings so this is in fact not a huge issue going forward.

Migrating From Python 2 to 3
############################

One way to migrate a file from python 2 to 3 is as follows...

.. warning::
  | To perform the following procedure on windows:
  | 1. Git Bash or similar will be required.
  | 2. To run the ``2to3`` script you will need to start the command-prompt.bat in the build directory and run ``%PYTHONHOME%\Scripts\2to3``

Run the following script to run the python 2 to 3 translation tool and rename the file to ``filename.py.bak``

.. code-block:: sh

   2to3 --no-diffs -w filename.py
   mv filename.py{,.bak};


Run **one** of the following commands to append the import statement listed above.

.. code-block:: sh

  awk '/(from|import)/ && !x {print "from __future__ import (absolute_import, division, print_function)\n"; x=1} 1' \
      filename.py.bak > filename.py

**or**

.. code-block:: sh

  sed -i '0,/^import\|from.*/s/^import\|from.*/from __future__ import (absolute_import, division, print_function)\n&/' filename.py

Check each changed block,

- If any change has replaced ``xrange`` with ``range`` then add ``from six.moves import range``
  to the imports list
- If any change has replaced ``ifilterfalse`` with ``filterfalse`` from ``itertools`` then replace a
  statement like ``from itertools import filterfalse`` with ``from six.moves import filterfalse`` in the
  imports list. There are more cases like this documented `here <https://pythonhosted.org/six/#module-six.moves>`_.
- If any change has replaced ``for k, v in knights.iteritems()`` with ``for k, v in knights.items()``
  then add ``from six import iteritems`` to the import list and update the replacement to
  ``for k, v in iteritems(knights)``.

In some cases like ``range``, pylint will complain about `Replacing builtin 'range'` or similar.
Make sure to put the proper ignore statement on that line using ``#pylint: disable=redefined-builtin``.

Check the code still runs as expected in Python 2.

.. note::
   ``2to3`` will try to keep the type of the objects the same. So, for example ``range(5)`` will
   become ``list(range(5))``. This is not necessary if you use it just for iteration. Things like
   ``for i in range(5)`` will work in both versions of Python, you don't need to transform it into a
   list.
