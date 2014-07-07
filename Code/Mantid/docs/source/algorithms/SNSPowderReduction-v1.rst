.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

About Filter Wall
#################

Filter wall is enabled by setting *FilterCharacterizations* property to true. 
Then the \_getTimeFilterWall routine is used to build the filter wall from the data defined by 
*SplitInformationWorkspace* and  *SplittersWorkspace* properties.
Time filter wall is used in \_loadData to load data in a certain range
of time. Here is how the filter is used:

| ``   1. There is NO filter if filter wall is NONE``
| ``   2. There is NO lower boundary of the filter wall if wall[0] is ZERO;``
| ``   3. There is NO upper boundary of the filter wall if wall[1] is ZERO;``


Usage
-----

This is a worksflow algorithm used to process and the results of scattering experimens on powders on SNS instruments.
Its usage sample can be found in `Mantid System tests repository <https://github.com/mantidproject/systemtests/blob/master/SystemTests/AnalysisTests/SNSPowderRedux.py>`_.

.. categories::
