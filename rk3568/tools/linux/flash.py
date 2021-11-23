#!/usr/bin/env python3

#encoding=utf-8
__author__ = 'addy.ke@rock-chips.com'

import traceback
import os
import sys
import threading
import time
import math
import getopt

class FlashTool:
    def __init__(self, arch):
        self.rootDir = os.getcwd()
        self.outDir = '%s/out/ohos-%s-release' % (self.rootDir, arch)
        self.imageDir = '%s/packages/phone/images' % self.outDir
        self.toolDir = '%s/device/hihope/rk3568/tools' % self.rootDir
        self.flashTool = 'sudo %s/linux/bin/flash.%s' % (self.toolDir, os.uname().machine)

    def QueryFlashMode(self):
        cmd = '%s LD' % self.flashTool
        fd = os.popen(cmd)
        line = fd.read()
        if line.find("Mode=Loader") >= 0:
            print('loader')
        elif line.find("Mode=Maskrom") >= 0:
            print('maskrom')
        else:
            print("none")

    def FlashReboot(self):
        return '%s rd ' % self.flashTool

    def FlashLoader(self):
        return '%s UL %s/MiniLoaderAll.bin' % (self.flashTool, self.imageDir)

    def FlashParameter(self):
        return '%s DI -p %s/parameter.txt' % (self.flashTool, self.imageDir)

    def FlashOne(self, part):
        if part in ('uboot', 'trust'):
            return '%s DI -%s %s/%s.img %s/parameter.txt' % (self.flashTool, part, self.imageDir, part, self.imageDir)
        else:
            return '%s DI -%s %s/%s.img' % (self.flashTool, part, self.imageDir, part)

def help(argv):
    text = 'Usage: ' + argv[0] + 'options [PARAMTER]\n'
    text += '\n'
    text += 'Options:\n'
    text += '  -h, --help                        Print help information\n'
    text += '  -q, --query                       Query device flash mode\n'
    text += '  -u, --uboot                       Flash uboot image\n'
    text += '  -k, --kernel                      Flash boot_linux(kernel) image\n'
    text += '  -s, --system                      Flash system image\n'
    text += '  -v, --vendor                      Flash vendor image\n'
    text += '  -d, --userdata                    Flash userdata image\n'
    text += '  -a, --all                         Flash all images\n'
    text += '\n'
    text += 'e.g.\n'
    text += '  ' + argv[0] + ' -sv\n'
    text += '  ' + argv[0] + ' -a\n'
    text +='\n'

    print(text)

try:
    options,args = getopt.getopt(sys.argv[1:], 'hquksvda', ['help', 'query', 'uboot', 'kernel', 'system', 'vendor', 'userdata', 'all'])

except getopt.GetoptError:
    help(sys.argv)
    sys.exit()

if len(options) == 0:
    help(sys.argv)
    sys.exit()

partList = ['uboot', 'boot_linux', 'system', 'vendor', 'userdata']
tool = FlashTool('arm')

cmds = []
cmds.append(tool.FlashLoader())
cmds.append(tool.FlashParameter())
for option, param in options:
    if option in ('-h', '--help'):
        help(sys.argv)
        sys.exit()
    elif option in ('-q', '--query'):
        tool.QueryFlashMode()
        sys.exit()
    elif option in ('-a', '--all'):
        for part in partList:
            cmds.append(tool.FlashOne(part))
    else:
        if option in ('-u', '--uboot'):
            cmds.append(tool.FlashOne('uboot'))
        if option in ('-k', '--kernel'):
            cmds.append(tool.FlashOne('boot_linux'))
        if option in ('-s', '--system'):
            cmds.append(tool.FlashOne('system'))
        if option in ('-v', '--vendor'):
            cmds.append(tool.FlashOne('vendor'))
        if option in ('-d', '--userdata'):
            cmds.append(tool.FlashOne('userdata'))

cmds.append(tool.FlashReboot())

for cmd in cmds:
    #print(cmd)
    if os.system(cmd) != 0:
        print('\033[0;31mFail to run cmd: %s\033[0m' % cmd)
        break
