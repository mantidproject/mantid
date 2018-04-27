.. _DesignDocumentGuidelines:

==========================
Design Document Guidelines
==========================

.. contents::
  :local:

A good design document minimises unexpected complications by addressing
them before the code is written. A document will provide you, your
manager and your team with a common vocabulary for talking about the
project. Writing an effective design document can be tricky in itself
they often add unnecessary complexity if not done with care. These
guidelines are to help you generate effective and simple design
documents.

Before starting, all Mantid design documents must be written in
`markdown <http://en.wikipedia.org/wiki/Markdown>`__ format with the md
extension. After approval from the the Mantid Project :ref:`TSC`, they should be stored
`here <https://github.com/mantidproject/documents/tree/master/Design>`__
or in one of the subdirectories of that directory. This page covers what
happens between those points.

Guidelines on Sections
######################

We want to avoid a prescriptive approach to design document layout.
Design documents are about communicating design ideas, not a box ticking
exercise, so developers are expected to use their own professional
judgement about what goes into them. We are not providing templates for
this reason. The following are guidelines for writing your design
document, to help give you some direction.

Motivation
----------

-  Why does this design document exist?
-  What is the overview of the problem?
-  What use cases exist showing the present issue? For example scripting
   issues.
-  How does this link in with long-term requirements such as SSC outputs
-  **This section should be readable by non developers**. Not everyone
   knows that the ADS stands for the Analysis Data Service and wider
   stakeholders definitely will not

Dictionary of Terms
-------------------

Your opportunity to pair abbreviations to longer explanations. This is
not always necessary in documents where there are no special terms to
explain. If you need one, a two column table would be sufficient.

Potential Solutions
-------------------

It is important that you consider a wide range of possible solutions,
and don't just put forward your favourite. Remember that the design
document is a way of avoiding mistakes before coding, so spend some time
considering how several possibilities could be made to work.

For each potential solution, you should probably consider:

-  Keep it brief and high-level at this stage
-  What would the scope of the changes be?
-  What are the pros/cons of this solution?

Chosen Solution
---------------

You should provide logical reasons why you are choosing to adopt
solution A over solution B, C, D ... As the project grows in size, we
may need to be able to understand in the future the reasons why certain
designs have been adopted. If you are unsure which solution would be
best, you may submit the partially complete design document to the :ref:`TSC` for help. Design
is itself an iterative process and documents are frequently not accepted
first time around, so be prepared to make amendments, and don't take it
personally if corrections are required.

How will we verify the design, what are the use cases that could be used
to verify the solution?

Implementation Detail
---------------------

You could merge this section here with the one above if you wish.

-  Use feedback to correct and clarify.
-  Add more implementation detail. Diagrams are great, but you don't
   have to use strict UML, and use the appropriate UML diagrams
   depending upon the solution. Diagrams should help you and readers to
   understand the solution in a simple way, not make it more
   complicated.
-  Could someone else follow the design and implement it? You may not be
   the one implementing this, and it's even more likely that you will
   not be the only one maintaining it.

Sign Off
--------

Design documents help us to manage risk and reduce cost to the project.
We have had some bad experiences in the past where high-risk changes
have been made ad-hoc. You must get approval from the :ref:`TSC` before embarking on the work outlined in the
design document.

Cheat Sheet and Checklist
#########################

The guidelines above do not need to be strictly followed, but the
following are necessary:

#. Can non experts understand the motivation for these changes?
#. Are your design decisions traceable? Does your design document link
   from requirements through to implementation details in a traceable
   manner?
#. Can someone else implement this?
#. Has the :ref:`TSC` approved the design.
#. What use cases verify that this design work
#. Documents must be version controlled and live in a subdirectory of
   `here <https://github.com/mantidproject/documents/tree/master/Design>`__
   as a `markdown <http://en.wikipedia.org/wiki/Markdown>`__ document
