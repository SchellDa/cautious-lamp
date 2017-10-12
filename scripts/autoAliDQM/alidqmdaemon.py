#/usr/bin/env python3
import argparse
import pyinotify
import re
import subprocess

class ProcessAlibavaTestbeamData(pyinotify.ProcessEvent):

    def __init__(self):
        self.aliFile = 'alibava\d+.dat'
        self.telFile = 'run\d{6}.raw'
        self.curAliRun = -1
        self.curTelRun = -1
        self.finAliRun = False
        self.finTelRun = False

    def process_IN_CREATE(self, event):
        if re.compile(self.aliFile).match(event.name) is not None:
            self.curAliRun = int(re.findall('\d+', event.name)[0])
            print("New Alibava run #", self.curAliRun)

        if re.compile(self.telFile).match(event.name) is not None:
            self.curTelRun = int(re.findall('\d{6}', event.name)[0])
            print("New Telescope run #", self.curTelRun)

    def process_IN_CLOSE_WRITE(self, event):
        if re.compile(self.aliFile).match(event.name) is not None:
            if int(re.findall('\d+', event.name)[0]) == self.curAliRun:
                print("Wrote Alibava run #", self.curAliRun)
                self.finAliRun = True

        if re.compile(self.telFile).match(event.name) is not None:
            if int(re.findall('\d{6}', event.name)[0]) == self.curTelRun:
                print("Wrote Telescope run #", self.curTelRun)
                self.finTelRun = True

        if self.finAliRun and self.finTelRun:
            print("START ANALYZING ALIBAVA RUN {0} AND TELESCOPE RUN {1}"
                  .format(self.curAliRun, self.curTelRun))
            subprocess.Popen("./startDQM.sh {0} {1}"
                             .format(self.curAliRun, self.curTelRun), 
                             shell=True)
            self.finAliRun = False
            self.finTelRun = False
            
        
    def process_default(self, event):
        print(event.pathname, ' -> DEFAULT')

if __name__ == "__main__":

    # Parse arguments
    parser = argparse.ArgumentParser(description='Auto analysis Daemon')
    parser.add_argument('-a','--alibava', nargs='?', type=str,
                        default='alibava', action='store')
    parser.add_argument('-t','--telescope', nargs='?', type=str,
                        default='telescope', action='store')
    parser.add_argument('-o', '--out', nargs='?', type=str,
                        default='output', action='store')
    args = parser.parse_args()

    # Instanciate new WatchManager
    wm = pyinotify.WatchManager()
    handler = ProcessAlibavaTestbeamData()
    notifier = pyinotify.Notifier(wm, handler)
    mask = pyinotify.IN_CLOSE_WRITE | pyinotify.IN_CREATE

    wm.add_watch(args.alibava, mask, rec=True)
    wm.add_watch(args.telescope, mask, rec=True)

    print("===> Start monitoring {0}".format(args.alibava))
    notifier.loop()
