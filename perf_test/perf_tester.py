#!/usr/bin/env python

import subprocess


class PerfTester:
    def __init__(self):
        self._test_id = ''

    def run(self):
        versions = ['v11']
        for version in versions:
            self._update_file(version)

    def _update_file(self, version):
        if version < 'v12':
            meta_suffix = ''
            bin_suffix = '-sorted'
            order_add = 'ItchOrderAdd'
            order_replace = 'ItchOrderReplace'
        else:
            meta_suffix = '-reverse-bid'
            bin_suffix = '-sorted-idx-reverse-bid'
            order_add = 'ItchOrderAddIdx'
            order_replace = 'ItchOrderReplaceIdx'

        self._execute("cp -f perf_test.cxx perf_test.cpp")
        self._sed(f"'s/v11/{version}/g'")
        self._sed(f"'s/__VERSION__/{version}/g'")
        self._sed(f"'s/__META_SUFFIX__/{meta_suffix}/g'")
        self._sed(f"'s/__BIN_SUFFIX__/{bin_suffix}/g'")
        self._sed(f"'s/ItchOrderAdd/{order_add}/g'")
        self._sed(f"'s/ItchOrderReplace/{order_replace}/g'")

    def _sed(self, sed):
        self._execute(f"sed -i {sed} perf_test.cpp")

    def _execute(self, cmd):
        subprocess.check_call(cmd, shell=True)


if __name__ == "__main__":
    PerfTester().run()
