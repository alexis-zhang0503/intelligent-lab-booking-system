# Qt/C++ Windows client

This is the C++ graphical version of the intelligent lab reservation assignment. It uses Qt 6 Widgets and CMake.

## Features

- Public student registration and password login
- Maintainer-provisioned administrator path for demonstration
- Forty-device plan with `*` available, `#` reserved, and `!` faulty states
- Assignment-defined hourly reservations from 08:00–17:00
- Student booking, cancellation, and fault reports
- Administrator device maintenance, student list, current-user count, and text export
- Local JSON demo store protected by a short file lock

## Build on Windows

Install Qt 6.4 or newer with the Widgets component and a C++ compiler supported by Qt. Then run:

```powershell
cmake -S qt-client -B qt-client/build
cmake --build qt-client/build --config Release
```

The application stores demo records under the current Windows user's application-data folder. To provision a local demonstration administrator, a maintainer sets `LAB_DEMO_ADMIN_PASSWORD` before launching, then signs in as `admin` with that value. No administrator password is compiled into the application.

## Shared backend

The current Qt client is deliberately runnable without cloud credentials. The production contract is documented in `../backend/supabase/schema.sql`: both the Qt network layer and the web client must call the same atomic reservation function when the Supabase project is configured. Until then, local file locking protects only simultaneous local writes and does **not** provide cross-device realtime behaviour.

## Attendance update

The local client now provides Check in (CST) and Check out actions in My reservations.
Select a reservation to enable the applicable action. Attendance timestamps survive restarts;
check-in is allowed only during the booked hour in Asia/Shanghai. This remains a local demo,
with no automatic synchronization to the web client's Supabase records.
See `../docs/attendance-fix.md` for build instructions and validation limits.
