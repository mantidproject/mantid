# Planning Documents

This directory contains planning and research documents for major Mantid project initiatives.

## Jenkins to GitHub Actions Migration

**Document**: [jenkins-to-github-actions-migration.md](jenkins-to-github-actions-migration.md)

**Related Issue**: [#39497](https://github.com/mantidproject/mantid/issues/39497)

**Status**: Research complete, ready for review and implementation planning

### Summary

This comprehensive research document analyzes the current Jenkins CI/CD infrastructure and provides detailed recommendations for migrating to GitHub Actions.

**Key Findings:**
- Current setup: 2 Jenkinsfiles, ~20 build scripts
- Migration timeline: 10-12 weeks in 5 phases
- Runner requirements: Self-hosted for all platform builds
- Benefits: Native GitHub integration, modern syntax, reduced infrastructure

**What's Included:**
- Complete inventory of current Jenkins jobs
- GitHub Actions workflow examples for all job types
- Security considerations and best practices
- Risk mitigation strategies
- Step-by-step migration plan

### How to Use This Research

1. **Review**: Read the full document to understand the migration scope
2. **Discuss**: Use the open questions section to guide team discussions
3. **Plan**: Use the phased migration plan to create implementation timeline
4. **Execute**: Follow the recommendations and examples when implementing

### Next Steps

To proceed with migration:
1. Review and approve the migration approach
2. Allocate resources for self-hosted runner setup
3. Create tracking issues for each migration phase
4. Begin Phase 1 (Preparation)

---

For questions or feedback, please comment on issue #39497.
