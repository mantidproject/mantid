.. _HistogramData:

======================================
Transition to the HistogramData module
======================================

.. contents::
  :local:

HistogramData is a new Mantid module dealing with histograms.
This is *not* a replacement for ``Histogram1D`` or ``ISpectrum``, but rather deals with the *histogram* part of the data stored in those classes.
It is not part of the main branch yet, so the information on this page does not apply to the distributed Mantid binaries.
This document is intended for Mantid developers.
It informs about the transition to the new way of dealing with histograms, and explains the new interfaces.

Instroduction
-------------

Motivation
##########

- Make working with histograms **easier**.
- Provide a **safer** way of working with histograms.
- Details can be found in the `design document <https://github.com/mantidproject/documents/blob/master/Design/histogram_type.md>`_.

Concepts
########

.. figure:: ../images/Histogram.png
   :figwidth: 50%
   :align: center

   The histogram concept and nomenclature in Mantid.
   The expressions commonly used in Mantid to refer to these concepts are given in quotation marks.
   Details can be found in the text below.
   The CamelCase keywords are new types and will be introduced later.

Histogram data in Mantid is stored in a number of different ways that reflect the way it was produced, or the way algorithms deal with the data:

- The x-data (typically corresponding to TOF or units derived from TOF) can represent bin edges (1 more than y-values), or points (1 for each y-value).
  Conversions between the two cases are done with the algorithms ``ConvertToPointData`` and ``ConvertToHistogram``.
- The y-data can be counts or counts divided by bin-width.
  In the latter case a workspace is said to contain *distribution data*.
  Conversions between the two cases are done with the algorithms ``ConvertToDistribution`` and ``ConvertFromDistribution``.
- The e-data (uncertainties) follows the same distinction as y-data.
  It typically stores the standard-deviation, but is sometimes (internally and temporarily) converted to a variance, i.e., the square of the standard-deviation.

The new histogram type described below is designed to follow the needs of the current way Mantid deals with histograms (the following description is mainly for x-data, but the same applies to y-data):

1. Some algorithms work with bin edges, some with points, and many do not care.
2. We need to share data, e.g., when the bin edges of all histograms in a workspace are identical. This is currently handled in ``ISpectrum``, ``Histogram1D``, and ``EventList``, which store a copy-on-write pointer (``Kernel::cow_ptr``) to a ``std::vector<double>``.

Mantid developers are all familiar with these two facts:

1. Algorithms that "do not care" access the data and simply work with it. All other algorithms attempt to determine whether the x-data corresponds bin edges or points by comparing its length to that of the y-data (or alternatively use ``MatrixWorkspace::isHistogramData()``). Based on the result they may convert, e.g., bin edges to points.
2. The copy-on-write mechanism is reflected in the interface of ``ISpectrum`` and ``MatrixWorkspace``:

  .. code-block:: c++
    :linenos:

    using MantidVec = std::vector<double>;
    using MantidVecPtr = Kernel::cow_ptr<std::vector<double>>;

    // Current ISpectrum interface:
    void setX(const MantidVec &X);
    void setX(const MantidVecPtr &X);
    void setX(const MantidVecPtr::ptr_type &X);
    MantidVec &dataX();
    const MantidVec &dataX() const;
    const MantidVec &readX() const;
    MantidVecPtr ptrX() const; // renamed to refX() in MatrixWorkspace

  - Non-``const`` access to data in a spectrum will trigger a copy.
  - To avoid triggering the copy we use ``readX()``.
  - When we want to share data we use ``setX()`` with a ``shared_ptr`` or ``cow_ptr`` (can be obtained with ``ptrX()`` or ``refX()``).

The shortcomings of the current implementation are mostly obvious and can also be found in the `design document <https://github.com/mantidproject/documents/blob/master/Design/histogram_type.md>`_.

The new ``Histogram`` type
--------------------------

Currently the implementation is incomplete: ``Histogram`` contains only the x-data, whereas all the others (y-data, e-data, x-uncertainties) have not been transitioned yet.

In its final form, we will be able to do things like the following:

