#########################################################################################
#       This python script is used to generate new SpeedBlock patches                   #
#                                                                                       #
#       Usage: Put this in a folder together with the following                         #
#       win             (folder)                                                        #
#       mac             (folder)                                                        #
#       linux           (folder)                                                        #
#       all             (folder)                                                        #
#       changelog       (text file)                                                     #
#                                                                                       #
#       Set the correct version_info path and update_folder path                        #
#       Put the new files for win/mac/linux OS in corresponding folder                  #
#       Put common files in all directory                                               #
#       Put the changelog you want to display with the patch in the changelog file      #
#       To create a new patch with version 0.1.17 run the command:                      #
#       python3 make_patch.py 117                                                       #
#########################################################################################

import json, sys, subprocess, os, pprint, stat
from collections import OrderedDict

# Make sure we have an int value for the new patch

if len(sys.argv) < 2 or not sys.argv[1].isdigit():
	print("usage: python3 make_patch.py [new patch value integer]")
	sys.exit()

# Control variables

ver = sys.argv[1]
version_info = '' #Path to your current version_info.json file
update_folder = '' #Path to folder where the patch should be put

# Load the json data, and create new dictionaries for the new patch

with open(version_info) as data_file:
        data = json.load(data_file)

data[ver] = dict()
data[ver]['win'] = dict()
data[ver]['mac'] = dict()
data[ver]['linux'] = dict()
data[ver]['all'] = dict()

# Get all the MD5-sums for the new files and add them into the dictionary

for f in os.listdir('win'):
	result = subprocess.run(['md5sum', 'win/' + f], stdout=subprocess.PIPE).stdout.decode('utf-8').partition(' ')[0]
	data[ver]['win']['win/' + f] = result

for f in os.listdir('mac'):
        result = subprocess.run(['md5sum', 'mac/' + f], stdout=subprocess.PIPE).stdout.decode('utf-8').partition(' ')[0]
        data[ver]['mac']['mac/' + f] = result

for f in os.listdir('linux'):
        result = subprocess.run(['md5sum', 'linux/' + f], stdout=subprocess.PIPE).stdout.decode('utf-8').partition(' ')[0]
        data[ver]['linux']['linux/' + f] = result

for f in os.listdir('all'):
        result = subprocess.run(['md5sum', 'all/' + f], stdout=subprocess.PIPE).stdout.decode('utf-8').partition(' ')[0]
        data[ver]['all']['all/' + f] = result

# Get changelog from ./changelog and put in dictionary

with open('changelog') as changelog_file:
	changelog = changelog_file.read()

data[ver]['all']['changelog'] = changelog

# Sort the dictionary in int-value order of the patch version

data2 = OrderedDict(sorted(data.items(), key=lambda t: int(t[0])))

# Overwrite the old version_info file with updated and ordered dictionary

with open(version_info, 'w') as data_file:
        data_file.write(json.dumps(data2, indent=4, separators=(',', ':')))

# Move the files into the web-root/update folder and give read permission

for f in os.listdir('win'):
        os.rename('win/' + f, update_folder + 'win/' + f)
        os.chmod(update_folder + 'win/' + f, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH)

for f in os.listdir('mac'):
        os.rename('mac/' + f, update_folder + 'mac/' + f)
        os.chmod(update_folder + 'mac/' + f, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH)

for f in os.listdir('linux'):
        os.rename('linux/' + f, update_folder + 'linux/' + f)
        os.chmod(update_folder + 'linux/' + f, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH)

for f in os.listdir('all'):
        os.rename('all/' + f, update_folder + 'all/' + f)
        os.chmod(update_folder + 'all/' + f, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH)

# Move old changelog files to so that the script will halt if a new one is not written

os.rename('changelog', 'changelog.old')
