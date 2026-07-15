# Upstream Base

The CaribeOS RV32 port started from:

- Repository: `https://github.com/apple-oss-distributions/xnu.git`
- Release tag: `xnu-2050.48.11`
- Commit: `4abd0e5909cf9dd063df2323306c1f1dffb2a536`
- Upstream commit date: 2013-10-09

At the Tranche 201 preservation point, the working tree was detached from that
tag and was linked to a `blob:none` partial clone whose Git administration
directory lived on another drive. No CaribeOS changes had been committed.

The timestamped pre-edit preservation set contains both the complete working
tree and the original common Git directory. The autonomous CaribeOS repository
starts with a curated import commit so its bundles are complete and clonable
without lazy-fetching missing upstream blobs. The official repository is kept
as the read-only `upstream` remote; no upstream history was rewritten.
