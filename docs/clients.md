# RDP clients

## Windows

Use `mstsc.exe`, open **Display**, and select **Use all my monitors for the
remote session**. Window-size changes use the Display Control channel when the
client supports dynamic resolution.

Command-line examples:

```text
mstsc.exe /v:HOST:3389 /multimon
mstsc.exe /v:HOST:3389 /dynamic-resolution
```

## FreeRDP

```sh
xfreerdp3 /v:HOST:3389 /u:USER /multimon /dynamic-resolution /sound /clipboard
```

Use `/monitor-list` to inspect client monitor IDs and `/monitors:...` to select
a subset. Option spellings differ in older FreeRDP releases; check
`xfreerdp /help`.

The stock KRdp compatibility backend is limited to one monitor and does not
expose Display Control or playback audio to KHeadless. `/multimon`,
`/dynamic-resolution`, and `/sound` become active after building the patched
embedded backend described in `patches/krdp/README.md`; diagnostics report the
currently active transport capabilities.

The newest authenticated writable client becomes controller. Older clients
remain viewers receiving graphics and playback audio. Input and bidirectional
text clipboard access belong only to the controller. When the controller
disconnects, the next-newest writable connection is promoted.
