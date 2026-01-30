# Algorithm Documentation

```{contents}
:local:
```

## Summary

This page deals with the specifics of how to document an algorithm. For
a more general guide to the Mantid documentation system see
`DocumentationGuideForDevs`.

## How to Document an Algorithm

Algorithm documentation is stored in two places.

- The code (.cpp / .h / .py) files: For strings that are needed in the
  GUI for tooltips etc.
- The .rst file: For all other documentation, including the algorithm
  description and [usage examples](AlgorithmUsageExamples).

### The Code Files

The code files hold documentation in two important areas.

The `summary` method

:   The summary method should return a string (in plain text) that
    describes in a short sentence the purpose of the algorithm. This is
    used to provide a summary in the algorithm dialog box and in the
    algorithm documentation web page.

In C++: (This example uses the .h file, you could of course implement it
in the .cpp file)

``` c++
/// Algorithm's name for identification overriding a virtual method
const std::string name() const override { return "Rebin";}

///Summary of algorithms purpose
const std::string summary() const override {return "Rebins data with new X bin boundaries.";}
```

### Property Descriptions

Each property declaration in the init method of the .cpp file should
include a description. This is used as a tooltip, and in the
'properties' table on the algorithm's documentation web page.

```{note}
The tooltip may only show the description up to the first dot. Be sure
that the first sentence in the description gives enough information to
understand the purpose of the property.
```

For example:

``` c++
declareProperty(
new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
  "Workspace containing the input data");
declareProperty(
new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
  "The name to give the output workspace");
declareProperty(
new ArrayProperty<double>("Params", std::make_shared<RebinParamsValidator>()),
  "A comma separated list of first bin boundary, width, last bin boundary. Optionally "
  "this can be followed by a comma and more widths and last boundary pairs. "
  "Optionally this can also be a single number, which is the bin width. "
  "In this case, the boundary of binning will be determined by minimum and maximum TOF "
  "values among all events, or previous binning boundary, in case of event Workspace, or "
  "non-event Workspace, respectively. Negative width values indicate logarithmic binning. ");
```

## Workflow algorithms

There should be a flow chart for workflow algorithms. See
[here](FlowchartCreation) on how to make one.

## Algorithm Directives

We have defined several custom directives using the Sphinx extension
framework. These are defined to generate the information from an
algorithm that is not required to be hand written, i.e the properties
table. Each .rst file that is documenting an algorithm should use these
directives.

As the **Description** and **Usage** of an algorithm *cannot* be
obtained automatically, it must be manually entered. The structure of an
algorithm's .rst is:

``` rest
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The description of your algorithm here.

Usage
-----

A custom usage example.

.. categories::

.. sourcelink::
```

[.. algorithm ::](.. algorithm ::)
:   This directive has several pieces of functionality, which includes:

- A referable link is created for the algorithm. This allows other
  documentation pages create references to it (e.g. create links to it).
- Insertion of the page title (which is the name of the algorithm,
  including the version).
- Insertion of a screenshot of the algorithm's dialog.
- Insertion of the Table Of Contents.

[.. summary::](.. summary::)
:   The content of the summary method declared in your algorithm is
    output as HTML, for example, the following method is used in Rebin:

``` c++
/// Summary of algorithms purpose
const std::string summary() const override {
   return "Rebins data with new X bin boundaries. For EventWorkspaces, you can very quickly rebin in-place by keeping the same output name and PreserveEvents=true.";
}
```

[.. relatedalgorithms::](.. relatedalgorithms::)
:   This directive obtains a list of related algorithms from the
    `seeAlso` and `alias` methods in the algorithm, for example, the
    following `seeAlso` is used in Rebin:

``` c++
/// Algorithm's seeAlso
const std::vector<std::string> seeAlso() const override {
 return {"RebinToWorkspace", "Rebin2D",           "Rebunch",
         "Regroup",          "RebinByPulseTimes", "RebinByTimeAtSample"};
}
```

[.. properties::](.. properties::)
:   As mentioned above, it is *critical* that you include a description
    for the properties of your algorithm. This directive obtains all of
    the algorithm's properties (set inside the algorithm's `init`
    method) and outputs in a table format.

[.. categories::](.. categories::)
:   By default, this directive obtains the categories that were set in
    the `categories` method the algorithm. For example, in Rebin the
    category method is in the header and contains:

``` c++
/// Algorithm's category for identification
const std::string category() const override {return "Transforms\Rebin";}
```

When the HTML is generate a categories list is built that contains:
Algorithms, Transforms and Rebin.

It is possible to add additional categories by passing the directive
arguments, for example, the following would output the above categories,
and also `Example`:

```
.. categories::

Algorithms, Transforms, Rebin, Example
```

[.. sourcelink::](.. sourcelink::)
:   This directive adds links to the algorithms source code.

## Description

This section must be manually entered. The description is an extension
of the summary, and must contain a detailed overview of the algorithm's
functionality.

### Referencing Other Algorithms

Every algorithm and version has been marked with a tag that can be used
to reference it from other documents. If you need to reference the
latest version of an algorithm, e.g. DiffractionFocussing, then in
Sphinx you type would type

``` rest
:ref:`DiffractionFocussing <inv:mantid#algm-diffractionfocussing-v1>`
```

If you need to reference a particular version then you would type

``` rest
:ref:`DiffractionFocussing version 2 <inv:mantid#algm-diffractionfocussing-v2>`
```

where the first part outside the angle brackets defines the link text
and the part inside the angle brackets references the tag name.

## Usage

This section *must* be manually entered. The usage is a 'code' example
of the algorithm in use. The `.. testcode::` directive must be used to
verify the usage code you entered works correctly. See
[here](AlgorithmUsageExamples) for more information on how to write
usage examples.

## Building the Documentation

One can update the documentation for a particular algorithm after
changes have been introduced into the corresponding documentation file.
Assuming you are in the build directory and want to update the
documentation for Rebin:

    PYTHONPATH=/path/to/build/bin python3 -m sphinx <sourcedir>/docs/source docs <sourcedir>/docs/source/algorithms/Rebin-v1.rst
