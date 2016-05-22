# T-Visor
Real-time hypervisor for ARM

# Example (Run Linux on Cubieboard2)
1. (on host) generate configures and make the project
```
sh ./config.sh cubiebaord2 linux_boot
make
```

1. compile linux-sunxi
```
git clone https://github.com/linux-sunxi/linux-sunxi.git
cd linux-sunxi
git checkout sunxi-3.4
wget https://gist.github.com/garasubo/3a60c509fffaedd058fd/raw/fe22497d93b6d4272c84eac5f73e3e652747cac6/.config
ARCH=arm CROSS_COMPILE=arm-linux-gnueabi- make
```

1. get dtb file
```
wget https://gist.github.com/garasubo/bf0acefc5a367fe5e594/raw/00078dcb7b5fda5ad0bf689c7cbc4d295f643788/dtb_virt.dts
```

1. (on cubieboard2) load `main_cubieboard2_linux_boot` at 0x72000000
    *  Download it via tftp server using U-Boot console is a easy way.

1. load linux zImage at 0x40008000 and dtb at 0x4eff7000

1. jump to 0x72000000 like this:
```
go 72000000
```

