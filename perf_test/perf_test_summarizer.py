#!/usr/bin/env python

import polars as pl


class PerfTestSummarizer:
    def run(self):
        fpaths = [
            '/tmp/PerfTest-2025-01-19_11-21-07.409009.csv',
            '/tmp/PerfTest-2025-01-19_13-00-50.573746.csv'
        ]
        for fpath in fpaths:
            print(fpath)
            self._run_impl(fpath)

    def _run_impl(self, fpath):
        columns = ['test_id', 'version', 'fname', 'ns', 'count', 'npo']
        df = pl.read_csv(fpath, has_header=False, new_columns=columns)
        df = df.sort(columns[:3])

        groups = []
        for _, grp in df.group_by(['test_id', 'version', 'fname'], maintain_order=True):
            grp = grp.sort('ns')
            grp = grp[1:-1]
            groups.append(grp)

        df = pl.concat(groups, rechunk=True)
        for (test_id, version), grp in df.group_by(['test_id', 'version'], maintain_order=True):
            avg = grp['ns'].sum() / grp['count'].sum()
            print(f'{test_id} {version} {avg:.2f}')


if __name__ == "__main__":
    PerfTestSummarizer().run()
