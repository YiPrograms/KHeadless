# D-Bus API

Service `org.kde.KHeadless1`, object `/org/kde/KHeadless1`, interface
`org.kde.KHeadless1`.

Monitor and connection collections use `aa{sv}`. A monitor dictionary contains
`id`, `name`, `x`, `y`, `width`, `height`, `scale`, `rotation`, `enabled`, and
`primary`.

A connection dictionary contains `id`, `username`, `peerAddress`,
`connectedAt`, `controller`, `readOnly`, and `requestedMonitors`.

Read methods:

- `Status() -> a{sv}`
- `ServerConfiguration() -> a{sv}`
- `Monitors() -> aa{sv}`
- `Connections() -> aa{sv}`
- `Diagnostics() -> a{sv}`
- `Profiles() -> as`
- `Users() -> as`

Mutation methods:

- `ApplyLayout(aa{sv}, temporary: b) -> b`
- `ConfirmLayout() -> b`
- `RevertLayout() -> b`
- `SetMode("follow-client" | "manual" | "profile") -> b`
- `ConfigureServer(a{sv}) -> b`
- `SaveProfile(name: s, aa{sv}) -> b`
- `ApplyProfile(name: s, temporary: b) -> b`
- `DeleteProfile(name: s) -> b`
- `SetPassword(username: s, password: s) -> b`
- `DeleteUser(username: s) -> b`
- `Start() -> b`
- `Stop() -> b`

The service emits state-specific change signals plus `Error(message)` for
asynchronous failures. Mutations are restricted to the owning session bus.
