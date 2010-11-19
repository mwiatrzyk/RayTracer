#!/usr/bin/python2.6

import os
import os
import re
import sys
import logging

from pprint import pprint
from optparse import OptionParser

logging.basicConfig(level=logging.DEBUG)

log = logging.getLogger(__name__)


def parse_args():
    """Parse command line arguments and return them. Returns None if script was
    called without arguments."""
    parser = OptionParser()
    parser.add_option('-s', '--source', dest='source', 
        help='source directory', metavar='PATH')
    parser.add_option('-d', '--dest', dest='dest', 
        help='destination directory', metavar='PATH')
    opts, args = parser.parse_args()

    if len(sys.argv) == 1:
        parser.print_help()
        return None

    if not os.path.isdir(opts.source):
        parser.error("source directory does not exist: %s" % opts.source)

    return opts


def walkthrough(path):
    """Traverses through given `path` and yields *.brs, *.cam, *.atr and *.lgt
    files full paths."""
    for fname in os.listdir(path):
        fullpath = os.path.join(path, fname)
        if os.path.isdir(fullpath):
            for p in walkthrough(fullpath):
                yield p
        else:
            p = fname.rfind('.')
            if p == -1:
                continue
            ext = fname[p+1:]
            if ext in ('brs', 'cam', 'atr', 'lgt'):
                yield fullpath


def filelines(fd):
    """Performs common normalization & filtering actions and generates lines
    from given open file."""
    for line in fd:
        line = line.strip()
        if not line:  # skip empty lines
            continue
        if line.startswith(';;') or line.startswith('//'):  # skip comment lines
            continue
        yield re.sub('\s+', ' ', line)


def normalize_brs(source, dest):
    """Copies *.brs file from `source` to `dest` and performs normalizations
    on the fly."""
    src = open(source, 'r')
    dst = open(dest, 'w')
    try:
        for p, line in enumerate(filelines(src)):
            # read number of vertices
            if p == 0:
                line = re.sub('[^0-9.-]', '', line)
                num_vertices = int(line)
                num_triangles = None
                dst.write("%s\n" % num_vertices)

            # read vertices
            elif num_vertices > 0:
                coords = re.split('\s+', line)
                dst.write("%s %s %s\n" % tuple(coords))
                num_vertices -= 1

            # read number of triangles
            elif num_triangles is None:
                line = re.sub('[^0-9.-]', '', line)
                num_triangles = int(line)
                num_parts = num_triangles
                dst.write("%s\n" % num_triangles)

            # read triangles
            elif num_triangles > 0:
                coords = re.split('\s+', line)
                dst.write("%s %s %s\n" % tuple(coords))
                num_triangles -= 1

            # read parts
            elif num_parts > 0:
                if 'parts' in line:
                    continue
                parts = re.split('\s+', line)
                dst.write("%s\n" % (' '.join(parts)))
                num_parts -= len(parts)
    finally:
        src.close()
        dst.close()


def normalize_atr(source, dest):
    src = open(source, 'r')
    dst = open(dest, 'w')
    try:
        used_params = ('kd', 'ks', 'gs', 'ka', 'color', 'kts', 'eta', 'ktd')

        data = {}
        for p in used_params:
            data[p] = []

        for line in filelines(src):
            if line.startswith('Attr') or line.startswith('enddef'):
                continue
            parts = re.split('\s+', line)
            if parts[0] not in used_params:
                continue
            if parts[0] == 'color':
                data[parts[0]].append(tuple(parts[1:]))
            else:
                data[parts[0]].append(parts[1])

        dst.write("%s\n" % len(data['kd']))

        for i in xrange(len(data['kd'])):
            row = []
            for k in used_params:
                if k == 'color':
                    for j in xrange(3):
                        row.append("%.4f" % (float(data[k][i][j]) / 255.0))
                else:
                    try:
                        row.append(data[k][i])
                    except IndexError:
                        log.warning("%s: value for %s is missing in file - setting to 0.0000", source, k)
                        row.append('0.0000')
            dst.write("%s\n" % (' '.join(row)))
    finally:
        src.close()
        dst.close()