.. code-block:: c++
  :linenos:

  BinEdges edges{1.0, 2.0, 4.0};
  Counts counts1{4, 100, 4};
  Counts counts2{0, 100, 0};
  Histogram histogram1(edges, counts1);
  Histogram histogram2(edges, counts2);
  // x-data in histogram1 and histogram2 is shared

  // Uncertainties are auto-generated, unless specified explicitly
  auto errors = histogram1.countStandardDeviations();
  errors[0]; // 2.0
  errors[1]; // 10.0
  errors[2]; // 2.0

  // Arithmetics with histograms
  histogram1 += histogram2; // Checks size, throws if mismatched!
  auto counts = histogram1.counts();
  counts[0]; // 4.0
  counts[1]; // 200.0
  counts[2]; // 4.0
  // Deals with errors as well!
  errors = histogram1.countStandardDeviations();
  errors[0]; // 2.0
  errors[1]; // sqrt(200.0)
  errors[2]; // 2.0

  // Need bin centers (points data) instead of bin edges?
  auto points = histogram.points();
  // Need variance instead of standard deviation?
  auto variances = histogram.countVariances();
  // Need frequencies (distribution data) instead of counts?
  auto frequencies = histogram.frequencies();
  auto variances = histogram.frequencyVariances();

  // Type-safe operations
  CountStandardDeviations sigmas{0.1, 0.1};
  histogram.setCountVariances(sigmas); // Ok, squares internally
  sigmas += CountVariances{0.01, 0.01}; // Ok, takes sqrt before adding
  sigmas[0]; // 0.2
  sigmas[1]; // 0.2

Further planned features:

- Arithmetics will all sub-types (``BinEdges``, ``Points``, ``Counts``, and ``Frequencies``, and also their respective ``Variances`` and ``StandardDeviations``).
- Rebinning.
- Generating bin edges (linear, logarithmic, ...).
- Extend the ``Histogram`` interface with more common operations.
- Non-member functions for more complex operations on histogram.

**Any feedback on additional capabilities of the new data types is highly appreciated.
I will happily consider adding more features as long as they fit the overall design.
Please get in** `contact <mailto:simon.heybrock@esss.se>`_ **with me!**

The plan is to merge the changes for the x-data right after the next release.
I will then work on reducing the use of the old interface (``readX()``, ``dataX()``, ...) as much as possible and also roll out changes for x-uncertainties. After that, changes for y-data and y-uncertainties will follow.

We also want to expose most parts of the ``HistogramData`` module to Python, but no schedule has been decided yet.
Parts of the old interface will be kept alive for now, in particular to maintain support for the old Python interface.

``Histogram``
#############

Contains a copy-on-write pointer (``cow_ptr``) to the x-data. The interface gives access to the data as well as the pointer:

.. code-block:: c++
  :linenos:

  class Histogram {
  public:
    // ...
    // Replacement for readX() and dataX() const
    const HistogramX &x() const;
    // Replacement for dataX()
    HistogramX &mutableX();

    // Replacement for refX()
    Kernel::cow_ptr<HistogramX> sharedX() const;
    // Replacement for setX()
    void setSharedX(const Kernel::cow_ptr<HistogramX> &X);
  };

``HistogramX``
##############

- The current fundamental type for x-data, ``std::vector<double>``, is replaced by ``HistogramX``.
- Internally this is also a ``std::vector<double>`` and the interface is almost identical.
- However, it does not allow for size modifications, since that could bring a histogram into an inconsistent state, e.g., by resizing the x-data without also resizing the y-data.

``BinEdges``
############

- For algorithms that work with bin edges, ``Histogram`` provides an interface for accessing and modifying the x-data as if it were stored as bin edges:

  .. code-block:: c++
    :linenos:
  
    class Histogram {
    public:
      // Returns by value!
      BinEdges binEdges() const;
      // Accepts any arguments that can be used to construct BinEdges
      template <typename... T> void setBinEdges(T &&... data);
    };

- If the histogram stores point data, ``Histogram::binEdges()`` will automatically compute the bin edges from the points.
- ``BinEdges`` contains a ``cow_ptr`` to ``HistogramX``. If the histogram stores bin edges, the ``BinEdges`` object returned by ``Histogram::binEdges()`` references the same ``HistogramX``, i.e., there is no expensive copy involved.
- Setting the same ``BinEdges`` object on several histograms will share the underlying data.
- ``Histogram::setBinEdges()`` includes a size check and throws if the histogram is incompatible with the size defined by the method arguments.

``Points``
##########

- For algorithms that work with points, ``Histogram`` provides an interface for accessing and modifying the x-data as if it were stored as points:

  .. code-block:: c++
    :linenos:
  
    class Histogram {
    public:
      // Returns by value!
      Points points() const;
      // Accepts any arguments that can be used to construct Points
      template <typename... T> void setPoints(T && ... data);
    };

- If the histogram stores bin edges, ``Histogram::points()`` will automatically compute the points from the bin edges.
- ``Points`` contains a ``cow_ptr`` to ``HistogramX``. If the histogram stores points, the ``Points`` object returned by ``Histogram::points()`` references the same ``HistogramX``, i.e., there is no expensive copy involved.
- Setting the same ``Points`` object on several histograms will share the underlying data.
- ``Histogram::setPoints()`` includes a size check and throws if the histogram is incompatible with the size defined by the method arguments.

Code examples
#############

All new classes and functions described here are part of the module ``HistogramData``. The following code examples assume ``using namespace HistogramData;``.

