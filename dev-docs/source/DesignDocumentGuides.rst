.. _DesignDocumentGuidelines:

==========================
Design Document Guidelines
==========================

.. contents::
  :local:

A good design document minimises unexpected complications by addressing
them before the code is written. A document will provide you, your
manager and your team with a common vocabulary for talking about the
project. Writing an effective design document can be tricky in itself, as
they often add unnecessary complexity if not done with care. These
guidelines are to help with when and how to generate effective and simple design
documents.

When to write a design document
###############################

This section provides some guidelines on when to produce a design document.
Note that, it is not about when to do a design and when not.
One should always perform design work before coding, no matter the scope and the nature of the feature.
These guidelines set out in which cases the design must be a *public*, *written* and *persistent* document, rather than a sketch on a white-board in a stand-up meeting.
Just like coding, design is an iterative and collaborative process.
Having a written document allows for discussion and iterations with peers before the first line of code is written.
Besides, the written document facilitates the pull request review process.
Having the design already approved, the reviewer will not need to do a design review with the code review, but will just need to make sure that the implementation matches the previously agreed design.
Finally, once written, the document can also serve as (or easily be turned into) a developer documentation for future reference.

Not every feature requires a design document. The benefit it adds must be gauged with the time and effort that goes into it.
Bug fixes, small extensions to the current functionalities, as well as concrete, single-class plugins, such as algorithms, do not need a document.
In the contrary, if the feature is large-scope, brand-new, many-class, including abstractions, and/or touches or refactors the internals of the core framework, it will most likely need a document.
Below are a couple of metrics that could help a developer decide whether or not the feature falls into that category.

- Number of classes and components.

  If the number of classes is more than just a few, the document is relevant in order to describe the relations between them.

- Fraction of abstract classes and interfaces in the design.

  Abstractions and APIs are hard to change, once in. So whenever the feature concerns creations of new abstraction layers and APIs, a wider discussion on the design is needed, hence the necessity of the document.

- Amount of files and components the feature will be touching.

  Single file edits are less likely to require a document.

- The layer the feature will be added to.

  If the feature is deep in the framework or workbench, it is more critical to have a document.
  If the feature is just a plugin (algorithm, function, GUI, script) in the periphery, with low risk of side effects, a design document is less important.

- Number of developers involved.

  If the feature is large enough that the implementation will be done in a distributed way, the document will ensure that the whole team has a common vision, discussed and agreed upfront.

- Number of users the feature will be facing.

  If the feature is generic, it will be facing a wider audience of users, hence a document and design review will make sure that the right feature is being built in the right way.
  In the contrary, if the feature is specific to one instrument or technique at one facility, producing a document is less critical.

These are just guidelines and a good intuition on when to write a document comes with experience.
That's why, whenever in doubt, get in touch with the senior members of the team - the gatekeepers.
A good first contact could be the local team lead at your facility, or the *tech-qa* channel on slack.

Design document process
#######################

Once identified that for a given feature a design document is needed, the process goes as follows:

#. Create an item in the technical working group roadmap `here <https://github.com/mantidproject/roadmap/projects/1>`__ under the intended release.

#. Produce the document in `markdown <http://en.wikipedia.org/wiki/Markdown>`__ format with the md extension. Once ready for review, open a PR in `here <https://github.com/mantidproject/documents/tree/main/Design>`__ or a subdirectory therein. The pull request should also include the sources (and not just the images) of any diagrams you put in the document. The diagrams must be drawn with `staruml <https://staruml.io/>`__ free edition, as it is cross-platform.

#. Once the PR is ready for review, put a message with a link in *tech-qa* channel, inviting the gatekeepers or other interested parties take a look and provide comments within one calendar week. Unlike the PR for code, the design reviews can and should have more than one assigned reviewer. The period can be extended if the scope is very large.

#. Answer the comments under the PR and iterate as long as needed.

#. Once the comments are incorporated, in absence of outstanding conflicts, the gatekeepers will approve and merge the PR of the design, which gives a green light to start the implementation.

#. If there is still a debate between gatekeepers, the :ref:`TSC` will set up a dedicated meeting, where the author will be invited to present and defend the design, and all the conflicts must be settled via discussion or, eventually with a majority vote.

#. Once the implementation PR is opened, the design document must be referenced in the PR message.

Design document content
#######################

We want to avoid a prescriptive approach to design document layout.
Design documents are about communicating design ideas, not a box ticking
exercise, so developers are expected to use their own professional
judgement about what goes into them. We are not providing templates for
this reason. The following are suggestions for sections that one should normally have in a design
document:

Motivation
----------

-  Why does this design document exist?
-  What is the overview of the problem?
-  What use cases exist showing the present issue?
-  How does this solve the requirements?
-  Note that, *this section should be readable to non-developers*.

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

Another thing to include is how can one verify the design? What are the use cases that could be used to prove the viability of the solution?

Implementation Detail
---------------------

You could merge this section here with the one above if you wish.

-  Use feedback to correct and clarify.
-  Add more implementation detail. Diagrams are great, but you don't
   have to use strict UML, and use the appropriate UML diagrams
   depending upon the solution. Diagrams should help you and readers to
   understand the solution in a simple way, not make it more
   complicated.
-  Could someone else follow the design and implement it based on the document without talking to you?
   You may not be the one implementing this, and it's even more likely that you will not be the only one maintaining it.

Cheat Sheet and Checklist
-------------------------

The guidelines above do not need to be strictly followed, but the following are necessary:

#. Can non-experts understand the motivation for these changes?
#. Does your design document link from requirements through the implementation details in a traceable manner?
#. Can someone else implement this?
#. What use cases verify that this design works?
#. Has the :ref:`TSC` approved it?
