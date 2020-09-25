
The aoo, juce, and ff_meters sub-directories here are all managed subrepos
from other github repos, using git-subrepo
(https://github.com/ingydotnet/git-subrepo).

The aoo and JUCE ones are forks of mine in github, the ff_meters is the
upstream author's repo.

This way all the code for the dependencies of sonobus is completely
constained in this git repo, avoiding complexities of dealing with
submodules, but having the benefit of being able to pull/push changes back
from/to them at any time.
