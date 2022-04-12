import sys
import os
import argparse
import subprocess

arg_parser = argparse.ArgumentParser(description='')
arg_parser.add_argument('--outdir', type=str, nargs='?', help='binaries output dir')
arg_parser.add_argument('--bytecode', nargs='?', help='output bytecodes')
arg_parser.add_argument('--shader', nargs='?', help='input shader sources')

if __name__ == "__main__":
    args = arg_parser.parse_args(sys.argv[1:])

    spirv = 0
    if args.bytecode == 'spirv':
        spirv = 1

    dxil = 0
    if args.bytecode == 'dxil':
        dxil = 1

    if spirv:
        if not os.path.exists(args.outdir + '/vulkan'):
            os.mkdir(args.outdir + '/vulkan')

        shader_name = args.shader
        output_shader_name = args.outdir + '/vulkan/' + os.path.basename(shader_name).replace('hlsl', 'bin')
        stage = ''

        if 'vert' in shader_name:
            stage = 'vert'
        if 'frag' in shader_name:
            stage = 'frag'
        if 'comp' in shader_name:
            stage = 'comp'

        subprocess.call(['glslangValidator', '-e', 'main', '-V', shader_name, '-o', output_shader_name ])
