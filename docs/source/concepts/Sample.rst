.. _Sample:

======
Sample
======


.. contents::
  :local:

What is the Sample object?
--------------------------

The sample object holds details of the samples in an experiment.
While most of the time this will refer to a single sample, it can also describe a collection of samples.
Specifically this holds information about a samples:

* Material properties and chemical composition
* Shape and dimensions
* Crystal structure

Working with Sample objects in Python
-------------------------------------

A full list of properties and operations can be found in :ref:`Sample API reference <mantid.api.Sample>`, but here are some of the key ones.

Getting the Sample Object from a Workspace
##########################################

.. testcode:: WorkspaceSample

  ws=CreateWorkspace(DataX='1,2',DataY='1')
  s=ws.sample()


Sample Properties
#################

.. testcode:: SamplePropertiestest

  ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)
  s = ws.sample()

  # Dimensions
  s.setHeight(0.1)
  s.setWidth(0.2)
  s.setThickness(0.3)
  print("Height: " + str(s.getHeight()))
  print("Width: " + str(s.getWidth()))
  print("Thickness: " + str(s.getThickness()))

  # Material
  SetSampleMaterial(ws,ChemicalFormula="V")
  m = s.getMaterial()
  print("Material Name " + str(m.name()))
  print("Total scattering X-Section: " + str(m.totalScatterXSection()))

  # Crystal Structure
  SetUB(ws,a=2,b=3,c=4,alpha=90,beta=90,gamma=90,u="1,0,0",v="0,0,1")
  ol=s.getOrientedLattice()
  print("Data lattice parameters are: {0} {1} {2} {3} {4} {5} ".format(ol.a(),ol.b(),ol.c(),ol.alpha(),ol.beta(),ol.gamma()))

.. testoutput:: SamplePropertiestest
  :hide:
  :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

  Height: 0.1
  Width: 0.2
  Thickness: 0.3
  Material Name V
  Total scattering X-Section: 5.1
  Data lattice parameters are: 2.0 3.0 4.0 90.0 90.0 90.0


Multiple Samples
################

The ``Sample()`` method actually returns a collection, however if you do not specify which sample you want you will get the first member of the collection.

.. testcode:: MultiSample

  ws = CreateSampleWorkspace("Histogram",NumBanks=1,BankPixelWidth=1)

  s = ws.sample()
  # Is the same as
  s = ws.sample()[0]

  # You can ask how many samples there are with
  size = ws.sample().size()


.. categories:: Concepts
