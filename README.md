# MediCore Hospital System

A desktop-based Hospital Management System built with **C++ and SFML**, featuring a graphical user interface for managing patients, doctors, appointments, billing, and prescriptions.

---

## Features

### Patient Portal
- Self-registration with contact validation and password protection
- Book appointments with available doctors by date and time slot
- Cancel pending appointments with automatic refund
- View all appointments, medical records, and billing history
- Pay bills directly from wallet balance
- Top up wallet balance

### Doctor Portal
- View today's and all appointments
- Mark appointments as **Completed** or **No-Show**
- Write prescriptions (medicines + notes) for completed appointments
- View patient history (restricted to patients with completed appointments)

### Admin Panel
- View all patients, doctors, and appointments
- Monitor unpaid bills (with overdue flagging after 7 days)
- Add and remove doctors
- Discharge patients (clears all their records after verifying no unpaid bills or pending appointments)
- View security/audit log
- Generate daily report (revenue, appointment stats, doctor summaries)

---

## Technology Stack

| Component | Details |
|-----------|---------|
| Language | C++17 |
| GUI Library | SFML 2.x |
| Data Storage | Flat `.txt` CSV files |
| Build | Manual / any C++ compiler (MSVC, GCC, Clang) |

---

## Project Structure

```
├── main.cpp                  # Entire source code (single-file architecture)
├── patients.txt              # Patient records (auto-created)
├── doctors.txt               # Doctor records (seeded with 4 defaults)
├── appointments.txt          # Appointment records
├── bills.txt                 # Billing records
├── prescriptions.txt         # Prescription records
├── admins.txt                # Admin credentials (default seeded)
├── security.log              # Audit/security log
└── discharged.txt            # Archived discharged patient records
```

---

## Getting Started

### Prerequisites
- C++17 compatible compiler (MSVC recommended on Windows)
- [SFML 2.x](https://www.sfml-dev.org/download.php) installed and linked

### Build (Example with g++)
```bash
g++ -std=c++17 main.cpp -o medicore -lsfml-graphics -lsfml-window -lsfml-system
```

### Build (MSVC)
Link against: `sfml-graphics.lib`, `sfml-window.lib`, `sfml-system.lib`

### Run
```bash
./medicore
```

All data files are created automatically in the working directory on first run.

---

## Default Credentials

### Admin
| Field | Value |
|-------|-------|
| ID | `1` |
| Password | `admin123` |

### Default Doctors (seeded on first run)
| ID | Name | Specialization | Password |
|----|------|----------------|----------|
| 1 | Ahmed Khan | Cardiology | `doc123` |
| 2 | Sara Malik | Neurology | `doc123` |
| 3 | Bilal Raza | General | `doc123` |
| 4 | Fatima Noor | Dermatology | `doc123` |

---

## How to Use

### As a New Patient
1. Click **New Patient Register** on the main screen
2. Fill in your details — note the **Patient ID** shown after registration
3. Log in with your Patient ID and password
4. **Top Up Balance** first, then book an appointment

### Booking an Appointment
1. Go to **Book Appointment**
2. Search for a doctor by name or specialization
3. Enter the **Doctor ID**, a date (`DD-MM-YYYY`), and a valid time slot
4. Available slots for that doctor/date are shown automatically
5. Click **Book Now** — the consultation fee is held from your balance

### Available Time Slots
`09:00` `10:00` `11:00` `12:00` `14:00` `15:00` `16:00` `17:00`

---

## Architecture Overview

```
Application
├── HospitalSystem          # Core business logic
│   ├── Storage<T>          # Generic fixed-capacity array store
│   ├── Patient / Doctor / Admin / Appointment / Bill / Prescription
│   ├── FileHandler         # CSV read/write for all entities
│   └── Validator           # Input validation rules
└── Screens (SFML GUI)
    ├── MainScreen
    ├── RegisterScreen
    ├── LoginScreen         # Shared for Patient / Doctor / Admin
    ├── PatientMenuScreen
    ├── DoctorMenuScreen
    └── AdminMenuScreen
```

### Key Design Patterns
- **Inheritance**: `Person` → `Patient`, `Doctor`, `Admin`
- **Template class**: `Storage<T>` for all entity collections
- **Operator overloading**: `Patient += amount` for balance top-up; `Appointment ==` for slot conflict detection
- **Screen state machine**: `enum class Screen` drives active screen transitions

---

## Data Validation Rules

| Field | Rule |
|-------|------|
| Contact | Exactly 11 digits |
| Password | Minimum 6 characters |
| Age | 1 – 120 |
| Date | `DD-MM-YYYY`, year 2000–2100 |
| Fee / Amount | Positive number |
| Time Slot | One of the 8 fixed slots |

---

## Limitations

- Single-file flat storage (no database); suitable for small-scale / academic use
- Fixed capacity arrays (`MAX_PATIENTS=300`, `MAX_DOCTORS=100`, etc.)
- No concurrent access support
- Font loaded from system fonts (`arial.ttf`, `segoeui.ttf`, `DejaVuSans.ttf`); must be available on the OS

---

## License

This project is intended for educational purposes.
