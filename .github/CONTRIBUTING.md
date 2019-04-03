The [developer documentation](http://developer.mantidproject.org/) has information on how to participate in the mantid project as a developer. This document is meant to outline the steps for contributing to mantid without becomming a developer. We aspire to have similar guidelines as [github](https://github.com/blog/1943-how-to-write-the-perfect-pull-request).

 1. [Fork](https://help.github.com/articles/fork-a-repo) the repository. *recommended:* Delete all branches other than `master`. This makes it easier to see what is in your fork later.
 2. Clone the repository with the remotes `origin` pointing at your fork as `origin` and `mantidproject/mantid` as `upstream`. This is a [common setup](https://help.github.com/articles/configuring-a-remote-for-a-fork/).
 3. Make changes as you see fit. Please still follow the guidelines for [running the unit tests](http://developer.mantidproject.org/RunningTheUnitTests.html) and the [build servers](http://developer.mantidproject.org/AutomatedBuildProcess.html).
 4. Submit a [pull request](https://help.github.com/articles/using-pull-requests) to this branch. This is a start to the conversation.
 
If you need help, you can go to the [forum](https://forum.mantidproject.org/).

Hints to make the integration of your changes easy (and happen faster):
- Keep your pull requests small
- Watch for the results of continuious integration
- Don't forget your unit tests
- All algorithms need documentation, don't forget the .rst file
- Much of the work in mantid gets publicised the release notes, please add a summary of your work there
- Don't take changes requests to change your code personally
