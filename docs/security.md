# Security model

- RDP users are separate from Linux/PAM users.
- Password files contain libsodium Argon2id verifiers and are mode 0600.
  KRdp’s protocol secret is stored separately in KWallet/Secret Service, never
  in the KHeadless configuration file or process arguments.
- A 3072-bit self-signed TLS certificate is generated on first start unless an
  administrator configures an imported certificate and key.
- The D-Bus API is on the target user’s session bus.
- The OCI service is unprivileged, capability-free, read-only, and receives
  only explicit session socket mounts.
- Incoming TCP 3389 should be restricted to a trusted LAN or VPN.

Do not put passwords in service environment files, Compose YAML, shell history,
or command arguments. `kheadlessctl passwd USER` reads from standard input.

The default address is `0.0.0.0` for Windows and FreeRDP interoperability.
Binding broadly is not equivalent to authorizing broad firewall access.