Working with bin edges
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c++
  :linenos:

  /////////////////////////////////////////////////////
  // Construct like std::vector<double>:
  /////////////////////////////////////////////////////
  BinEdges edges(length); // initialized to 0.0
  BinEdges edges(length, 42.0);
  BinEdges edges{0.1, 0.2, 0.4, 0.8};
  std::vector<double> data(...);
  BinEdges edges(data);
  BinEdges edges(data.begin() + 1, data.end());

  /////////////////////////////////////////////////////
  // Iterators:
  /////////////////////////////////////////////////////
  BinEdges edges = {1.0, 2.0, 4.0};
  if(edges.cbegin() != edges.cend())
    *(edges.begin()) += 2.0;

  /////////////////////////////////////////////////////
  // Index operator:
  /////////////////////////////////////////////////////
  BinEdges edges = {1.0, 2.0, 4.0};
  edges[0]; // 1.0
  edges[1]; // 2.0
  edges[2]; // 4.0

  // Only const! This is not possible:
  edges[0] += 2.0; // DOES NOT COMPILE

  // REASON: BinEdges contains a copy-on-write pointer to data, dereferencing in
  // tight loop is expensive, so interface prevents things like this:
  for (size_t i = 0; i < edges.size(); ++i)
    edges[i] += 0.1; // does not compile

  // If you need write access via index, use:
  auto x = edges.mutableData(); // works similar to current dataX()
  for (size_t i = 0; i < x.size(); ++i)
    x[i] += 0.1;

  // Better (for simple cases):
  edges += 0.1;

Working with points
~~~~~~~~~~~~~~~~~~~

.. code-block:: c++
  :linenos:

  // Works identically to BinEdges
  Points points(length);
  // ...

  // Type safe!
  BinEdges edges(...);
  points = edges; // DOES NOT COMPILE

  // Can convert
  points = Points(edges); // Points are defined as mid-points between edges
  edges = BinEdges(points); // Note that this is lossy, see ConvertToHistogram

Working with histograms
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c++
  :linenos:

  /////////////////////////////////////////////////////
  // Construct Histogram:
  /////////////////////////////////////////////////////
  // Note that this currently only takes arguments for the
  // x-data, but will be extended later, e.g.,
  // Histogram histogram(BinEdges(length+1), Counts(length));
  Histogram histogram(BinEdges{0.1, 0.2, 0.4});
  histogram.xMode(); // returns Histogram::XMode::BinEdges

  /////////////////////////////////////////////////////
  // Assignment:
  /////////////////////////////////////////////////////
  histogram2 = histogram1; // Data is automatically shared

  /////////////////////////////////////////////////////
  // Basic access:
  /////////////////////////////////////////////////////
  auto edges = histogram.binEdges(); // size 3, references Histogram::x()
  auto points = histogram.points(); // size 2, computed on the fly
  points[0]; // 0.15
  points[1]; // 0.3
  const auto &x = histogram.x(); // size 3
  auto &x = histogram.mutableX(); // size 3

  /////////////////////////////////////////////////////
  // Modify bin edges:
  /////////////////////////////////////////////////////
  auto edges = histogram.binEdges();
  edges[1] += 0.1;
  histogram.setBinEdges(edges);

  /////////////////////////////////////////////////////
  // Outlook (not implemented yet):
  /////////////////////////////////////////////////////
  histogram2 += histogram1; // Checks for compatible x, adds y and e

  /////////////////////////////////////////////////////
  // Side remark -- bin edges and points:
  /////////////////////////////////////////////////////
  Histogram histogram(BinEdges{0.1, 0.2, 0.4});
  histogram.xMode(); // returns Histogram::XMode::BinEdges
  auto edges = histogram.binEdges(); // size 3, references Histogram::x()
  auto points = histogram.points(); // size 2, computed on the fly
  // XMode::BinEdges, size 3 is compatible with Points of size 2, so:
  histogram.setPoints(points); // size check passes
  histogram.xMode(); // returns Histogram::XMode::Points
  edges = histogram.binEdges(); // size 3, computed on the fly
  points = histogram.points(); // size 2, references Histogram::x()
  // Note that edges is now different from its initial values (same
  // behavior as ConvertToPointData followed by ConvertToHistogram).




Legacy interface
----------------

For compatibility reasons an interface to the internal data, equivalent to the old interace, is still available. Using it is discouraged, since it cannot enforce size checks!

.. code-block:: c++
  :linenos:

  class Histogram {
  public:
    MantidVec &dataX();
    const MantidVec &dataX() const;
    const MantidVec &readX() const;
    // Pointer access is slightly modified, holding a HistogramX:
    void setX(const Kernel::cow_ptr<HistogramX> &X);
    Kernel::cow_ptr<HistogramX> ptrX() const;
  };

.. categories:: Concepts
