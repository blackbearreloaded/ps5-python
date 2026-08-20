import os
import platform
import sys
import time


def main():
    print("System report")
    print("python: {0}".format(sys.version.split()[0]))
    print("platform: {0}".format(platform.platform()))
    print("pid: {0}".format(os.getpid()))
    print("cwd: {0}".format(os.getcwd()))
    print("time: {0}".format(time.strftime("%Y-%m-%d %H:%M:%S")))
    print("arguments: {0}".format(sys.argv[1:]))


if __name__ == "__main__":
    main()
