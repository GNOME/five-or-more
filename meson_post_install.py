#!/usr/bin/env python3

import os
import subprocess

sourcedir = os.environ['MESON_SOURCE_ROOT']
hooksdir = os.path.join(sourcedir, '.git', 'hooks')
precommit = os.path.join(hooksdir, 'pre-commit')
if os.path.lexists(precommit):
    os.remove(precommit)
# os.symlink('../../../libgnome-games-support/style-checker', precommit)
