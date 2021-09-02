# [rt_mcuboot](https://github.com/iysheng/rt_mcuboot.git)

[![Apache 2.0](https://img.shields.io/badge/License-Apache%202.0-blue.svg)][license]

[license]: https://github.com/iysheng/rt_mcuboot/blob/master/LICENSE

MCUboot for rt-thread port version 1.8.0

MCUboot is a secure bootloader for 32-bit MCUs. The goal of MCUboot is to
define a common infrastructure for the bootloader, system flash layout on
microcontroller systems, and to provide a secure bootloader that enables
simple software upgrades.

## Using MCUboot with RT-Thread
The contents is similar to [MCUboot](https://github.com/mcu-tools/mcuboot.git) as:
```
├── boot
│   ├── bootutil
│   │   ├── include
│   │   ├── rtthread
│   │   ├── SConscript
│   │   └── src
│   └── SConscript
├── ext
│   ├── mbedtls
│   │   ├── include
│   │   ├── library
│   │   └── SConscript
│   ├── SConscript
│   └── tinycrypt
│       ├── lib
│       └── SConscript
├── image_sign.pem
└── SConscript
```
The `bootutil` directory is the core of mcuboot. When you want to add more chip
support with `mcuboot`, you may add the flash chip **flash area layer driver**
to `boot/bootutil/rtthread` dir. The demo code you can refrence 
is `boot/bootutil/rtthread/bootutil_flash_w25q.c`.
## Roadmap
The demo code is run with [ART-PI](https://github.com/RT-Thread-Studio/sdk-bsp-stm32h750-realthread-artpi). A board designed with RT-Thread.

The size of W25Q64 chip on ART-PI is 8MB, and 4K sector size,
the default flash map designed with the demo is:
```
┌─────────────────────┐
│        256KB        │
│image0 primary SLOT  │
├─────────────────────┤
│        256KB        │
│image0 secondary SLOT│
├─────────────────────┤
│                     │
│                     │
│        ...          │
│                     │
│                     │
│                     │
│                     │
└─────────────────────┘
```
You can run the [art_pi_bootloader](https://github.com/RT-Thread-Studio/sdk-bsp-stm32h750-realthread-artpi/tree/master/projects/art_pi_bootloader) demo code with the package `mcuboot` after you do some modify.

The simple diff file is
``` diff
```

After you run the art_pi_bootloader demo success with the package, you may see as:
![](https://s3.bmp.ovh/imgs/2021/09/484037e7a0305c2e.png)

Because there is no image in w25q64 chip, so the output is `[E/main] get rsp failed.`. After you download a app code in image0 primary slot, you may see as. Attention the image boot address must not start at `0x90000000`, becase at the address there is the image bin, and when I test I just build a app boot start at `0x90001000`.

After build your own app, first of all , you should use the [image_tool]() add image header and trailers.
``` bash
python imgtool.py sign --version 1.0.0 --header-size 0x1000 --align 4 --slot-size 0x40000 --pad-header demo.bin demo_header.bin
```

Then you may download the `demo_header.bin` to address 0x0 of the flash chip w25q64. After you reboot, you may get:
![](https://s3.bmp.ovh/imgs/2021/09/91c45c750932949a.png)

Why this happened ? It's because you forget sign the image use the pem key `image_sign.pem`, so let's do as:
``` bash
python imgtool.py sign  -k image_sign.pem --public-key-format hash --align 4 --version 1.0.0 --header-size 0x1000 --align 4 --slot-size 0x40000 --pad-header demo.bin sign_demo.bin
```

Now you may get the file sign_demo.bin, download this file at 0x0 address of the flash chip. This time you will boot the app success as:
![](https://s3.bmp.ovh/imgs/2021/09/903b17014ea9d356.png)

## Support board
|Board|Flash chip|
|---|---|
|ART-PI|W25Q64|

