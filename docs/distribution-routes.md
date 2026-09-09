# Distribution routes outside the VCV Library

Researched 2026-09-09. The official library route is closed: the GitHub account is
blocked from VCVRack/library (HTTP 403 on issue creation), two submission threads
are locked, and several emails went unanswered.

Two of the four channels investigated are real for this plugin. Two are not.

## Cardinal: viable, discretionary

The open-source Rack distribution maintained by DISTRHO/falkTX, shipping as CLAP,
VST3 and LV2. Actively maintained: release 26.02 on 2026-02-28, on a monthly to
bimonthly cadence.
https://github.com/DISTRHO/Cardinal/releases

Licensing fits, and this is the part that usually disqualifies people:

- Modules are statically linked into one GPLv3+ binary, so the most restrictive
  code licence wins.
- The candidates wiki requires GPL-3.0-or-later compatibility. GPL-3.0-only is
  explicitly NOT allowed. Count Modula is listed as incompatible for exactly that.
- Veridical is GPL-3.0-or-later, so the "or later" is doing real work here.
https://github.com/DISTRHO/Cardinal/blob/main/docs/LICENSES.md
https://github.com/DISTRHO/Cardinal/wiki/Possible-modules-to-include

Technical constraints, all of which Veridical already satisfies:

- Network access is strictly forbidden. Veridical makes none.
- Modules run on the host audio thread; Cardinal does not spawn worker threads.
  Veridical spawns none.
- Dependencies must stay minimal. Veridical has none beyond the Rack SDK.
https://github.com/DISTRHO/Cardinal/blob/main/docs/DIFFERENCES.md

Process: there is no submission form. The FAQ says to request inclusion rather
than adding it yourself. Candidates are tracked on the wiki page above, annotated
by licence compatibility. Discussion #63 is the historical thread.
https://github.com/DISTRHO/Cardinal/discussions/63

Accepted or pending examples: Axioma, Lilac Loop, RJ Modules, ComputersCare.
Refused examples and their reasons: HC-One (too niche), a Milkdrop visualiser
(binary size and dependencies), Dumbwaiter (needed network access to an iOS app),
anything non-commercial-licensed.

Inclusion is permanent in practice. A maintainer comment on #63 explains the
caution: once 1.0 is tagged, modules cannot be removed no matter how badly they
turn out. That is why the bar is high and the queue is slow.

### The one thing to settle first

Artwork licence, separate from code licence. Cardinal needs panel artwork that can
be redistributed and modified, ideally including derivative dark variants. GPL on
the code does not automatically cover SVG panels, and other requests have stalled
on exactly this. Veridical ships both dark and light panels already, which helps,
but the artwork licence should be stated explicitly before asking.

UNVERIFIED: whether Cardinal expects a module to have been in the official VCV
Library first. Nothing states this either way, and it matters here.

## AUR: viable, precedented

Third-party VCV plugins are packaged individually on the Arch User Repository
today, for example vcvrack-computerscare and vcvrack-audible-instruments. Anyone
can submit and maintain a package; no VCV or Cardinal approval is involved. A
PKGBUILD pointing at the GitHub release tarball is the normal shape.

UNVERIFIED: exact naming conventions and current maintainers. Every fetch of
aur.archlinux.org and wiki.archlinux.org returned an Anubis anti-bot block on
2026-09-09, so this needs a human browser session to confirm.

## GitHub Topics: free, immediate, self-service

Tagging the repo with vcv-rack, vcv-rack-plugins and vcv-rack-modules puts it in
GitHub's own discovery index, alongside countmodula/VCVRackPlugins,
ValleyAudio/ValleyRackFree and others. No approval from anyone.
https://github.com/topics/vcv-rack-plugins

## Patchstorage: not a distribution channel

It hosts .vcv patch files, not plugins. The VCV platform page has a patch upload
flow and patch categories only. Patchstorage does host real plugin binaries for
other platforms via its LV2 uploader tooling, so the data model could represent
installable software, but nothing indicates that applies to the VCV entry.

Useful for discovery only: demo patches showcasing the five modules would be a
legitimate use, but that is publicity, not distribution.

Confidence: inferred from site structure. No explicit policy statement found.

## ModularGrid: no route found

Its VCV integration maps physical hardware to authorised virtual clones of that
hardware. Veridical is original, not a clone, so there is no hardware entry to
link from. A general "Submit a Module" form exists, but nothing documents it as a
path for original VCV plugins.

The VCV community thread asking for a ModularGrid-like site for Rack modules
exists precisely because ModularGrid was not seen as fitting this purpose.

Confidence: medium. This is a negative finding, which is weaker than a positive
confirmation. No ModularGrid policy page states it in their own words.

## Also worth knowing

KVR Audio catalogues installable products, so a module that only runs inside a
host probably does not fit. Not confirmed either way.

audiopluginsforfree.com has a general freeware submission form and a VCV modules
page, though that page links out to the official library rather than maintaining
its own database.

The VCV community forum at community.vcvrack.com is a separate system from the
GitHub repo where the account is blocked, and developers there routinely announce
release-only plugins. Whether the block extends to Discourse is UNVERIFIED and was
deliberately not investigated.
