#!/usr/bin/python2.6

import os
import re
import sys

from pprint import pprint
from optparse import OptionParser


def parse_args():
    parser = OptionParser()
    parser.add_option('-s', '--source', dest='source', help='use FILE as source file', metavar='FILE')
    parser.add_option('-d', '--dest', dest='dest', help='use FILE as destination file', metavar='FILE')
    opts, args = parser.parse_args()

    if len(sys.argv) == 1:
        parser.print_help()
        return None

    if not os.path.isfile(opts.source):
        parser.error("invalid source file (-s): %s" % opts.source)

    return opts


def source_lines(filename):
    fd = open(filename, 'r')
    try:
        for row in fd:
            row = row.strip()
            if row:
                yield row
    finally:
        fd.close()


def main():
    opts = parse_args()

    data = []
    stack = []
    for line in source_lines(opts.source):
        if line.startswith(';;'):
            continue  # comment
        if 'enddef' in line.lower():
            data.append(stack.pop())
            continue
        tmp = re.split('\s+', line)
        if tmp[0].lower() == 'attr':
            val = int(re.sub('0+', '', tmp[1])[1:]) - 1
            stack.append({'surface_id': val})
        else:
            s = tmp[1:]
            stack[-1][tmp[0]] = s if len(s) > 1 else s[0]
    
    data.sort(key=lambda x: x['surface_id'])

    fd = open(opts.dest or '/tmp/attribute.atr', 'w')
    try:
        fd.write("%s\n" % len(data))
        for d in data:
            color = []
            for i in xrange(len(d['color'])):
                color.append(int(d['color'][i])/255.0)
            fd.write("%s %s %s %s %s %s %s\n" % (d['kd'], d['ks'], d['gs'], d['ka'], color[0], color[1], color[2]))
            fd.write("%s %s %s\n" % (d['kts'], d['eta'], '0.0000'))
    finally:
        fd.close()

    return 0


if __name__ == '__main__':
    sys.exit(main())
