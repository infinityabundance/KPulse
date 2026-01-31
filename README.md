# KPulse
<p align="center">
  <a href="https://github.com/infinityabundance/KPulse">
    <img
      src="assets/kpulse.png"
      alt="KPulse logo"
      width="1000"
    />
  </a>
</p>

<p align="center">
  <a href="https://github.com/infinityabundance/KPulse/actions">
    <img src="https://img.shields.io/github/actions/workflow/status/infinityabundance/KPulse/ci.yml?label=CI&logo=github" alt="CI Status">
  </a>
  <a href="https://github.com/infinityabundance/KPulse/releases">
    <img src="https://img.shields.io/github/v/release/infinityabundance/KPulse?label=release" alt="Latest Release">
  </a>
  <a href="https://github.com/infinityabundance/KPulse/blob/main/LICENSE">
    <img src="https://img.shields.io/badge/license-Apache--2.0-blue.svg" alt="License: Apache-2.0">
  </a>
  <img src="https://img.shields.io/badge/platform-CachyOS%20%2F%20Arch%20Linux-informational" alt="Platform: CachyOS / Arch Linux">
  <img src="https://img.shields.io/badge/language-C%2B%2B-blue" alt="Language: C++">
  <img src="https://img.shields.io/badge/build-CMake-success" alt="Build system: CMake">
</p>

KPulse is a KDE Plasma CachyOS / Archlinux system heartbeat viewer that turns dense system logs
and background activity into a readable, visual timeline.

Instead of digging through `journalctl` output, KPulse shows **when** something
happened and **what else was occurring at the same time**. GPU driver resets,
thermal throttling, service restarts, and background process spikes are
displayed as events aligned on a shared time axis.

KPulse is designed to answer one simple question:

> **What changed, when did it change, and what might have caused it?**

Rather than exposing the system journal directly, KPulse focuses on **signal over noise**, helping you spot meaningful system events at a glance.

---

## 🧠 Key Concepts

- **Semantic Classification**  
  Events are categorized into GPU, Thermal, Network, Process, and System.

- **Noise Gating**  
  Routine systemd chatter and benign informational messages are automatically suppressed.

- **Timeline Visualization**  
  Events are placed on a time axis to make correlations and cause–effect relationships obvious.

- **Structured Storage**  
  Events are stored in a local SQLite database for fast querying and export.

- **Live UI & Tray Integration**  
  Events arrive live via DBus, and a tray app provides quick access and status visibility.

---

## 🚀 Features

- 🕒 Live journald ingestion
- 🔎 Automatic filtering and classification
- 📊 Timeline view with hover and table highlighting
- 💾 Persistent SQLite event storage
- 📋 Copy to clipboard (text / JSON)
- 📁 CSV export
- 🐧 DBus IPC for injection and automation
- 🛠 Tray application (Qt-only fallback; KDE integration optional)

---

## 🔍 How KPulse Compares to Other Journal Viewers

KPulse is **not** intended to be a general replacement for tools like `journalctl` or GUI journal browsers. Instead, it occupies a different layer in the observability stack.

A good point of comparison is [**journal-viewer** by mingue](https://github.com/mingue/journal-viewer).

### Journal Viewer (mingue/journal-viewer)

**Journal Viewer** is a modern, well-designed GUI for browsing the systemd journal.

It excels at:
- Viewing **raw journal entries** directly
- Powerful **filtering and searching** by unit, priority, transport, kernel, etc.
- Showing **log volume graphs** over time
- Acting as a visual alternative to `journalctl`

Its core philosophy is:

> *“Make the systemd journal easier and more pleasant to browse.”*

This makes it an excellent tool for **log inspection and exploration**.

### KPulse

**KPulse takes a different approach.**

Rather than exposing the journal directly, KPulse:
- **Ingests** journald events into a daemon
- **Classifies** them into meaningful categories (GPU, Thermal, Network, Process, System)
- **Filters noise automatically** (systemd accounting spam, benign info messages, known background chatter)
- **Persists only relevant events** into a structured SQLite database
- Presents them as a **semantic timeline of system health**, not a raw log stream

Its core philosophy is:

> *“Surface only the events that matter, when they matter, in a form humans can reason about.”*

### Key Differences at a Glance

| Aspect | Journal Viewer | KPulse |
|------|---------------|--------|
| Primary Goal | Browse raw systemd logs | Understand system health events |
| Data Shown | All journal entries | Curated, classified events |
| Noise Handling | Manual filtering | Automatic gating & suppression |
| Timeline | Log volume graph | Semantic event timeline |
| Persistence | Ephemeral | SQLite-backed event history |
| Correlation | Manual | Designed for cross-event insight |
| Live Updates | Refresh-based | Live DBus push |
| Export | Not primary focus | CSV & clipboard (text/JSON) |

### When to Use Which

Use **Journal Viewer** when you want:
- Full visibility into the journal
- Deep inspection of arbitrary services
- A visual alternative to `journalctl`

Use **KPulse** when you want:
- A **high-level, readable view of system health**
- To correlate issues like **micro-stutter, GPU hangs, thermal spikes, or background process spikes**
- A timeline that highlights *anomalies*, not routine noise
- Exportable, structured event data for analysis or reporting

### Complementary Tools

These tools are **complementary, not competitors**.

A typical workflow might be:
1. KPulse highlights *that* something went wrong and *when*
2. A journal viewer (or `journalctl`) is used to deep-dive into raw logs if needed

KPulse intentionally sits *above* raw logging — closer to a **system health instrument** than a log browser.

---

## Components

- `kpulse-daemon`
  Background daemon that watches journald and system metrics, writes events
  into an SQLite database, and exposes them over D-Bus.

- `kpulse-ui`
  Qt6 / KDE-friendly GUI that displays the event timeline, table, and details.

- `kpulse-tray`
  KDE tray app using `KStatusNotifierItem` to show current health and quickly
  open the main UI.

---

## 📥 Installation

### Dependencies

Required:
- Qt 6 (Widgets, DBus)
- CMake ≥ 3.20
- SQLite3
- systemd (journald access)

Optional:
- KDE Frameworks 6 (for enhanced tray integration)

```bash
sudo pacman -S cmake ninja base-devel \
    qt6-base qt6-tools qt6-declarative \
    kcoreaddons kstatusnotifieritem \
    systemd-libs sqlite systemd extra-cmake-modules kcoreaddons kio
```

### 🛠️Build:

```bash
git clone https://github.com/infinityabundance/KPulse.git
cd KPulse

cmake -B build -S .
cmake --build build
```

### ▶️ Run

Start everything (daemon + tray + UI) with one command:
```
./build/launcher/kpulse
```

Or start pieces manually:

Start the daemon:
```
./build/daemon/kpulse-daemon
```

Start the UI:
```
./build/ui/kpulse-ui
```

Start the tray app:
```
./build/tray/kpulse-tray
```

### 💡 Usage

Start `kpulse-daemon`

Open KPulse via the UI or tray

Select a time range

Inspect the timeline and event table

Right-click events to copy text or JSON

Export event ranges to CSV

Inject a test event
```
busctl --user call \
  org.kde.kpulse.Daemon \
  /org/kde/kpulse/Daemon \
  org.kde.kpulse.Daemon \
  InjectTestEvent \
  ssss \
  "gpu" \
  "warning" \
  "Manual test event" \
  "{\"source\":\"busctl\",\"value\":1}"
```

---

## 🧩 Contributing

Issues, feature requests, and pull requests are welcome.

---

## 📜 License

Apache 2.0
