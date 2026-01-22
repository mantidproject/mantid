# New Starter Python

::: {.contents local=""}
:::

## Prerequisites

- Access to the [Python documentation](https://docs.python.org/3/)
- You will also need a computer with a Python development environment.
- Git: *Windows/Mac* - Download the latest version from
  <http://git-scm.com/>; *Linux* - Install from package manager (ubuntu
  pkg=git-gui)

## Version Control

You will be using the [git](http://git-scm.com/documentation) version
control system with Mantid. Here we aim to get you started using git
while working through the new exercises. Our code is stored on
[github](https://www.github.com/) so to get started you will need an
account.

1.  Navigate to [github](https://github.com/), fill in the details:
    username (e.g. firstnamelastname), email & password and click sign
    up.
2.  Go to <https://help.github.com/articles/set-up-git> and follow the
    instructions to set up git for your environment (for windows do NOT
    use the native app)

Before you start the exercises below it is a good idea to read
[this](http://git-scm.com/book/en/Git-Basics-Recording-Changes-to-the-Repository)
page that discusses the basic operations of working with git.

The idea of version control is that snapshots of the development history
can be recorded and these states returned to if necessary. As you go
through the exercises and get to a point where something is working it
is a good idea to *commit* the changes to your new repository. On Mantid
we use what are known as branches within the repository to keep track of
a single piece of work. The idea is that each feature/bugfix is
developed independently on a separate branch within the repository. When
the work is complete, it is tested by another developer and merged
within a special branch, called **main**. This branch is reserved code
that will form part of that distributed to users. More details on
Mantid's workflow with git can be found
[here](https://developer.mantidproject.org/GitWorkflow.html).

While developing the code for your exercises you will work in a separate
repository [here](https://github.com/mantidproject/newstarter) but the
intention is that you will follow the workflow described in the document
above and in particular using the commands described in
[this](https://developer.mantidproject.org/GitWorkflow.html#Workflow_Git_Commands)
section. Do not use the macros yet, the aim is to understand the process
by using the real commands.

Before starting the exercises, there are some setup steps:

- Clone this repository: <https://github.com/mantidproject/newstarter>
- Make a new branch
  `git checkout --no-track -b firstname_lastname_exercises origin/main`,
  where *firstname*, *lastname* should be replaced as appropriate.
- Make a copy of the *exercises-python/template* directory and name it
  *firstname_lastname*. The directory should be in the exercises
  directory.
- Make a directory called *builds* in the root of the *newstarter*
  repository. Git is setup to ignore this directory.

Now you are ready to code the solution to the exercise in your chosen
native build environment.

- As you work use the git commands to commit to your branch and push to
  GitHub.
- When you think you have completed the exercise you can use the
  continuous integration build servers to check your work. To do this
  you first need to create a pull-request. See [create a pull
  request](https://help.github.com/articles/creating-a-pull-request/)
  for your branch so that it can be reviewed by a senior developer.
- The pull request will kick-off builds on Red Hat and Windows platforms
  and GitHub will mark up the results of these builds on the pull
  requests. Try and get each build to a green status before saying it is
  ready for review. As you push further commits to your branch the PR
  will update and new builds will kick off to check your work. Continue
  in this pattern until the builds pass. If you're not sure how to
  resolve some errors check with another member of the team.

## Python Basics

Feel free to skim read sections that you understand, just pay attention
to anything that is new to you.

Mantid uses Python 3.

### Reading

- Python library:
  [Strings](https://docs.python.org/3/library/stdtypes.html#text-sequence-type-str)
- Python tutorial: [Defining
  Functions](https://docs.python.org/3/tutorial/controlflow.html#defining-functions)
- Python tutorial:
  [Dictionaries](https://docs.python.org/3/tutorial/datastructures.html#dictionaries)
- Python tutorial: [Command Line
  Arguments](https://docs.python.org/3/tutorial/stdlib.html#command-line-arguments)

### Exercise

The code should be placed in
*exercises-python/firstname_lastname/ex01_basics/"*

Write a command line program that will:

1.  Take a filename of an ascii file as an argument (you can use the
    example file
    [here](https://github.com/martyngigg/cpp-examples/raw/master/Holmes.txt))
2.  Load that ascii file.
3.  Count the number of occurrences of unique words (longer than 4
    characters and split hyphenated words, treating each part as
    different words). It should be case and punctuation insensitive. You
    only need to consider the following punctuation characters
    `.,?'"!():` (hint: you will need a backslash escape character for
    the double-quote)
4.  Consider handling of common error cases, such as the wrong file name
    specified. Return error and status information to the user of the
    command line tool.
5.  Print the results to screen showing the unique words and the number
    of uses in descending order of usage, e.g.

<!-- -->

    Word    Usage

    which           55
    holmes          49
    there           32
    could           25
    photograph      21
    ...

## Object Oriented Python Basics

### Reading

- Python tutorial:
  [Classes](https://docs.python.org/3/tutorial/classes.html)
- Python HowTo: [Sorting](https://docs.python.org/3/howto/sorting.html)

### Exercise

The code should be placed in
*exercises-python/firstname_lastname/ex02_oo_basics"*

Write a command line program that:

1.  Has classes to allow number of shapes to be defined: square (side1),
    rectangle(side1, side2), circle(radius), triangle(height, base).
    1.  Each shape class should know it's type ("Square"), how many
        sides it has.
    2.  Each shape needs to be able to calculate it's perimeter and
        area. For the triangle you can assume it is isoceles and the
        perimeter can be computed using $p = b + 2\sqrt{h^2+(b^2/4)}$,
        where $b$ is the base and $h$ is the height.
2.  Within the Main method create a variety of the shapes and put them
    in a list
3.  Create a class ShapeSorter which should contain four methods
    1.  Print out the Shapes that match a chosen type
    2.  Print out the Shapes that match a chosen number of sides
    3.  Print out the Shapes in order of area descending
    4.  Print out the Shapes in order of perimeter descending

## Further reading

Further Python:

- [Think Python](https://greenteapress.com/wp/think-python-2e/)
- Python style guide [PEP8](https://www.python.org/dev/peps/pep-0008/)
