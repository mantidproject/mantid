.. _FlowchartCreation:

==================
Flowchart Creation
==================

.. contents::
  :local:


The flowchart diagrams are created by writing ``graphviz`` .dot files that describe the diagram in plain text, and placing them into ``docs/source/diagrams``. These can then be rendered in documentation by using the diagram directive in an .rst file:

.. code-block:: rest

   .. diagram:: AlgorithmName-v1_wkflw.dot

Examples of this can be found in `ReflectometryReductionOne-v2.rst <https://raw.githubusercontent.com/mantidproject/mantid/main/docs/source/algorithms/ReflectometryReductionOne-v2.rst>`__ and `ReflectometryReductionOneAuto-v2.rst <https://raw.githubusercontent.com/mantidproject/mantid/main/docs/source/algorithms/ReflectometryReductionOneAuto-v2.rst>`__.

The .dot format is quite simple¸ but very powerful for describing graphs. The approach we use is to describe all the nodes (shapes) we want first, grouping them into their types, and then defining how they're connected.

To provide a uniform look to all the workflow diagrams, templated keywords are provided which are swapped out with the correct styling options when the documentation is built. They are of the form ``${context}_style``. They're defined by the `diagram directive <https://github.com/mantidproject/mantid/blob/main/docs/sphinxext/mantiddoc/directives/diagram.py>`__.

An algorithm that takes one input workspace and scales it by a given parameter/property if it was given, may look like this:

::

   digraph DiagramName {
   //Comments are inserted in the same way as C++
   label = "MultiplyByParam Workflow Diagram"
   $global_style

   subgraph params {
     //These keywords beginning with $ are replaced with commands to style all the nodes in the subgraph correctly
     $param_style
     inputWorkspace  [label="InputWorkspace"]
     outputWorkspace [label="OutputWorkspace"]
     coefficient     [label="Coefficient"]
   }

   subgraph decisions {
     $decision_style
     checkCoefficient [label="Was Coefficient\ngiven?"]
   }

   subgraph algorithms {
     $algorithm_style
     scale [label="Scale"]
   }

   //Define the connections, labelling some of them
   inputWorkspace   -> checkCoefficient
   coefficient      -> scale           [label="Factor"]
   checkCoefficient -> scale           [label="Yes"]
   checkCoefficient -> outputWorkspace [label="No"]
   scale            -> outputWorkspace
   }

While creating the diagrams it's inconvenient to recompile the documentation with each change, so you may want to render the graph manually. This can be achieved on linux or cygwin by running the following command. *You may need to comment out the "$foo_style" lines when manually rendering as they are not valid graphviz syntax* (you can do this on the fly using sed to avoid having to edit the file).

::

   dot -Tpng -o output_image.png input_file.dot                       # render a graph manually
   sed 's/\$/\/\/$/g' input_file.dot | dot -Tpng -o output_image.png  # excludes $foo_style lines

You can also render them in a web browser using this `online graph renderer <http://www.webgraphviz.com/>`__.
