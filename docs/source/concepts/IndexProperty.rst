.. _IndexProperty:

Index Property
==============

.. contents::
  :local:

What is an IndexProperty?
------------------------

The ``IndexProperty`` in Mantid provides a consistent interface for algorithms 
which need to access a subset of the workspace spectra for processing. This property facilitates the
retrieval of a set of workspace indices (or spectrum indices), called a ``SpectrumIndexSet``,
once provided with a set of workspace indices or spectrum numbers [#]_. Therefore
algorithm developers do not need to perform any conversions manually. In 
situations where data is distributed across several clusters (distributed processing) [#]_,
the underlying IndexInfo object, which is used to obtain the SpectrumIndexSet, hides
these mpi-specific details. Therefore access to the workspace data remains
unchanged for developers.

.. [#] Not specifying any indices/spectrum numbers results in the processing of the entire workspace. Or all indices on a given MPI rank.
.. [#] See :ref:`AlgorithmMPISupport` for more information on MPI support.

How to use the IndexProperty
----------------------------

Unlike other property types in Mantid, the ``IndexProperty`` is designed to be used in conjunction with other properties
which define the workspace and the input type of the indices which represent
the subset of the data. However, developers do not need to concern themselves
with maintaining these properties on their own. There are few special methods in
``Algorithm`` which handle this. Namely, ``Algorithm::declareWorkspaceInputProperties``,
``Algorithm::setWorkspaceInputProperties`` and ``Algorithm::getWorkspaceAndIndices`` [#]_. 

Property declaration is as shown below: 

.. code-block:: cpp

  //Declare property with default settings
  declareWorkspaceInputProperties("InputWorkspace");

Internally, a ``WorkspaceProperty`` is created along with an ``IndexTypeProperty`` for
managing the workspace and the type of user-defined input index list respectively. Their names are
automatically generated based on the property name in the declaration. 
The resulting algorithm dialog in the GUI would have the following inputs defined:

.. image:: ../images/IndexPropertyDialogExample.PNG
   :height: 300px
   :alt: IndexProperty Dialog Example

After properties have been set, client code can retrieve the values of interest from
within the algorithm as follows:

.. code-block:: cpp

  //Declare workspace and index set
  MatrixWorkspace_sptr inputWs;
  SpectrumIndexSet indexSet;
  
  //Method returns a tuple of the workspace
  //and index set simultaneously
  std::tie(inputWs, indexSet) = 
        getWorkspaceAndIndices<MatrixWorkspace>("InputWorkspace");
        
  for(auto index: indexSet){
    auto &spec = inputWs->getSpectrum(index);
    //do something with spectrum.
  }

For setting the property values, there are 4 valid options:

.. code-block:: cpp

   //Set Property with workspace_sptr and string of indices
   setWorkspaceInputProperties<MatrixWorkspace, std::string>(
      "InputWorkspace", ws, IndexType::WorkspaceIndex, "1:5")
      
   //Set Property with workspace name and string of indices
   setWorkspaceInputProperties<MatrixWorkspace, std::string>(
      "InputWorkspace", "ws", IndexType::WorkspaceIndex, "1:5")
      
   //Set Property with workspace_sptr and vector of indices
   setWorkspaceInputProperties<MatrixWorkspace, std::vector<int>>(
      "InputWorkspace", ws, IndexType::WorkspaceIndex, 
       std::vector<int>{1, 2, 3, 4, 5})
       
   //Set Property with workspace name and vector of indices
   setWorkspaceInputProperties<MatrixWorkspace, std::vector<int>>(
      "InputWorkspace", "ws", IndexType::WorkspaceIndex, 
       std::vector<int>{1, 2, 3, 4, 5})

.. categories:: Concepts

.. [#] It is important to note that any attempt to access the ``IndexProperty`` or the ``WorkspaceProperty`` in isolation will fail. Once defined using the ``Algorithm::declareWorkspaceInputProperties`` method, all access must be via the three methods mentioned above.

