### Description of work

#### Summary of work
<!-- Please provide a short, high level description of the work that was done.
-->

<!-- Why has this work been done? If there is no linked issue please provide appropriate context for this work.
#### Purpose of work
This can be removed if a github issue is referenced below
-->

Fixes #xxxx. <!-- and fix #xxxx or close #xxxx xor resolves #xxxx. One line per issue fixed. -->
<!-- alternative
*There is no associated issue.*
-->

<!-- If the original issue was raised by a user they should be named here. Do not leak email addresses
**Report to:** [user name]
-->

#### Further detail of work
<!-- Please provide a more detailed description of the work that has been undertaken.
-->

### To test:

<!-- Instructions for testing.
There should be sufficient instructions for someone unfamiliar with the application to test - unless a specific
reviewer is requested.
If instructions for replicating the fault are contained in the linked issue then it is OK to refer back to these.
-->

<!-- delete this if you added release notes
*This does not require release notes* because **fill in an explanation of why**
If you add release notes please save them as a separate file using the Issue or PR number as the file name. Check the file is located in the correct directory for your note(s).
-->

<!-- Ensure the base of this PR is correct (e.g. release-next or main)
Finally, don't forget to add the appropriate labels, milestones, etc.!  -->

---

### Reviewer

Please comment on the points listed below ([full description](http://developer.mantidproject.org/ReviewingAPullRequest.html)).
**Your comments will be used as part of the gatekeeper process, so please comment clearly on what you have checked during your review.** If changes are made to the PR during the review process then your final comment will be the most important for gatekeepers. In this comment you should make it clear why any earlier review is still valid, or confirm that all requested changes have been addressed.

#### Code Review

- Is the code of an acceptable quality?
- Does the code conform to the [coding standards](http://developer.mantidproject.org/Standards/)?
- Are the unit tests small and test the class in isolation?
- If there is GUI work does it follow the [GUI standards](http://developer.mantidproject.org/Standards/GUIStandards.html)?
- If there are changes in the release notes then do they describe the changes appropriately?
- Do the release notes conform to the [release notes guide](https://developer.mantidproject.org/Standards/ReleaseNotesGuide.html)?

#### Functional Tests

- Do changes function as described? Add comments below that describe the tests performed?
- Do the changes handle unexpected situations, e.g. bad input?
- Has the relevant (user and developer) documentation been added/updated?

Does everything look good? Mark the review as **Approve**. A member of `@mantidproject/gatekeepers` will take care of it.

### Gatekeeper

If you need to request changes to a PR then please add a comment and set the review status to "Request changes". This will stop the PR from showing up in the list for other gatekeepers.
