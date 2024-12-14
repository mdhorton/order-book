import gzip
import csv
import struct
import datetime as dt


class ItchParser:
    def __init__(self):
        self.max_order_id = 0

    def run(self):
        mtypes = self._message_types()
        counts = {k: 0 for k in mtypes.keys()}
        total_bytes = 0

        base_path = '/data/nasdaq-itch/12302019.NASDAQ_ITCH50'

        with gzip.open(f'{base_path}.gz', 'rb') as infile:
            with open(f'{base_path}.csv', 'w', newline='') as csvfile:
                with open(f'{base_path}.bin', 'wb') as binfile:
                    csvout = csv.writer(csvfile)

                    while True:
                        if total_bytes % 10_000_000 == 0:
                            print(f'{total_bytes:,}')
                        mtype = infile.read(1)
                        if not mtype:
                            break
                        total_bytes += 1
                        if mtype not in mtypes:
                            continue
                        counts[mtype] += 1
                        msg_size = mtypes[mtype]
                        msg = infile.read(msg_size)
                        fn = getattr(self, f'_handle_msg_type_{mtype.decode()}', None)
                        if callable(fn):
                            row, packed = fn(msg)
                            csvout.writerow(row)
                            binfile.write(packed)
                        total_bytes += msg_size
        print(counts)
        print(f'max_order_id: {self.max_order_id}')

    def _handle_msg_type_A(self, msg):
        msg = struct.unpack('>HH6sQcL8sL', msg)
        msg = list(msg)
        if len(msg) != 8:
            raise Exception('bad record')
        market_id = msg[0]
        tstamp = self._parse_timestamp(msg[2])
        order_id = msg[3]
        bid = msg[4] == b'B'
        qty = msg[5]
        price = msg[7]
        if order_id > self.max_order_id:
            self.max_order_id = order_id
        row = b'A', market_id, tstamp, order_id, bid, qty, price
        packed = struct.pack('<cHQL?LL', *row)
        return row, packed

    def _handle_msg_type_F(self, msg):
        msg = struct.unpack('>HH6sQcL8sL4s', msg)
        msg = list(msg)
        if len(msg) != 9:
            raise Exception('bad record')
        market_id = msg[0]
        tstamp = self._parse_timestamp(msg[2])
        order_id = msg[3]
        bid = msg[4] == b'B'
        qty = msg[5]
        price = msg[7]
        row = b'F', market_id, tstamp, order_id, bid, qty, price
        packed = struct.pack('<cHQL?LL', *row)
        return row, packed

    def _handle_msg_type_E(self, msg):
        msg = struct.unpack('>HH6sQLQ', msg)
        msg = list(msg)
        if len(msg) != 6:
            raise Exception('bad record')
        market_id = msg[0]
        tstamp = self._parse_timestamp(msg[2])
        order_id = msg[3]
        qty = msg[4]
        row = b'E', market_id, tstamp, order_id, qty
        packed = struct.pack('<cHQLL', *row)
        return row, packed

    def _handle_msg_type_C(self, msg):
        msg = struct.unpack('>HH6sQLQcL', msg)
        msg = list(msg)
        if len(msg) != 8:
            raise Exception('bad record')
        market_id = msg[0]
        tstamp = self._parse_timestamp(msg[2])
        order_id = msg[3]
        qty = msg[4]
        printable = msg[6] == b'Y'
        price = msg[7]
        row = b'C', market_id, tstamp, order_id, qty
        packed = struct.pack('<cHQLL', *row)
        return row, packed

    def _handle_msg_type_X(self, msg):
        msg = struct.unpack('>HH6sQL', msg)
        msg = list(msg)
        if len(msg) != 5:
            raise Exception('bad record')
        market_id = msg[0]
        tstamp = self._parse_timestamp(msg[2])
        order_id = msg[3]
        qty = msg[4]
        row = b'X', market_id, tstamp, order_id, qty
        packed = struct.pack('<cHQLL', *row)
        return row, packed

    def _handle_msg_type_D(self, msg):
        msg = struct.unpack('>HH6sQ', msg)
        msg = list(msg)
        if len(msg) != 4:
            raise Exception('bad record')
        market_id = msg[0]
        tstamp = self._parse_timestamp(msg[2])
        order_id = msg[3]
        row = b'D', market_id, tstamp, order_id
        packed = struct.pack('<cHQL', *row)
        return row, packed

    def _handle_msg_type_U(self, msg):
        msg = struct.unpack('>HH6sQQLL', msg)
        msg = list(msg)
        if len(msg) != 7:
            raise Exception('bad record')
        market_id = msg[0]
        tstamp = self._parse_timestamp(msg[2])
        orig_order_id = msg[3]
        new_order_id = msg[4]
        qty = msg[5]
        price = msg[6]
        row = b'U', market_id, tstamp, orig_order_id, new_order_id, qty, price
        packed = struct.pack('<cHQLLLL', *row)
        return row, packed

    def _parse_timestamp(self, tstamp):
        packed = struct.pack('>2s6s', b'\x00\x00', tstamp)
        unpacked = struct.unpack('>Q', packed)
        return unpacked[0]

    def _message_types(self):
        mtypes = dict()
        mtypes[b"S"] = 11  # system event
        mtypes[b"R"] = 38  # stock directory
        mtypes[b"H"] = 24  # stock trade status
        mtypes[b"Y"] = 19  # short restricted indicator
        mtypes[b"L"] = 25  # market participant position
        mtypes[b"V"] = 34  # mwcb decline
        mtypes[b"W"] = 11  # mwcb status
        mtypes[b"K"] = 27  # ipo quote period indicator
        mtypes[b"J"] = 34  # limit up / limit down
        mtypes[b"h"] = 20  # halt
        mtypes[b"A"] = 35  # order add
        mtypes[b"F"] = 39  # order add mpid
        mtypes[b"E"] = 30  # order executed
        mtypes[b"C"] = 35  # order executed with price
        mtypes[b"X"] = 22  # order cancel (partial)
        mtypes[b"D"] = 18  # order delete
        mtypes[b"U"] = 34  # order replace
        mtypes[b"P"] = 43  # trade
        mtypes[b"Q"] = 39  # trade cross
        mtypes[b"B"] = 18  # trade broken
        mtypes[b"I"] = 49  # net order imbalance
        mtypes[b"N"] = 19  # retail price improvment indicator
        mtypes[b"O"] = 47  # direct listing with capital
        return mtypes


if __name__ == '__main__':
    print(dt.datetime.now())
    ItchParser().run()
    print(dt.datetime.now())
