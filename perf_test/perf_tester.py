#!/usr/bin/env python

import subprocess

from datetime import datetime as dt


class PerfTester:
    def run(self):
        test_time = dt.now().strftime('%Y-%m-%d_%H-%M-%S.%f')
        build_type = 'release'
        compiler = 'g++-12'
        iters = 10
        always_inline = ""
        inline = ""

        versions = ['v09', 'v10',
                    'v11', 'v12', 'v13', 'v14', 'v15',
                    'v16', 'v17', 'v18', 'v19', 'v20',
                    'v21', 'v22', 'v23']

        self._execute('rm -fr build')

        for version in versions:
            if version < 'v12':
                meta_suffix = ''
                bin_suffix = '-sorted'
                order_add = 'ItchOrderAdd'
                order_replace = 'ItchOrderReplace'
            elif version < 'v17':
                meta_suffix = ''
                bin_suffix = '-sorted-idx'
                order_add = 'ItchOrderAddIdx'
                order_replace = 'ItchOrderReplaceIdx'
            else:
                meta_suffix = '-reverse-bid'
                bin_suffix = '-sorted-idx-reverse-bid'
                order_add = 'ItchOrderAddIdx'
                order_replace = 'ItchOrderReplaceIdx'

            self._update_file(version, order_add, order_replace, always_inline, inline)
            self._rebuild(build_type, compiler)
            self._run_test(test_time, compiler, version, meta_suffix, bin_suffix, iters)

    def _run_test(self, test_time, test_id, version, meta_suffix, bin_suffix, iters):
        cmd = f'build/perf_test/perf_test {test_time} {test_id} {version} "{meta_suffix}" "{bin_suffix}" {iters}'
        self._execute(cmd)

    def _rebuild(self, build_type, compiler):
        cmd = f'cmake -S .. -B build -DCMAKE_BUILD_TYPE="{build_type}" -DCMAKE_CXX_COMPILER="{compiler}"'
        self._execute(cmd)
        cmd = f'cmake --build build --target perf_test'
        self._execute(cmd)

    def _update_file(self, version, order_add, order_replace, always_inline, inline):
        self._execute("cp -f perf_test.cxx perf_test.cpp")
        self._sed(f"'s/v11/{version}/g'")
        self._sed(f"'s/ItchOrderAdd/{order_add}/g'")
        self._sed(f"'s/ItchOrderReplace/{order_replace}/g'")
        self._sed(f"'s/__REPLACE__ALWAYS_INLINE__/{always_inline}/g'")
        self._sed(f"'s/__REPLACE__INLINE__/{inline}/g'")

    def _sed(self, sed):
        self._execute(f"sed -i {sed} perf_test.cpp")

    def _execute(self, cmd):
        subprocess.check_call(cmd, shell=True)


if __name__ == "__main__":
    PerfTester().run()
