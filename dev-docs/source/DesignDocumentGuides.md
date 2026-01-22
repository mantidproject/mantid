# Design Document Guidelines

```{contents}
:local:
```

A good design document minimises unexpected complications by addressing
them before the code is written. A document will provide you, your
manager and your team with a common vocabulary for talking about the
project. Writing an effective design document can be tricky in itself,
as they often add unnecessary complexity if not done with care. These
guidelines are to help with when and how to generate effective and
simple design documents.

## When to write a design document

This section provides some guidelines on when to produce a design
document. Note that, it is not about when to do a design and when not.
Except for trivial changes, one should always perform design work before
coding, no matter the scope or the nature of the feature. These
guidelines set out in which cases the design must be a *public*,
*written* and *persistent* document, rather than a sketch on a
white-board in a stand-up meeting. Just like coding, design is an
iterative and collaborative process. Having a written document allows
for discussion and iterations with peers, including across facilities,
before the first line of code is typed. Besides, the written document
facilitates the pull request review process. Having the design already
approved, the reviewer will not need to do a design review with the code
review, but will just need to make sure that the implementation matches
the previously agreed design. Finally, once written, the document can
also serve as (or easily be turned into) a developer documentation for
future reference.

Not every feature requires a design document. The benefit it adds must
be gauged with the time and effort that goes into it. Below are the main
scenarios when a design document is necessary or not.

<table>
<colgroup>
<col style="width: 42%" />
<col style="width: 57%" />
</colgroup>
<thead>
<tr class="header">
<th><blockquote>
<p>Design Document Needed</p>
</blockquote></th>
<th><blockquote>
<p>Design Document Not Needed</p>
</blockquote></th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td><ul>
<li>Large-scope feature - e.g. slice viewer, instrument viewer</li>
<li>Brand-new feature - e.g. crash reporter, plot manager</li>
<li>Core refactors - e.g. HistogramData, DetectorInfo</li>
<li>Abstractions, APIs - e.g. simpleapi, a new workspace type</li>
<li>Fully fledged libraries - e.g. Crystal library in the framework</li>
</ul></td>
<td><blockquote>
<ul>
<li>Bug fixes of any kind</li>
<li>Minor extensions to the current tools - e.g. adding another button
in a GUI widget</li>
<li>New functionalities to an extant algorithm – e.g. another target of
unit conversion</li>
<li>Concrete, single-class plugins - e.g. a new algorithm or a new fit
function</li>
</ul>
</blockquote></td>
</tr>
</tbody>
</table>

Below are a handful of measurable quantities that could help a developer
decide which category the feature belongs to.

### Amount of classes

Single-class plugins, such as algorithms, do not require design
documents. In the contrary, if the number of classes is more than a
dozen, the document could be relevant in order to describe the relations
between them.

### Amount of files

If the feature intends to touch (add or edit) a single file, design
document is not needed. If it is more than a hundred, opposite is likely
the case.

### Abstractness fraction

Stability of a piece of code must be proportional to its abstractness;
abstractions and APIs are hard to change, once in. So whenever the
feature concerns creations of new abstraction layers and APIs, a wider
discussion on the design is needed, hence the necessity of the document.
A good measure for this is the intended fraction of the abstract classes
(virtual or pure virtual) in the design.

### The layer of intent

If the feature is deep in the framework or workbench, it is more
critical to have a document. Features in the core layer must be
carefully thought out, as many other things will depend on them. In the
contrary, if the feature is just a plugin in the periphery (algorithm,
function, GUI, script), with low risk of side effects, a design document
is less important.

### Number of developers

If the feature is large enough that the implementation will be done in a
distributed way (i.e. more than one developer), the document will ensure
that the whole team has a common vision, discussed and agreed upfront.

### Number of users

If the feature is generic, it will be facing a wider audience of users,
hence a document and design review will make sure that the right feature
is being built in the right way. In the contrary, if the feature is
specific to one instrument or technique at one facility, producing a
document is less important.

### Number of person-months

Another metric could be the estimated effort in person-months. If the
feature is estimated to take a year to develop, it is worth spending a
few weeks to iterate and improve on the design. If the feature is less
than one month of work, it's probably an overkill.

