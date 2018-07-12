#!/usr/bin/env python
from __future__ import (absolute_import, division, print_function, unicode_literals)
import os
import sys

DOCS = {
    'index.rst':'''.. _v{version}:

===========================
Mantid {version} Release Notes
===========================

.. figure:: ../../images/ImageNotFound.png
   :class: screenshot
   :width: 385px
   :align: right

.. contents:: Table of Contents
   :local:

.. warning:: This release is still under construction. The changes can be found in the nightly builds on the `download page`_.

We are proud to announce version {version} of Mantid. This release
celebrates 10 years of the Mantid project supporting Neutron and Muon
research.

**TODO: Add paragraph summarizing big changes**

This is just one of many improvements in this release, so please take a
look at the release notes, which are filled with details of the
important changes and improvements in many areas. The development team
has put a great effort into making all of these improvements within
Mantid, and we would like to thank all of our beta testers for their
time and effort helping us to make this another reliable version of Mantid.

Throughout the Mantid project we put a lot of effort into ensuring
Mantid is a robust and reliable product. Thank you to everyone that has
reported any issues to us. Please keep on reporting any problems you
have, or crashes that occur on our `forum`_.

Installation packages can be found on our `download page`_
which now links to sourceforge to mirror our download files around the world, you can also
access the source code on `GitHub release page`_.

Citation
--------

Please cite any usage of Mantid as follows: **TODO update with current version doi**

- *Mantid {version}: Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project*. `doi: 10.5286/SOFTWARE/MANTID{version}
   <http://dx.doi.org/10.5286/SOFTWARE/MANTID{version}>`_

- Arnold, O. et al. *Mantid-Data Analysis and Visualization Package for Neutron Scattering and mu-SR Experiments.* Nuclear Instruments
  and Methods in Physics Research Section A: Accelerators, Spectrometers, Detectors and Associated Equipment 764 (2014): 156-166
  `doi: 10.1016/j.nima.2014.07.029 <https://doi.org/10.1016/j.nima.2014.07.029>`_
  (`download bibtex <https://raw.githubusercontent.com/mantidproject/mantid/master/docs/source/mantid.bib>`_)


Changes
-------

.. toctree::
   :titlesonly:

   Framework <framework>
   User Interface & Usability <ui>
   Diffraction <diffraction>
   LowQ <lowq>
   Muon Analysis <muon>
   Spectroscopy <spectroscopy>

Full Change Listings
--------------------

For a full list of all issues addressed during this release please see the `GitHub milestone`_.

.. _download page: http://download.mantidproject.org

.. _forum: http://forum.mantidproject.org

.. _GitHub milestone: http://github.com/mantidproject/mantid/pulls?utf8=%E2%9C%93&q=is%3Apr+milestone%3A{milestone}+is%3Amerged

.. _GitHub release page: https://github.com/mantidproject/mantid/releases/tag/v{version}
''',
    'framework.rst':'''=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Concepts
--------

Algorithms
----------

Data Objects
------------

Python
------
''',
    'ui.rst':'''======================
UI & Usability Changes
======================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Installation
------------

Workbench
---------

SliceViewer and Vates Simple Interface
--------------------------------------
''',
    }

################################################################################
################################################################################

TECH_DOCS = {
    'diffraction.rst':('Diffraction Changes', '''
.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Powder Diffraction
------------------

Engineering Diffraction
-----------------------

Single Crystal Diffraction
--------------------------

Imaging
-------
'''),

    'spectroscopy.rst':('Spectroscopy Changes', '''
.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Direct Geometry
---------------

Indirect Geometry
-----------------
'''),

    'muon.rst':('MuSR Changes', '''
.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

'''),

    'lowq.rst':('Low-Q Changes', '''
.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Reflectometry
-------------

Small Angle Scattering
----------------------
'''),
    }

TECH_HEAD = '''{divider}
{technique}
{divider}
'''

TECH_CONTENTS = '''
.. contents:: Table of Contents
   :local:
'''


def createTechniquePage(technique, body, components):
    '''
    @param technique is the name of the page
    @param body is the contents of the page
    @param components is list of component names in github to link to
    '''
    technique = technique.strip()
    content = TECH_HEAD.format(divider=('=' * len(technique)), technique=technique)
    content += TECH_CONTENTS + body
    return content


def getReleaseRoot():
    script_dir = os.path.split(sys.argv[0])[0]
    return os.path.abspath(os.path.join(script_dir, '../../docs/source/release/'))


def fixReleaseName(name):
    if name.startswith('v'):
        name = name[1:]

    # make sure that all of the parts can be converted to integers
    try:
        version = [int(i) for i in name.split('.')]
    except ValueError as e:
        raise RuntimeError('expected version number form: major.minor.patch', e)
    if len(version) == 3:
        pass # perfect
    elif len(version) == 2:
        name += '.0'
    elif len(version) == 1:
        name += '.0.0'
    else:
        raise RuntimeError('expected version number form: major.minor.patch')

    return 'v' + name


def toMilestoneName(version):
    version = version[1:].split('.')
    version = '"Release+{major}.{minor}"'.format(major=version[0], minor=version[1])
    return version


def addToReleaseList(release_root, version):
    filename = os.path.join(release_root, 'index.rst')
    newversion = '   %s <%s/index>\n' % (version, version)

    # read in the entire old version
    with open(filename, 'r') as handle:
        oldtext = handle.readlines()

    # write out the new version
    with open(filename, 'w') as handle:
        search_for_insertion = True
        for i in range(len(oldtext)):
            line = oldtext[i].strip()
            if search_for_insertion and line.startswith('v') and line.endswith('/index>'):
                if version not in line:
                    handle.write(newversion)
                search_for_insertion = False
            handle.write(oldtext[i])


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser(description="Generate generic release pages")
    parser.add_argument('--release', required=True)
    parser.add_argument('--milestone', required=False, default=None,
                        help="Formatted with html escapes already")
    args=parser.parse_args()

    # parse, repair, and create missing arguments
    args.release = fixReleaseName(args.release)
    print('  release:', args.release)
    if args.milestone is None:
        args.milestone = toMilestoneName(args.release)
    print('milestone:', args.milestone)
    release_root = getReleaseRoot()
    print('     root:', release_root)

    # add the new sub-site to the index
    addToReleaseList(release_root, args.release)

    # create all of the sub-area pages
    release_root = os.path.join(release_root, args.release)
    if not os.path.exists(release_root):
        print('creating directory', release_root)
        os.makedirs(release_root)

    release_link = '\n:ref:`Release {0} <{1}>`'.format(args.release[1:], args.release)

    for filename in DOCS.keys():
        contents = DOCS[filename].format(milestone=args.milestone, version=args.release[1:])
        filename = os.path.join(release_root, filename)
        print('making', filename)
        with open(filename, 'w') as handle:
            handle.write(contents)
            if 'index.rst' not in filename:
                handle.write(release_link)

    for filename in TECH_DOCS.keys():
        name, contents = TECH_DOCS[filename]
        contents = contents.format(milestone=args.milestone, version=args.release[1:])
        filename = os.path.join(release_root, filename)
        print('making', filename)
        with open(filename, 'w') as handle:
            handle.write(createTechniquePage(name, contents, [1,2,3]))
            handle.write(release_link)
