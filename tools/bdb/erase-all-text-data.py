#!/usr/bin/python
#
# Erases the text of every file in a BDB repository
#

import sys, os

import skel, svnfs

from svnfs_bsddb.db import *

def main():
  if len(sys.argv) == 2:
    dbhome = os.path.join(sys.argv[1], 'db')
    for i in ('', 'uuids', 'revisions', 'transactions', 'representations',
        'strings', 'changes', 'copies', 'nodes'):
      if not os.path.exists(os.path.join(dbhome, i)):
        sys.stderr.write("%s: '%s' is not a valid bdb svn repository\n" %
            (sys.argv[0], sys.argv[1]))
        sys.exit(1)
  else:
    sys.stderr.write("Usage: %s <bdb-svn-repository>\n" % sys.argv[0])
    sys.exit(1)

  print "WARNING!: This program will destroy all text data in the subversion"
  print "repository '%s'" % sys.argv[1]
  print "Do not proceed unless this is a *COPY* of your real repository"
  print "If this is really what you want to do, " \
      "type 'YESERASE' and press Return"
  confirmation = raw_input("Confirmation string> ")
  if confirmation != "YESERASE":
    print "Cancelled - confirmation string not matched"
    sys.exit(0)
  print "Opening database environment..."
  cur = None
  ctx = svnfs.Ctx()
  try:
    ctx.open(DBEnv(), dbhome)
    cur = ctx.nodes_db.cursor()
    nodecount = 0
    newrep = skel.Rep()
    newrep.str = "empty"
    empty_fulltext_rep_skel = newrep.unparse()
    del newrep
    ctx.strings_db['empty'] = ""
    rec = cur.first()
    while rec:
      if rec[0] != "next-key":
        if (nodecount % 10000 == 0) and nodecount != 0:
          print "Processed %d nodes..." % nodecount
        nodecount += 1
        node = skel.Node(rec[1])
        if node.kind == "file":
          rep = skel.Rep(ctx.reps_db[node.datarep])
          if rep.kind == "fulltext":
            if ctx.strings_db.has_key(rep.str):
              del ctx.strings_db[rep.str]
            ctx.reps_db[node.datarep] = empty_fulltext_rep_skel
          else:
            for w in rep.windows:
              if ctx.strings_db.has_key(w.str):
                del ctx.strings_db[w.str]
            ctx.reps_db[node.datarep] = empty_fulltext_rep_skel
      rec = cur.next()
    print "Processed %d nodes" % nodecount
  finally:
    if cur:
      cur.close()
    ctx.close()
  print "Done"

if __name__ == '__main__':
  main()
