# NASDAQ ITCH

https://emi.nasdaq.com/ITCH/

https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHSpecification.pdf

sudo apt install libfmt-dev

cpupower frequency-info -o proc
sudo cpupower --cpu 23 frequency-set --governor performance

sudo vim /etc/default/grub
GRUB_CMDLINE_LINUX="transparent_hugepage=never default_hugepagesz=1G hugepagesz=1G hugepages=24 hugepagesz=2M hugepages=12288 mitigations=off isolcpus=21-23 rcu_nocbs=21-23 nohz=on nohz_full=21-23 irqaffinity=0-20
sudo update-grub
