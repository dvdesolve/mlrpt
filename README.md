# mlrpt
`mlrpt` is an application originally developed by [Neoklis Kyriazis](http://www.5b4az.org/). `mlrpt` is an universal receiver, demodulator and decoder of LRPT weather satellites images for Linux systems which works in console.

## Requirements
### Hardware
`mlrpt` uses [SoapySDR](https://github.com/pothosware/SoapySDR) library to communicate with SDR hardware. So in principle any hardware supported by SoapySDR should work. However, only [RTL-SDR](https://www.rtl-sdr.com/buy-rtl-sdr-dvb-t-dongles/), [Airspy Mini](https://airspy.com/airspy-mini) and [Airspy R2](https://airspy.com/airspy-r2) units were tested quite well.

### Software
In order to use `mlrpt` you should have the following dependencies satisfied:
- `SoapySDR` (install proper modules to get support for your hardware)
- `libjpeg-turbo`
- `libconfig`

To build `mlrpt` be sure to have the following dependencies installed:
- `gcc` (4.8 or higher)
- `make`
- `cmake` (3.12 or higher)

## Installation
First of all check if `mlrpt` is already in your distro repository. For example, on Arch Linux you can install it [from AUR](https://aur.archlinux.org/packages/mlrpt/). If there is no package for your distro then you must compile it by hands.

### Building from source code
Download latest stable release or clone `master` branch directly:
```
git clone https://github.com/dvdesolve/mlrpt.git
cd mlrpt
```

Prepare your build (for example, the following will install `mlrpt` into `/usr` instead of `/usr/local`):
```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
```

Build and install `mlrpt`:
```
make
make install
```

Now you're ready to use `mlrpt`.

## Usage

### Config files
`mlrpt` comes with a set of sample config files that are usually stored in `/usr/share/mlrpt/config`. `mlrpt`'s config files use [libconfig](http://hyperrealm.github.io/libconfig/) syntax. Example config files are well-documented so consult them for details.

You can specify config file with command-line option or store default one as `$HOME/.mlrptrc`.

Maximum length of string parameters in config files is limited to the 80 characters. Please use ASCII symbols only in strings! Also there are a bunch of mandatory options that should be presented in config files. They are listed below.

#### SDR receiver frequency (kHz)
Option name: `freq`; option group: `receiver`; valid values: `136000 <= freq <= 138000`.

#### QPSK modulation mode
Option name: `mode`; option group: `demodulator`; valid values: `"QPSK", "DOQPSK", "IDOQPSK"`.

#### QPSK transmission symbol rate (Sym/s)
Option name: `rate`; option group: `demodulator`; valid values: `50000 <= rate <= 100000`.

#### Active APIDs
Option name: `apids`; option group: `decoder`; valid values: `[64-69, 64-69, 64-69]`.

All missing/invalid optional settings will be set to their defaults. So minimal working configuration would be like that (Meteor-M2 example):
```
receiver: {
        freq = 137100
}

demodulator: {
        mode = "QPSK"
            rate = "72000"
}

decoder: {
        apids = [66, 65, 64]
}
```

### Command-line options

#### `-c <config-file>`
Specify which configuration file to use.

#### `-s <HHMM-HHMM>`
Start and stop operation time in HHMM format.

#### `-q`
Run in quiet mode (no messages printed).

#### `-i`
Flip images (useful for South to North passes).

#### `-h`
Print usage information and exit.

#### `-v`
Print version information and exit.

### Decoding images
Use [GPredict](https://github.com/csete/gpredict) to get pass time list for the satellite of interest. Connect your SDR receiver and run `mlrpt`. Wait until satellite rises over the horizon - PLL should lock after some time. When the flight is over or you decided to stop just close `mlrpt`. Decoded images will be saved into `$XDG_CACHE_HOME/mlrpt` (or in `$HOME/.cache/mlrpt` if `$XDG_CACHE_HOME` is not set).

## Troubleshooting

### `mlrpt` fails to initialize device
`mlrpt` requires connected SDR before starting up. Connect your receiver and restart `mlrpt`.

### PLL never locks
Try to play with gain settings in config file and find reasonable value to get highest SNR. Also you can try to increase filter bandwidth to somewhat higher value such as 140 kHz. One more thing you could try is to increase PLL lock threshold.

### Poor signal quality
Be sure to properly install your antenna. [V-dipole](https://lna4all.blogspot.com/2017/02/diy-137-mhz-wx-sat-v-dipole-antenna.html) setup in contrast with traditional QFH and turnstile gives you extra advantage that terrestrial broadcasting will be attenuated by ~20 dB. You can try also to switch to manual gain setting and reduce it until you get decent SNR.

### My images are rotated upside-down!
If you've decoded South-to-North satellite pass you will end up with inverted image. Use invert feature that `mlrpt` provides.

## Additional information

### Current Meteor status
You can monitor current status of Meteor satellites and operational characteristics [here](http://happysat.nl/Meteor/html/Meteor_Status.html).

### More info about Meteor-M2 satellite
[This page](https://directory.eoportal.org/web/eoportal/satellite-missions/m/meteor-m-2) contains quite detailed characteristics of the Meteor-M2 mission including description of instruments installed on the satellite and its operational parameters.

### Meteor-3M satellite programme
You could learn about past, current and future missions of Meteor-3M satellite programme [here](https://www.wmo-sat.info/oscar/satelliteprogrammes/view/100).
