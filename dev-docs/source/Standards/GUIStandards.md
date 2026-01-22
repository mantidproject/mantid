# GUI Standards

```{contents}
:local:
```

## Prerequisite Knowledge

- MVP Design Pattern `MVPDesign`
- [Dependency Injection and Basic
  Mocking](https://vladris.com/blog/2016/07/06/dependency-injection-in-c.html)

## Preface

This document is structured as both a quick reference guide and detailed
descriptions as follows:

- The standards are the header titles and can be used standalone.

For most developers looking for something to link to / follow at a
glance the table of contents will have everything you need.

- Each standard has bullet points accompanying it for quick guidance if
  required.

If your unsure about a standard the bullet points should help give some
pointers related to that point. Feel free to skim as required to jog
your memory.

- Additional explanation, clarification and justification is provided
  after.

This is a larger commentary on why / what for people looking to either
learn or go into detail. It's not expected people read this if they are
already comfortable with the associated standard or points.

## Overview

There are multiple approaches to GUI design and implementation within
the Mantid codebase. The aim of this document is to adhere to these
agreed standards for future GUI work.

Existing GUIs in Mantid may differ, so developers should follow the
existing conventions where they are established. Each GUI will have a
document explaining these differences, as explained later in the
document.

This document should not be used as a reason to rewrite swathes of GUI
code, except for cases where the effort to match established patterns,
now or in the future, exceeds the work required to move to these
standards.

Developers should follow these guidelines when writing new interfaces as
closely as possible. There may be times where it's more appropriate to
deviate from these standards, but like all standards this must be
justified on a case-by-case basis in the associated PR.

## Standards

### Design

#### UX design reviews must be done beforehand

- You should get reviews off people before doing the work

Other developers will recognise potential pitfalls or problems with your
GUI, or maybe even adapt a thing or two on theirs.

- These don't need to be saved or published, as it's an informal process
- Don't verbally describe, always sketch something

Feel free to do these on whatever medium works best for you, be it tools
like Balsamiq (licenses available), the back of the envelope, a
whiteboard or crop circles in a field. The point is have a visual
reference that other people can see, as no two people will imagine the
same thing.

- Use this mockup to pretend you're interacting with the actual GUI.

Go through the process you expect a user to do, show drop downs, radio
buttons and tabs. You'll be surprised how many clunky design choices
this will show you before anything is written.

- Don't prototype with real code

The temptation is there to just go ahead and adapt your prototype. This
is going to take a lot of time to do, and is unfair on your reviewer. A
five minute sketch is a lot more comfortable to critique than a 3 hour
prototype.

#### Workflow alg. must be completed first

- Write a workflow algorithm that can be proved working before starting
  on the GUI

First and foremost we are a scientific tool. It's easier to validate the
results we produce in a separate PR without trying to figure out what
the GUI might have broke.

Don't be tempted to work on the GUI simultaneously, for your users' and
your benefit. Having something they can run from Python whilst you're
putting the finishing touches to the GUI will allow them to process data
for their users and allow you to deliver a fantastic product.

### Documentation

#### New work requires design documentation

- New GUIs or interfaces should have a design document completed
- Ideally, you'll want to do your informal UX design beforehand

New work should have an associated design document completed and
reviewed by another developer before any code is written. This should
layout what work should be completed beforehand, how the code will be
structured and what responsibilities each interface will have.

- Look for scope creep
- Is your GUI doing too much?
- Is there a "god" class/tab developing?

Users and developers alike will sometimes push to have everything in one
single page for accessibility. This
[article](https://blog.codinghorror.com/this-is-what-happens-when-you-let-developers-create-ui/)
shows you (and discusses briefly) what the end result is. A window with
30+ "convenient" buttons is only useful once you memorise them and
useless to external users.

Follow design principles provided in the training section and push back
if required when scope creep starts to happen. The later it's recognised
the harder it becomes to fix, yet the more problems it causes.

- Design docs should be reviewed by another dev

The same reasons we hold PR reviews applies, it gives us all a chance to
learn, recognise issues and discuss concerns. The design document should
go through a PR process like code would too.

#### The maintenance guide gets updated in the PR

- Keep the guide up to date

As you work on your GUI update the maintenance documents as appropriate
as part of the same PR. This prevents them falling out of date and will
give you the satisfaction of deleting that one thing that irked you
about the GUI for the last N sprints.

### Existing GUIs

#### Assess if the MVP code is getting too large

- Anecdotally one common cause of bugs is "...logic kept getting added
  to existing code"
- It's hard to step back and assess if you're suffering from scope
  creep, so do it often

As developers enjoy writing new features (well at least most of us), we
get into [Go Fever](https://en.wikipedia.org/wiki/Go_fever) and don't
stop to reflect on what we're working on.

Periodically step back, look at your design documents and ask, "does
this fit in?" Better still talk to a nearby co-developer, interested
user or rubber duck debugging device. Describe what features were
already there and tell them about this new feature, do they sound like
they fit in the same place?

- When it doubt, split it out
- An eager split is easier to fix than a split too late

When you think the scope of your current file / directory is growing too
far split it out early. Especially with UI files, the later you leave it
the harder it becomes to unpick everything.

If you split, then it turns out the work completed wasn't as large as
anticipated it's as simple as moving your imports, then cutting and
pasting code and tests across. This is much easier than the other way
where you have to unpick all the callers in the same file and shared
variables.

- Feel free to create helper classes where required

Especially with models, the logic can grow quickly and become difficult
to test too. If you can justify the scope of the model then look into
delegating work to classes which each have a single responsibility.

For example, if you have a lot of logic to work out what titles to put
on various plot windows consider moving it to a PlottingWindowTitles
helper class with its own associated test.

#### Work on existing GUIs should follow maintenance docs

- Follow the conventions in the guide
- Don't rewrite the world because you don't like how it looks
- Look at the maintenance docs and check new work conforms during the PR
  process

Each GUI should have an associated maintenance guide with it. The
conventions might be incorrect, weird or downright wrong. However,
consistency is key to helping future you and everyone else who works on
the GUI maintain some level of understanding.

One temptation we get as developers is to rewrite something we don't
understand, this isn't to deny the fact that some code
is....difficult... These standards are a result of those mistakes, but
we don't have time to go back and put the world to right.

- If you're spending excessive time fixing bugs / maintaining code
  discuss modernising

This is beyond the scope of these standards, but don't feel like
everything is set in stone and you've been prescribed to draw proverbial
blood from said stone. If you're spending excessive time fixing bugs or
maintaining code raise these concerns in the project management chain
for the possibility of rewriting.

### Model

#### No matter how small, logic goes into the model

- Avoid stumbling into the trap of, "I'll just get the presenter to do
  this"
- "Just an if statement" can grow into a whole model in the presenter
- You'll save time writing testing in the long-term

It's been said that the best developers write the least code, however in
this case it works against them. That simple if statement which changes
whether a check-box is enabled should be in the model. Often a simple if
statement in the presenter gets some new checks added...etc. until it
becomes a hidden model.

Suddenly you'll end up having to pry out a model from the presenter,
taking significantly more time than that first bit of work saved.

#### The GUI must map 1:1 to a workflow alg.

- Always have a 1:1 mapping between GUI and algorithm

Reflectometry used the N algs to one GUI mapping. This initially appears
like a good idea, since it reduces the code per workflow algorithm.
However, long-term you now have N entry points to test separately and
maintain.

Having a single algorithm that handles the workflow associated with your
GUI keeps it trivial to maintain, and also helps you maintain another
standard; the workflow algorithm must work without the GUI.

#### The GUI must work with workspace history

- Keep an eye out for reduction logic creeping into your MVP code
- Any decisions the GUI makes about data (beyond browsing to it), goes
  into an alg.

Any steps performed outside of the algorithm disappear from the
workspace history. This breaks the reproducibility of data since running
a script skirts around any code in your model.

- Keep the workflow separate, the GUI should become a way to run a
  workflow
- Run your workflow algorithm from the GUI, then re-run it as a script

Imagine your GUI could be swapped for a CLI for users, would it still
work on the back-end?

### Presenter

#### All notifications need to go through a single point

- Becomes obvious when it's starting to grow outwards
- Prevents dead code paths, and multiple code paths to do the same thing
- Makes debugging significantly easier

Firstly, having a single method in your presenter handle every
notification means you can put a single break-point when debugging.
Imagine a future dev trying to figure out why that field in the GUI
keeps changing, but no breakpoints hit because you made another
notification handler. Now imagine you're that future dev.....

The other aspect is having a single method that's growing beyond 30
lines is painfully obvious. Maybe you have some dead code paths that
need to be trimmed out, maybe you are suffering from scope creep or
maybe your notifier is doing model logic. In any case it's easier to
spot now rather than in 5 different places.

#### Do not let Qt types in

- Keep non-standard types contained inside the view
- Some debuggers can't see what these type values are
- You then only have to think about one type instead of doing
  conversions everywhere

To use a cliché, when you have a hammer suddenly everything looks like a
nail: Many developers will use conversions (hammer) at the last second
to coerce their type (nail) into something "standard" which the rest of
the code-base uses at the point they need it.

You only notice something is wrong when you've put in so many
conversions that it's either hard to keep track of what the type is at
this point (Python), or you can't see into them debugging (C++).

- Convert to a standard type in the view, not the presenter

The view is the boundary between your code and external code you have no
control of. Do the conversion at the point return something, that way
it's obvious in the presenter which types you're working with.

If the structure is quite complex such as a class containing multiple
related fields consider packing it into your own struct (C++) or POD
class (Python). That way these Qt classes don't leak into your tests
either where you need a more complex type.

#### If you can't test it, put it in the view

- Don't let Qt / Plotting bleed in

It's tempting to check if plotting still plots, or Qt still Qts. But
leave that to the project / teams responsible and test up to the API
call only.

Keep your API calls to things you cannot test in the view, and keep the
logic in the model. The idea is that the presenter should be doing calls
like, "here is a workspace and figure, go plot that workspace on that
figure". The dumber the logic the less likely it will break.

- Can you test with only mocks?

This is a good litmus test, a presenter should not have tests that do
ASSERT_EQUAL, as that's probably doing logic. The model (which you've
tested previously), should just return the right thing.

Your tests should check that the presenter blindly accepts from the
model or view and forwards to the opposite, unmodified. The easiest way
is to mock a return value from the view/model, and check that's passed
in as a parameter to the model/view as is.

#### Limit chains of observers notifying each other

- You don't want to follow a big circle whilst debugging
- It can introduce nasty race conditions that are nightmarish to
  reproduce
- Handle this by moving handling into its own presenter which handles
  all chains of this sort
- Ultimately good organisation of these chains is ultimately key

Having a big chain of observers notifying each other tends to happen
implicitly; multiple presenters or views are wired up to notify on value
changed. This then changes yet more values and cascades.

Existing GUIs (such as Reflectometry) avoids this by having a presenter
which handles all notifications. It's obvious who is notifying who, and
how far the chain can extend.

#### Notify must use enums to signal

- Don't use primitive types (int/string...etc.)
- Have a single point which accepts an enum of expected signals

As the number of signals you accept grows using primitives can quickly
cause problems: was 1 this signal or that, what does a blank string
mean? ...etc.

This is used in combination with having a single notification point, if
you need multiple enums or your current enum class is growing you are
likely suffering scope creep. Step back and consider if your GUI has
gone beyond its initial design.

#### Presenters must use non-Qt communication patterns

- This avoids the pain point of "Qthings" propagating
- A signal can trigger another signal without you being able to see it
  easily

Signals and slots have an allure which cause some to break the standard
of keeping Qt in the view. Quickly these Q types will bleed through as
you need to handle each in the presenter.

- Use an observer pattern:
- More explicit what goes where
- Typing system can help you in C++
- They don't silently fail unlike signals and slots
- This avoids issues where signals go "in circles"

Take the time out to write an observer pattern, this can be combined to
also satisfy the rule of having a limit of chains by using a single
presenter that handles everything.

This incurs an initial cost of writing the observer, but quickly you can
mock out the implementation saving you manually testing it in the
future. Later on when your co-developers are fighting Qt firing a signal
but not slots reacting you can point to your observer pattern that
either just works or fails with a nice stack trace.

### Training

#### Discuss estimates within teams

- One of the hardest things to do is give an accurate estimate
- Being accurate means our users can prioritise work better
- Some users will take estimations as deadlines too
- This practises the estimation process

Before and after starting work developers should be estimating. However,
GUIs tend to require most careful consideration since things will catch
you out. For example, what happens if you need to redo some of your UX
design, or it turns out that new feature Y will require us splitting our
work from GUI X?

Take the time to talk to other developers within your team about the
work you think will be required, and what might come up in the best and
worst case.

Likewise after the work is completed feedback on what you were right and
wrong about and take it forward to your next chunk of GUI work. Maybe
you're on the cusp of having to split a file and any extra features will
force you to do it or you've spent extra time refactoring last time so
it should be quicker next.

#### Encourage newer staff to raise queries

- Are you solving a symptom for a user, rather than their actual
  problem?

What separates a good product from a great one from a user perspective
is recognising what problem someone is trying to solve rather than the
symptoms. Taking a step back and asking what problem someone is trying
to solve can sometimes yield better solutions, such as writing a script
or tweaking a workflow algorithm.

For example, are they asking for a tick box for something which is
almost always true except for once a year? In this case can we add an
option to the workflow alg but not the GUI, that way most users can't
get it wrong. Then write a script which does it once a year for those
power users who need that feature.

- Newer devs: talk to more experienced devs, ask if there is a better
  way to do stuff

Invite them along to your user meetings to sit back and help. They
should not be there to run your meeting or take over. Instead they might
recognise something that another group has done, a potential problem or
even a hidden solution.

Feel comfortable saying no (in gentler terms) whenever you think
something is unreasonable, raise concerns and suggest alternatives.
Users would rather not see multiple months wasted on something they
thought was trivial to do, as there can be a disconnect between
perceived and actual difficulty. An extreme example might be a bespoke
GUI for a single user when a script would be perfectly adequate.

When in doubt raise it through your management chain, they're there to
help!

- Experienced devs: check the dialogue is happening

Ask to go to meetings with newer devs until you feel like they can
recognise the various components and tools available from Mantid for
solving problems.

Check that the requirement gathering, estimation process and UX design
steps are being done with the group. Encourage developers to partake in
negotiating what work is appropriate and where it should be scheduled.

Watch out for things being added mid-sprint silently, newer devs can
feel obligated to complete the planned work in addition to the extra
work. This can cause burn out and setup an unofficial mechanism where
requests will bypass the sprint planning mechanism.

#### Periodically revise best UX practice

- Ask experienced UX Developers for guidance
- Multiple online resources exist:
- [NNGroup](https://www.nngroup.com/ux-conference/)

### Testing

#### If you have too many dependencies to inject...

- First consider if you're making the problem worse by adding
- Having too many deps is a sign you're doing too much

Check if you're suffering from scope creep that you can resolve, not
adding DI and relying on mechanisms such as patching disguises this
problem.

- Look at using a factory pattern if you really must pass lots of args
  through

If you're adding tests to existing code or require major refactoring
work consider adding a factory class which allows you to get all your
dependencies. This is injected instead and the constructor calls a
builder method for each dependency, the testing class will inherit and
override to inject mocks.

- Last resort is to create a test class as a friend of the presenter

This should be considered a last resort as it also opens an avenue for
manipulating the internals of the class under test conditions. Tests
which manipulate internals are no longer testing a public API and are
usually a sign you have a helper class lurking inside.

#### Use dependency injection

- Dependency Injection makes it clear what needs to be mocked out for
  each test
- Keep the number of deps low and have a param for each one in the
  constructor

Dependency injection should be used to inject various mock objects, this
is a well established pattern that has been shown to simplify tests when
used well.

If you have a constructor which has several args due to DI your MVP code
has probably started suffering scope creep. Can you either create new
classes to encapsulate multiple dependencies, or create a new MVP set?

- You may have a problem with the entry point code for production

Look at the maintenance guide or dev documentation, some interfaces will
use a top window view which creates the real instances to inject into
its children. By keeping it in the view (not model or presenter) we
follow the standard, if you can't test it put it in the view.

#### Use dependency injection in Python too

- You should use DI in Python too! This will also future proof you for
  growth too.
- Use this over patching wherever possible

Patching can be difficult to get right, resulting in lots of copy and
paste code. Instead DI can be done once in the setup method of your unit
test class. This will make it easier to add new tests in the future too
since you've already done the work in your setup.

Getting the path right can be difficult and developers have to learn
various rules about how things are imported into different namespaces,
which are only really used for patching tests.

- For existing code can you let init run then replace it?

Python allows you to reach into a class's internals and replace them.
Consider replacing the attribute with a mock. For example:

``` python
# View needs to be mocked but no DI available
presenter = MyPresenter()
# Injected the view without using mocks
presenter.view = mock.Mock()
```

Note this shouldn't be used in lieu of dependency injection through the
constructor since it will hide the code smell of feature creep

### View

#### Keep Qt / Plotting in the view

- Keep Qt/Plotting API calls and types in the view
- Convert your types within the view

Covered in `cant_test_in_view` from the other perspective.

Assume that anything in the view is difficult to test internally, but is
tested elsewhere. For example, it doesn't make sense to check if
Matplotlib still opens plots if you ask it to show a figure.

Ultimately, anything that goes in the view has to be tested manually, so
less code means fewer ways for it to silently break without manual
testing noticing.

- Have one or two very generic methods handle a lot of calls to an API

Having one or two very generic methods handle all calls to an API will
mean if it ever broke it would break everything in your GUI at once,
making it obvious where the problem is. Having 6 different ways to plot
from the view for example means you now need to test 6 different
plotting methods in your presenter manually.

#### If you can test it, take it out of the view

- Look out for small logic snippets creeping into the view
- For loops in the view are somewhere logic usually gets hidden in
- A view API should have everything prepared for it and be as "dumb" as
  possible

It's very easy to have a for loop in a view which starts off iterating
through a list then adding extra steps while you're there. The view
should have this all pre-prepared for it in a way that any for loops
take an object, unpack it and forward it to an API only.

For example, if you need to group 12 items into 3 boxes of 4 in the view
don't put the grouping into the view. Instead pack them into a list of
lists or create a new struct (C++) / POD class (Python) to hold this and
iterate through that instead.

- Go line by line and think, "could I test just this line alone?"

For certain lines such as calling Qt there would be no possible way to
tell if a test passed or not, or if they use view only types (see
`keep_qt_in_view` ) then they can stay.

If a line would have an expected output it might need moving to the
model instead. Consider using mocks if the input types it expects are
difficult.

#### One signal per slot, never multiple

- Signals should always have a 1:1 mapping with slots
- Makes it obvious if the combo is broken

Signals and slots should always be paired up, but never more than 1:1.
It can look like one signal and slot is working as tested, which then
hides the fact you unintentionally broke 3 other slots.

#### Signals setup in the view to the presenter only

- Views should only signal the presenter
- There should not be signals firing other signals in the same view

Signals that go to the view are almost impossible to automatically test,
since the side effects are invisible to the presenter unless more
signals are then chained up.

In addition it can lead to logic ending up in the view which goes
against `logic_goes_into_the_model` . This should be combined with
`notifications_through_single_point` in your presenter which should then
switch to a non-Qt notification method `use_non_qt_comms_presenter`.
Combining all of these patterns ensures that everything can be tested in
an automated fashion.

## Future Work

### Coding

#### Naming conventions

- Outstanding work to unify across the project
- Need to document what patterns exist
- Look at harmonising into the future, maybe meet and decide going into
  the future

### Docs

- Look at writing a document detailing how each GUI is structured for
  next maintenance period
- Add structure to dev-docs, and add files into that. Move maintenance
  guides into there
- Add examples where code should have been broken down
- Shared code examples where problems have been solved or how to do
  stuff

### Possible Training

#### Concept discussion sessions for discussing these design principles

- Structured day(s) (possibly 3) to teach these concepts
- Part of a larger group session, further down the line
- Define a sequence of concept discussions, with what needs to be
  covered
- Look at a retrospective on the concept discussions

#### UX Point of contact

- Look at having a UX point of contact
