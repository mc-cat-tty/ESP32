# MicroPython

uPython for ESP32 code examples

## Timer Interrupt
See:
- https://docs.micropython.org/en/latest/library/machine.Timer.html
- https://techtutorialsx.com/2017/10/07/esp32-micropython-timer-interrupts/

Don't forget to disable (and then re-enable) *IRQ* while you are accessing a shared resource, this will prevent race conditions

Instructions:
```python
state = machine.disable_irq()
CENTS += 1  # Shared resource access
machine.enable_irq(state)
```


# MicroPython overview

## Introduction

Official website: [MicroPython - Python for microcontrollers](https://micropython.org/)

### What is MicroPython?

> MicroPython is a lean and efficient implementation of the Python 3 programming language that includes a small subset of the Python standard library and is optimised to run on microcontrollers and in constrained environments.

*MicroPython* is sometimes called *uPython* or shortened to *MPY*

### Features

- REPL **interactive prompt**

    *MicroPython* provides an interactive Python interpreter, able to execute runtime commands. It runs on the bare-metal, without the need for additional components (like an OS)

    REPL - Read Evaluate Print Loop - prompt is an useful tool for instructions prototyping and testing

    Autocompletion and suggestions are available while you're typing (triggered by TAB)

- **Compact**

    The entire firmware is compact enough to fit and run within just 256kb of code space and 16Kb or RAM memory

- **Portable**

    *MicroPython* aims to be as compatible as possible with standard CPython (v.3); it indeed supports closures (nested functions), list comprehension, generators, exception handling and more

## Getting started

### Installation

[Getting started with MicroPython on the ESP32 - MicroPython 1.13 documentation](https://docs.micropython.org/en/latest/esp32/tutorial/intro.html)

1. Download the firmware

    [MicroPyhton firmware download](https://micropython.org/resources/firmware/esp32-idf3-20200902-v1.13.bin)

2. Intall *esptool*

    ```bash
    sudo pip3 install esptool
    ```

3. Erase flash memory

    ```bash
    sudo python3 -m esptool --port /dev/ttyUSBx erase_flash
    ```

4. Flashare the firmware

    ```bash
    python3 -m esptool --chip esp32 --port /dev/ttyUSBx write_flash -z 0x1000 binary_firmware_path
    ```

### Testing

Connect to the serial port of your MCU, here you should find the interactive prompt

## Interactive prompt

### Linux

#### Serial port identification

We are going to find out which is the serial port opened by our ESP

```bash
sudo dmesg
```

Log related to the connection of USB-to-serial cp210x chip

![log_cp210x.png](log_cp210x.png)

After the connection, a file will be created. Its name should be similar to */dev/ttyUSBx*

*x* is an integer that depends on the number of connected USB devices

#### Open serial port

Run one of the following commands

```bash
sudo apt install screen
sudo screen /dev/ttyUSBx 115200
```

Exit with: **Ctrl + a**, then **d**

```bash
sudo apt install python3-serial
sudo miniterm /dev/ttyUSBx 115200 --raw
```

Exit with: **Ctrl** + **]** ⇒ **Ctrl** + **Alt Gr** + **]**

### Windows

Run putty and configure parameters from the GUI

```powershell
putty
```

### Run and import scripts

[MicroPython Basics: Load Files & Run Code](https://www.digikey.com/en/maker/projects/micropython-basics-load-files-run-code/fb1fcedaf11e4547943abfdd8ad825ce)

```bash
sudo pip3 install adafruit-ampy
ampy -p /dev/ttyUSBx -b 115200 run *scriptname.py*
```

This command doesn't work very well. See *main.py*

### Startup script

[MicroPython Basics: Load Files & Run Code](https://learn.adafruit.com/micropython-basics-load-files-and-run-code/boot-scripts)

#### Scripts execution order

1. *boot.py* → debug logs and WebREPL configuration
2. *main.py* → Run after *boot.py* (if exists)

#### Import main script

Run in your desktop:

```bash
ampy -p /dev/ttyUSBx -b 115200 put scriptname.py main.py
```

*main.py* is the name of a special script: it is run after each boot (soft and hard boot)

To see stdout open serial connection and press RST button (or fire Ctrl+D to start soft boot):

```bash
sudo miniterm /dev/ttyUSBx 115200 --raw
```

Press Ctrl+C to get back interactive prompt

#### Main script deletion

Run in your desktop:

```bash
sudo ampy -p /dev/ttyUSBx -b 115200 rm main.py
```

### Documentation

[Quick reference for the ESP32 - MicroPython 1.13 documentation](https://docs.micropython.org/en/latest/esp32/quickref.html#)

## Filesystem

[3. The internal filesystem - MicroPython 1.8.2 documentation](http://docs.micropython.org/en/v1.8.2/esp8266/esp8266/tutorial/filesystem.html)

*MicroPython* implements a Unix-like filesystem (VFS - Virtual File System -)

You can navigate through files and directories using *os* module or *rshell* tool

```python
import os
```

Just as in a real filesystem, the following operations are available:

- Read a file

    ```python
    print(open("filename").read())
    ```

- Write on a file

    ```python
    write(open("filename", "wa").write("text"))
    ```

- Delete a file

    ```python
    os.remove('filename')
    ```

- Create a dir

    ```python
    os.mkdir("dirname")
    ```

- Delete a dir

    ```python
    os.rmdidr('dirname')
    ```

- List a dir

    ```python
    os.listdir('path')
    ```

- Change file/folder name

    ```python
    os.rename('oldname', 'newname')
    ```

## rshell

[dhylands/rshell](https://github.com/dhylands/rshell)

### Installation

```bash
sudo pip3 install rshell
```

### Configuration

```bash
sudo usermod -a -G dialout $USER
```

```bash
sudo chown $USER /dev/ttyUSBx
```

### Open connection

```bash
rshell -p /dev/ttyUSBx -b 115200
```

Now *rshell* connection is opened, but we are not able to browse the virtual filesystem yet.

With ESP32 boards flash memory is mounted on */pyboard* and not */flash*

### Commands

#### Filesystem browsing

First of all, move into *MicroPython* VFS:

```bash
> cd /pyboard
```

These commands will act inside ESP32 VSF (implmented by *uPython)*

```bash
/pyboard> ls -l
/pyboard> mkdir test_dir
/pyboard> ls
/pyboard> cd test_dir
/pyboard/test_dir> ls
/pyboard/test_dir> echo "test_text" > test_file
/pyboard/test_dir> ls
/pyboard/test_dir> cat test_file
/pyboard/test_dir> cd ..
/pyboard> rm -rf test_dir
/pyboard> ls
```

To see all available commands:

```bash
help
```

#### Transfer files from host to ESP32

*rshell* is able to copy files from one filesystem to the other

```bash
/pyboard> cp boot.py /home/$USER
```

#### Exit

Launch:

```bash
exit
```

## WebREPL

> WebREPL (REPL over WebSockets, accessible via a web browser) is an experimental feature available in ESP32 port.

*rshell* web version

[MicroPython WebREPL](http://micropython.org/webrepl/)

[micropython/webrepl](https://github.com/micropython/webrepl)
