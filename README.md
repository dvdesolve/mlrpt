# mlrpt
`mlrpt` is an application originally developed by [Neoklis Kyriazis](http://www.5b4az.org/). `mlrpt` is an universal receiver, demodulator and decoder of LRPT weather satellites images for Linux systems which works in console.

## Requirements
### Hardware
`mlrpt` currently supports [RTL-SDR](https://www.rtl-sdr.com/buy-rtl-sdr-dvb-t-dongles/), [Airspy Mini](https://airspy.com/airspy-mini) and [Airspy R2](https://airspy.com/airspy-r2) dongles only.

### Software
In order to use `mlrpt` you should have `librtlsdr` and/or `libairspy` installed on your system.

To build `mlrpt` be sure to have the following dependencies installed:
- `gcc` (4.8 or higher)
- `make`
- `automake` and `autoconf`

## Installation
Download latest stable release or clone `master` branch directly:
```
git clone https://github.com/dvdesolve/mlrpt.git
cd mlrpt
```

Prepare your build (for example, the following will prepare installation destination for `/usr` instead of `/usr/local`):
```
./autogen.sh
./configure --prefix=/usr
```

Build and install `mlrpt`:
```
make
make install
```

Now you're ready to use `mlrpt`.
