.. _IDM-Structure-ref:

Inelastic Data Processor Class Structure
========================================
The figure below shows the structure of the Inelastic Data Processor window. The window consists of five tabs: Symmetrise, Moments, Sqw, Iqt and Elwin tabs.
The three tabs share a similar structure where every tab has three components a view, a model, and a presenter.

.. figure:: images/QENS/InelasticDataProcessorStructure.png

Data Processor Tab View
+++++++++++++++++++++++
The view will construct different UI elements. It initializes properties, setups editor factories,
setups properties managers, and adds properties dynamically to the tree property browser. Then, it will set up different signals and slots for the properties.
Also, it is responsible for data validation and visualization.

Data Processor Tab Model
++++++++++++++++++++++++
The model is responsible for setting the parameters of different algorithms. It also initializes the algorithms with the suitable parameters
and adds them to the batch algorithm runner.

Data Processor Tab (Presenter)
++++++++++++++++++++++++++++++
All tabs inherit ``DataProcessor`` class which contains shared functionalities used by all tabs.
The presenter coordinates between the view and the model using signals and slots. It updates the model data and parameters whenever there are new changes emitted
from the view. Also, it is responsible for running algorithms and notifying the state of the running operation.
