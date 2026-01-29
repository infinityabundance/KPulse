# KPulse

KPulse is a KDE Plasma system heartbeat viewer that turns dense system logs
and background activity into a readable, visual timeline.

Instead of digging through `journalctl` output, KPulse shows **when** something
happened and **what else was occurring at the same time**. GPU driver resets,
thermal throttling, service restarts, and background process spikes are
displayed as events aligned on a shared time axis.

## Features

- Journald-backed event collection via a lightweight daemon
- High-level categories (System, GPU, Thermal, Process, Update, Network)
- Visual timeline ("heartbeat") view
- Event table + JSON details
- KDE tray icon showing recent system health at a glance

## Components

- `kpulse-daemon`
  Background daemon that watches journald and system metrics, writes events
  into an SQLite database, and exposes them over D-Bus.

- `kpulse-ui`
  Qt6 / KDE-friendly GUI that displays the event timeline, table, and details.

- `kpulse-tray`
  KDE tray app using `KStatusNotifierItem` to show current health and quickly
  open the main UI.

## Building

KPulse uses CMake, Qt6, KDE Frameworks 6, and libsystemd.

On Arch / CachyOS, you can install dependencies roughly like:

```bash
sudo pacman -S cmake ninja base-devel \
    qt6-base qt6-tools qt6-declarative \
    kcoreaddons kstatusnotifieritem \
    systemd-libs
```

Then build:

```bash
git clone https://github.com/infinityabundance/KPulse.git
cd KPulse

cmake -B build -S .
cmake --build build
```

This will produce:

- `build/daemon/kpulse-daemon`
- `build/ui/kpulse-ui`
- `build/tray/kpulse-tray`

## Running without install

In one terminal, start the daemon:

```bash
./build/daemon/kpulse-daemon
```

In another terminal, start the UI:

```bash
./build/ui/kpulse-ui
```

Optionally, start the tray app:

```bash
./build/tray/kpulse-tray
```

You should see KPulse appear with event timeline, table, and a tray icon.

## Installing locally

For a simple local install under `~/.local/bin`:

```bash
mkdir -p ~/.local/bin
cp build/daemon/kpulse-daemon ~/.local/bin/
cp build/ui/kpulse-ui ~/.local/bin/
cp build/tray/kpulse-tray ~/.local/bin/
```

Make sure `~/.local/bin` is in your `$PATH`.

## Installing via CMake

You can also install KPulse system-wide (or into a prefix) using CMake:

```bash
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
```

This will install the `kpulse-*` binaries into `/usr/local/bin`.

## systemd user service

KPulse is designed to run `kpulse-daemon` as a user service.

Copy the provided unit file:

```bash
mkdir -p ~/.config/systemd/user
cp packaging/kpulse-daemon.user.service ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now kpulse-daemon.user.service
```

This will start `kpulse-daemon` as a user service on login.

You can then simply run:

```bash
kpulse-ui &
kpulse-tray &
```

to use the UI and tray app.

## Debugging

Check that the daemon is running:

```bash
systemctl --user status kpulse-daemon.service
```

Check that the D-Bus service is registered:

```bash
qdbus org.kde.kpulse /org/kde/kpulse/Daemon org.kde.kpulse.Daemon.GetEvents 0 999999999999 []
```

If the UI shows no events, verify that journald has new entries or that the
metrics collector is generating high-load events.
