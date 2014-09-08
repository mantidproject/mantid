.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Creates a shape object that defines the sample and sets the sample for
the given workspace. Shapes are defined using XML descriptions that can
be found :ref:`here <HowToDefineGeometricShape>`.

Usage
-----

**Example - A Sphere**  

.. testcode:: Sphere

    ws = CreateSampleWorkspace("Histogram",BankPixelWidth=1)

    shape_xml = '''<sphere id="some-sphere">
      <centre x="0.0"  y="0.0" z="0.0" />
      <radius val="1.0" />
      </sphere>'''
    CreateSampleShape(ws,shape_xml)

**Example - A ball with a cylinder carved out of the middle**  

.. testcode:: BallwithHole

    ws = CreateSampleWorkspace("Histogram",BankPixelWidth=1)

    shape_xml = '''<cylinder id="stick">
      <centre-of-bottom-base x="-0.5" y="0.0" z="0.0" />
      <axis x="1.0" y="0.0" z="0.0" />
      <radius val="0.05" />
      <height val="1.0" />
      </cylinder>
      <sphere id="some-sphere">
      <centre x="0.0"  y="0.0" z="0.0" />
      <radius val="0.5" />
      </sphere>
      <algebra val="some-sphere (# stick)" />'''
    CreateSampleShape(ws,shape_xml)

.. categories::
