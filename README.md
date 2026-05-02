# awdl-sitter

Permanently suppresses macOS AWDL (`awdl0`) to eliminate WiFi interference during gaming or other latency-sensitive workloads.

AWDL (Apple Wireless Direct Link) is used by AirDrop, AirPlay, and Handoff. The OS can raise the `awdl0` interface at any time in response to Bluetooth peer discovery or app requests, causing packet loss and latency spikes on the shared WiFi channel.

## How it works

Rather than polling, `awdl-sitter` uses **SCDynamicStore** — the macOS System Configuration framework — to register an event-driven callback on `State:/Network/Interface/awdl0/Link`. The kernel fires this callback the instant `awdl0` is raised. The callback issues a `SIOCSIFFLAGS` ioctl directly to bring the interface back down, with no forking or shell invocation. The process then returns to a `CFRunLoop`, consuming zero CPU until the next event.

It runs as a **launchd daemon** (macOS equivalent of a systemd service), started at boot and restarted automatically on crash.

**Trade-off:** AirDrop and AirPlay to nearby devices will not work while the daemon is running.

## How it differs from other AWDL suppressors

Most existing tools (homebrew-awdl, awdlkiller) use polling — checking `awdl0` every 1-2 seconds. `awdl-sitter` is purely event-driven: it registers a SCDynamicStore callback and sleeps at zero CPU cost until the OS itself notifies it that `awdl0` changed state. No polling, no wasted cycles, sub-millisecond reaction time.

## Build

Requires Xcode Command Line Tools (`xcode-select --install`).

```bash
cc -o awdl-sitter awdl-sitter.c -framework SystemConfiguration -framework CoreFoundation
```

## Install

Copy the binary and plist into place, then load the daemon:

```bash
sudo cp awdl-sitter /usr/local/bin/awdl-sitter
sudo cp com.local.awdl-sitter.plist /Library/LaunchDaemons/com.local.awdl-sitter.plist
sudo launchctl load -w /Library/LaunchDaemons/com.local.awdl-sitter.plist
```

Verify it is running (a PID in the first column means it's alive):

```bash
sudo launchctl list | grep awdl-sitter
```

View logs:

```bash
log show --predicate 'process == "awdl-sitter"' --last 5m
```

## Uninstall

```bash
sudo launchctl unload -w /Library/LaunchDaemons/com.local.awdl-sitter.plist
sudo rm /Library/LaunchDaemons/com.local.awdl-sitter.plist
sudo rm /usr/local/bin/awdl-sitter
```
