.. _06_settings:

========
Settings
========

Mantid's main configuration occurs through properties files. There are two (three on Linux) that allow customisation of Mantid (read in this order):

* Mantid.properties (found in the bin directory) - Default settings, replaced with each version of Mantid
* Mantid.system.properties (linux only) - System-wide settings for multi-user systems.
* Mantid.user.properties (found in the bin directory on Windows & in $HOME/.mantid on Linux) - User settings that will never be touched by a later version of Mantid.

The settings within these files occur as key-value pairs, i.e the default instrument is specified as

.. code-block:: python

	default.instrument = INST_NAME
	# Note, this is not a Python command

where INST_NAME is the name of the default instrument, for example 'LARMOR'.


All settings specified in these files are accessible via python through the config object, which can be imported from the mantid module. For example, to access the default instrument:

.. code-block:: python

	from mantid import config

	default_inst_name = config['default.instrument']

The config object behaves like a Python dictionary, where each setting in the properties file is used as a key to obtain its value from the config object.


The dictionary-like manner can also be used to update settings, e.g.

.. code-block:: python

	from mantid import config

	config['default.instrument'] = 'INST_NAME'

where the value on the RHS must be a string. NOTE: Settings changed in this manner only affect the current Mantid session


Data search directories
=======================

Some keys in the properties are considered special and have their own accessors. For example, the datasearch.directories key is a semi-colon separated list of search directories. This is cumbersome to use as a string and various methods exist to manipulate the search paths more easily

.. code-block:: python

	from mantid import config

	# Get a list of the search paths
	current_paths = config.getDataSearchDirs()

	# Add a new directory to the list
	config.appendDataSearchDir('new-dir')

	# Beware, the following commands will change your data search path
	# After that you may not be able to find your training data with Load()

	#Avoids dependency on the key name
	config.setDataSearchDirs('path1;path2;path3')

	# Set it via a list
	config.setDataSearchDirs(['path1','path2'])


Facility & Instrument Information
=================================

Mantid has another config file called facilities.xml that is found in the bin directory of the installation. This file provides Mantid with information regarding a facility & its instruments. The types of information provided are:
a list of instruments at the facility along with techniques that these instruments support;
a default zero-padding for the run files at the facility, which each instrument declaration can override;
a default list of extensions for files considered as data from an instrument;
the data catalog in use at the facility;
the archive search plugin used to access the data archive.

All of these properties can be accessed via Python:

.. code-block:: python

	from mantid import config

	facility = config.getFacility('SNS')  # or config.getFacility() returns the current default
	inst_info = config.getInstrument('CNCS') # or config.Instrument() returns the current default

The list of methods available are shown in the python reference: :ref:`InstrumentInfo`
