# TODO

## WiFi & Networking

- [ ] **Multiple WiFi credentials** — store a list of known networks, try each on startup, auto-switch when signal drops
- [ ] **Connection watchdog** — detect lost WiFi mid-run and reconnect (or fall back to AP mode)
- [ ] **mDNS support** — reach device via `esp32.local` instead of bare IP

## Admin Dashboard

- [ ] **Change password** — update admin password via dashboard (UI card exists, backend missing)
- [ ] **Show current mode** — display whether device is in Station or AP/Hotspot mode

## Server / HTTP

- [ ] **Route parameters** — support `/user/:id` style dynamic segments
- [✅] **DELETE and PATCH direct methods** — only GET, POST, PUT wired up currently
- [ ] **Connection timeout** — dispatcher task has the logic commented out (`server.cpp:271–288`), needs a clean implementation
- [ ] **Chunked / streaming response** — large payloads currently buffered entirely in RAM
- [ ] **CORS middleware** — built-in helper for cross-origin headers

## Security

- [ ] **HTTPS / TLS** — optional TLS wrapper for the TCP socket
- [ ] **Salt Hashin** — Use more modern approach for password hashing

## Developer Experience

- [ ] **OTA updates** — upload new firmware via HTTP endpoint
- [ ] **Structured logging levels** — replace raw `Serial.printf` calls with DEBUG / INFO / WARN levels that can be toggled at compile time
