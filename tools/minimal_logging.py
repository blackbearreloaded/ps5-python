"""Small logging surface used internally by concurrent.futures on PS5.

The full logging package is not part of the runtime bundle yet. Futures keep
their public Future/Executor behavior; this logger only prevents diagnostic
logging from becoming an import dependency.
"""


class Logger:
    def critical(self, *args, **kwargs):
        pass

    def exception(self, *args, **kwargs):
        pass

    def error(self, *args, **kwargs):
        pass

    def warning(self, *args, **kwargs):
        pass

    warn = warning

    def info(self, *args, **kwargs):
        pass

    def debug(self, *args, **kwargs):
        pass


def getLogger(name=None):
    return Logger()