These are just guidelines and not strict rules. It is hard to define
exact thresholds on these quantities, and often your feature is going to
be in a grey zone anyway. Therefore, it is advised to combine all these
metrics and make a professional judgement on a case-by-case basis.
Nevertheless, a good intuition on when a document is useful, and when
not, comes with experience. That's why, whenever in doubt, get in touch
with the senior members of the team - the gatekeepers. A good first
contact could be the local team lead at your facility, or the *tech-qa*
channel on slack.

## Design document process

Once identified that for a given feature a design document is needed,
the process goes as follows:

1.  Create an item in the technical working group roadmap
    [here](https://github.com/mantidproject/roadmap/projects/1) under
    the intended release.
2.  Produce the document in
    [markdown](http://en.wikipedia.org/wiki/Markdown) format with the md
    extension. Once ready for review, open a PR in
    [here](https://github.com/mantidproject/documents/tree/main/Design)
    or a subdirectory therein. The pull request should also include the
    sources (and not just the images) of any diagrams you put in the
    document. The diagrams can be drawn with
    [staruml](https://staruml.io/) or similar cross-platform or WEB
    solution.
3.  Once the PR is ready for review, put a message with a link in
    *tech-qa* channel, inviting the gatekeepers or other interested
    parties take a look and provide comments within one calendar week.
    Unlike the PR for code, the design reviews can and should have more
    than one assigned reviewer. The period can be extended if the scope
    is very large.
4.  Answer the comments under the PR and iterate as long as needed.
5.  Once the comments are incorporated, in absence of outstanding
    conflicts, the gatekeepers will approve and merge the PR of the
    design, which gives a green light to start the implementation.
6.  If there is still a debate between gatekeepers, the [Tsc](TSC) will set
    up a dedicated meeting, where the author will be invited to present
    and defend the design, and all the conflicts must be settled ideally
    via consensus, or in the absence thereof, via majority vote.
7.  Once the implementation PR is opened, the design document must be
    referenced in the PR message. If the feature required a design
    document, high is the chance that the implementation PR will require
    also a developer documentation.

## Design document content

We want to avoid a prescriptive approach to design document layout.
Design documents are about communicating design ideas, not a box ticking
exercise, so developers are expected to use their own professional
judgement about what goes into them. We are not providing templates for
this reason. The following are suggestions for sections that one should
normally have in a design document:

### Motivation

- Why does this design document exist?
- What is the overview of the problem?
- What use cases exist showing the present issue?
- How does this solve the requirements?
- Note that, *this section should be readable to non-developers*.

### Dictionary of Terms

Your opportunity to pair abbreviations to longer explanations. This is
not always necessary in documents where there are no special terms to
explain. If you need one, a two column table would be sufficient.

### Potential Solutions

It is important that you consider a wide range of possible solutions,
and don't just put forward your favourite. Remember that the design
document is a way of avoiding mistakes before coding, so spend some time
considering how several possibilities could be made to work.

For each potential solution, you should probably consider:

- Keep it brief and high-level at this stage
- What would the scope of the changes be?
- What are the pros/cons of this solution?

### Chosen Solution

You should provide logical reasons why you are choosing to adopt
solution A over solution B, C, D ... As the project grows in size, we
may need to be able to understand in the future the reasons why certain
designs have been adopted. If you are unsure which solution would be
best, you may submit the partially complete design document to the [Tsc](TSC)
for help. Design is itself an iterative process and documents are
frequently not accepted first time around, so be prepared to make
amendments, and don't take it personally if corrections are required.

Another thing to include is how can one verify the design? What are the
use cases that could be used to prove the viability of the solution?

### Implementation Detail

You could merge this section here with the one above if you wish.

- Use feedback to correct and clarify.
- Add more implementation detail. Diagrams are great, but you don't have
  to use strict UML, and use the appropriate UML diagrams depending upon
  the solution. Diagrams should help you and readers to understand the
  solution in a simple way, not make it more complicated.
- Could someone else follow the design and implement it based on the
  document without talking to you? You may not be the one implementing
  this, and it's even more likely that you will not be the only one
  maintaining it.

### Cheat Sheet and Checklist

The guidelines above do not need to be strictly followed, but the
following are necessary:

1.  Can non-experts understand the motivation for these changes?
2.  Does your design document link from requirements through the
    implementation details in a traceable manner?
3.  Can someone else implement this?
4.  What use cases verify that this design works?
5.  Has the [Tsc](TSC) approved it?
