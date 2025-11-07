### Description of work

<!-- Please provide an outline and reasoning for the work.
If there is no linked issue provide context.
-->

Closes #xxxx. <!-- One line per closed issue. -->

<!-- If issue raised by user. Do not leak email addresses.
**Report to:** [user name]
-->

### To test:

<!-- Include sufficient instructions for someone unfamiliar with the application to test.
Ok to refer back to instructions in the issue.
-->

<!-- REMEMBER:
- Add labels, milestones, etc.
- Ensure the base of this PR is correct (e.g. release-next or main)
- Add release notes in separate file as per ([guidelines](https://developer.mantidproject.org/Standards/ReleaseNotesGuide.html)), or justify their absence:
  *This does not require release notes* because <fill in an explanation of why>
-->

<!--  GATEKEEPER: When squashing, remove the section from HERE...  -->
---

### Reviewer

**Your comments will be used as part of the gatekeeper process.** Comment clearly on what you have checked and tested during your review. Provide an audit trail for any changes requested.

As per the [review guidelines](http://developer.mantidproject.org/ReviewingAPullRequest.html):

- Is the code of an acceptable quality? ([Code standards](http://developer.mantidproject.org/Standards/)/[GUI standards](http://developer.mantidproject.org/Standards/GUIStandards.html))
- Has a thorough functional test been performed? Do the changes handle unexpected input/situations?
- Are appropriately scoped unit and/or system tests provided?
- Do the release notes conform to the [guidelines](https://developer.mantidproject.org/Standards/ReleaseNotesGuide.html) and describe the changes appropriately?
- Has the relevant (user and developer) documentation been added/updated?
- If the PR author isn’t in the `mantid-developers` or `mantid-contributors` teams, add a review comment `rerun ci` to authorize/rerun the CI

### Gatekeeper

As per the [gatekeeping guidelines](https://developer.mantidproject.org/Gatekeeping.html):

- Has a thorough first line review been conducted, including functional testing?
- At a high-level, is the code quality sufficient?
- Are the base, milestone and labels correct?
<!--  GATEKEEPER: ...To HERE  -->