def normalize_cam(source, dest):
    src = open(source, 'r')
    dst = open(dest, 'w')
    try:
        for line in filelines(src):
            if line.startswith('Camera') or line.startswith('enddef'):
                continue
            line = re.sub('[A-Za-z]+', '', line).strip()
            if not line:
                continue
            dst.write("%s\n" % line)
    finally:
        src.close()
        dst.close()


def normalize_lgt(source, dest):
    src = open(source, 'r')
    dst = open(dest, 'w')
    try:
        lights = {}
        fixtures = {}
        stack = []
        for line in filelines(src):
            parts = line.split()
            if not stack and (line.startswith('Light') or line.startswith('Fixture')):
                stack.append({tuple(parts): []})
                continue
            elif not stack:
                continue
            elif line.startswith('enddef'):
                data = stack.pop()
                key = data.keys()[0]
                if key[0] == 'Light':
                    lights.setdefault(key, [])
                    for v in data[key]:
                        if v[0] in ('TotalFlux', 'intensity'):
                            lights[key].append(v)
                elif key[0] == 'Fixture':
                    fixtures.setdefault(key, [])
                    for v in data[key]:
                        if v[0] in ('Light', 'Position'):
                            fixtures[key].append(v)
                continue
            k = stack[-1].keys()
            stack[-1][k[0]].append(parts)

        if not lights or not fixtures:
            log.warning("%s: different file format - trying once again...", source)
            
            used_params = ('Position', 'TotalFlux', 'intensity')
            
            lights = {}
            for k in used_params:
                lights[k] = []

            src.close()
            src = open(source, 'r')
            for line in filelines(src):
                parts = line.split()
                if parts[0] in used_params:
                    lights[parts[0]].append(parts[1:])
            dst.write("%s\n" % len(lights[used_params[0]]))
            for i in xrange(len(lights[used_params[0]])):
                dst.write("%s %s %s\n" % (' '.join(lights['Position'][i]), ' '.join(lights['TotalFlux'][i]), ' '.join(lights['intensity'][i])))
            return
        
        dst.write("%s\n" % len(fixtures))
        for f in fixtures.itervalues():
            data = {}
            for atr in f:
                if atr[0] == 'Light':
                    tmp = {}
                    for l in lights[tuple(atr)]:
                        tmp[l[0]] = l[1:]
                    data[atr[0]] = tmp
                else:
                    data[atr[0]] = atr[1:]
            dst.write("%s %s %s\n" % (' '.join(data['Position']), ' '.join(data['Light']['TotalFlux']), ' '.join(data['Light']['intensity'])))
    finally:
        src.close()
        dst.close()


def main():
    opts = parse_args()
    if not opts:
        return 0
    
    log.info("source directory: %s", opts.source)
    log.info("destination directory: %s", opts.dest)

    for path in walkthrough(opts.source):
        rel_source_file = path.replace(opts.source, '')
        abs_dest_dir = os.path.join(opts.dest, os.path.dirname(rel_source_file))
        abs_dest_file = os.path.join(opts.dest, rel_source_file)
        if not os.path.isdir(abs_dest_dir):
            os.makedirs(abs_dest_dir)
        
        log.info("processing file: %s", path)

        if path.endswith('brs'):
            pass #normalize_brs(path, abs_dest_file)
        elif path.endswith('cam'):
            normalize_cam(path, abs_dest_file)
        elif path.endswith('atr'):
            normalize_atr(path, abs_dest_file)
        elif path.endswith('lgt'):
            normalize_lgt(path, abs_dest_file)
        else:
            log.warning("...skipping - unexpected file type")

    """data = []
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
        fd.close()"""

    return 0


if __name__ == '__main__':
    sys.exit(main())
