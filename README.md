# ideapad-acpi-tray

This program has 1 purpose - a system tray icon that shows / toggles battery conservation mode on Lenovo Ideapad laptops.

Inspired by [gnome-shell-extension-ideapad][1].

## Compiling

### Prerequisites

- gcc
- make
- libappindicator

On apt systems:
```sh
# apt install libappindicator3-dev make gcc
```

On dnf systems:
```sh
# dnf install libappindicator-devel make gcc
```

### Configuration

There's not much configuration that you can (need to) change.

Check out `config.h` for all the configurations.

### Building
```
make
```

## Running

Run `ideapad-acpi-tray` with `sudo` or `setuid`.
The Makefile uses `setuid` by default, so the program should be runnable as root without `sudo`.

If you want to specify the device ID, you can run:
```sh
$ ideapad-acpi-tray [device ID]
```

## License

```
BSD Zero Clause License

Copyright (c) 2022 takase1121

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
```


[1]: https://github.com/laurento/gnome-shell-extension-ideapad
