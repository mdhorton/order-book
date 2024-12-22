# NASDAQ ITCH

https://emi.nasdaq.com/ITCH/

https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHSpecification.pdf

version 2.0
price_maps -> std::unordered_map
orders/sec: 8.2 M

version 2.1
price_maps -> boost::unordered_flat_map
orders/sec: 10.1 M

version 2.2
orders_ -> huge pages 2MB
orders/sec: 12.7 M

version 2.3
orders_ -> huge pages 1GB
orders/sec: 13.3 M

version 2.4
group/sort orders by stock_code
orders/sec: 26.3 M
