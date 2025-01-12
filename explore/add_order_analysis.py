#!/bin/env python

import polars as pl


class AddOrderAnalysis:
    def __init__(self):
        pass

    def run(self):
        df = pl.read_csv('/remote/data/nasdaq-itch/12302019.NASDAQ_ITCH50.csv')
        print(df.head())


if __name__ == '__main__':
    AddOrderAnalysis().run()
