#define _CRT_SECURE_NO_WARNINGS
#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <algorithm>
#include <cctype>
using namespace std;

// ============================================================
// Constants
// ============================================================
static const int MAX_PATIENTS = 300;
static const int MAX_DOCTORS = 100;
static const int MAX_APPOINTMENTS = 600;
static const int MAX_BILLS = 600;
static const int MAX_PRESCRIPTIONS = 600;
static const int MAX_ADMINS = 10;
static const int MAX_ROWS = 800;
static const int WIN_W = 1150;
static const int WIN_H = 720;
static const int NAV_W = 220;

// ============================================================
//  Utility
// ============================================================
static string myToLower(const string& s){
	string r = s; for (char& c : r) c = (char)tolower((unsigned char)c); return r;
}
static bool myContains(const string& h, const string& n){
	return myToLower(h).find(myToLower(n)) != string::npos;
}
static int myStrToInt(const string& s){
	if (s.empty())return 0;
	int r = 0, i = 0; bool neg = false;
	if (s[0] == '-'){ neg = true; i++; }
	for (; i<(int)s.size() && s[i] >= '0'&&s[i] <= '9'; i++) r = r * 10 + (s[i] - '0');
	return neg ? -r : r;
}
static float myStrToFloat(const string& s){
	if (s.empty())return 0.f;
	float r = 0.f; int i = 0; bool neg = false;
	if (s[0] == '-'){ neg = true; i++; }
	for (; i<(int)s.size() && s[i] >= '0'&&s[i] <= '9'; i++) r = r*10.f + (s[i] - '0');
	if (i<(int)s.size() && s[i] == '.'){
		i++; float d = 0.1f;
		for (; i<(int)s.size() && s[i] >= '0'&&s[i] <= '9'; i++){ r += (s[i] - '0')*d; d *= 0.1f; }
	}
	return neg ? -r : r;
}
static string myIntToStr(int n){
	if (n == 0)return"0";
	string r; bool neg = n<0; if (neg)n = -n;
	while (n>0){ r = (char)('0' + n % 10) + r; n /= 10; }
	return neg ? "-" + r : r;
}
static string myFloatToStr(float f, int dec = 2){
	ostringstream o; o.precision(dec); o << fixed << f; return o.str();
}
static int splitCSV(const string& line, string out[], int maxOut){
	int count = 0; string tok; istringstream ss(line);
	while (count<maxOut&&getline(ss, tok, ',')) out[count++] = tok;
	return count;
}
static void safeLocaltime(const time_t* t, tm* result){
#ifdef _WIN32
	localtime_s(result, t);
#else
	localtime_r(t, result);
#endif
}
static string getTodayStr(){
	time_t t = time(nullptr); tm ti; safeLocaltime(&t, &ti);
	int d = ti.tm_mday, m = ti.tm_mon + 1, y = ti.tm_year + 1900;
	ostringstream o;
	o << (d<10 ? "0" : "") << d << "-" << (m<10 ? "0" : "") << m << "-" << y;
	return o.str();
}
static bool parseDateDMY(const string& s, int& d, int& m, int& y){
	if ((int)s.size()<10 || s[2] != '-' || s[5] != '-')return false;
	d = myStrToInt(s.substr(0, 2)); m = myStrToInt(s.substr(3, 2)); y = myStrToInt(s.substr(6, 4));
	return true;
}
static int dateDiffDays(const string& ds){
	int dd, dm, dy; if (!parseDateDMY(ds, dd, dm, dy))return 0;
	tm t1 = {}; t1.tm_mday = dd; t1.tm_mon = dm - 1; t1.tm_year = dy - 1900;
	time_t t1t = mktime(&t1); time_t now = time(nullptr);
	return(int)(difftime(now, t1t) / 86400.0);
}
static bool isAllDigits(const string& s){
	if (s.empty())return false;
	for (char c : s)if (c<'0' || c>'9')return false;
	return true;
}

// ============================================================
//Validator
// ============================================================
class Validator{
public:
	static const string* getSlots(){
		static const string s[8] = { "09:00", "10:00", "11:00", "12:00", "14:00", "15:00", "16:00", "17:00" };
		return s;
	}
	static bool isValidSlot(const string& sl){
		const string* s = getSlots(); for (int i = 0; i<8; i++)if (s[i] == sl)return true; return false;
	}
	static bool isValidDate(const string& ds){
		int d, m, y; if (!parseDateDMY(ds, d, m, y))return false;
		return d >= 1 && d <= 31 && m >= 1 && m <= 12 && y >= 2000 && y <= 2100;
	}
	static bool isValidContact(const string& c){ return(int)c.size() == 11 && isAllDigits(c); }
	static bool isValidPassword(const string& p){ return(int)p.size() >= 6; }
	static bool isPositiveFloat(const string& s){
		bool hasDot = false, hasDigit = false;
		for (char c : s){
			if (c >= '0'&&c <= '9')hasDigit = true;
			else if (c == '.'&&!hasDot)hasDot = true; else return false;
		}
		return hasDigit&&myStrToFloat(s)>0.f;
	}
	static bool isPositiveInt(const string& s){ return isAllDigits(s) && myStrToInt(s)>0; }
};

// ============================================================
// Storage<T>
// ============================================================
template<typename T>
class Storage{
	T* data; int count, capacity;
public:
	Storage(int cap = 300) :count(0), capacity(cap){ data = new T[cap]; }
	~Storage(){ delete[]data; }
	bool add(const T& item){ if (count >= capacity)return false; data[count++] = item; return true; }
	bool removeById(int id){
		for (int i = 0; i<count; i++)if (data[i].getId() == id){
			for (int j = i; j<count - 1; j++)data[j] = data[j + 1]; count--; return true;
		}
		return false;
	}
	T* findById(int id){ for (int i = 0; i<count; i++)if (data[i].getId() == id)return&data[i]; return nullptr; }
	T* get(int i){ return(i >= 0 && i<count) ? &data[i] : nullptr; }
	T* getAll(){ return data; }
	int size()const{ return count; }
	void clear(){ count = 0; }
	Storage(const Storage&) = delete;
	Storage& operator=(const Storage&) = delete;
};

// ============================================================
// Domain Classes
// ============================================================
class Person{
protected:
	int id; string name, contact, password;
public:
	Person() :id(0){}
	Person(int i, const string& n, const string& c, const string& p) :id(i), name(n), contact(c), password(p){}
	virtual ~Person(){}
	int           getId()      const{ return id; }
	const string& getName()    const{ return name; }
	const string& getContact() const{ return contact; }
	const string& getPassword()const{ return password; }
	void setId(int i){ id = i; }
	virtual void displayInfo()const = 0;
};

class Patient :public Person{
	int age; string gender; float balance;
public:
	Patient() :Person(), age(0), balance(0.f){}
	Patient(int i, const string& n, int a, const string& g, const string& c, const string& p, float b)
		:Person(i, n, c, p), age(a), gender(g), balance(b){}
	int           getAge()    const{ return age; }
	const string& getGender() const{ return gender; }
	float         getBalance()const{ return balance; }
	void setBalance(float b){ balance = b; }
	Patient& operator+=(float a){ balance += a; return*this; }
	Patient& operator-=(float a){ balance -= a; return*this; }
	void displayInfo()const override{
		cout << "ID:" << id << " " << name << " Age:" << age << " " << gender << " " << contact << " Bal:PKR " << myFloatToStr(balance) << "\n";
	}
	string toCSV()const{
		return myIntToStr(id) + "," + name + "," + myIntToStr(age) + "," + gender + "," + contact + "," + password + "," + myFloatToStr(balance);
	}
};

class Doctor :public Person{
	string specialization; float fee;
public:
	Doctor() :Person(), fee(0.f){}
	Doctor(int i, const string& n, const string& sp, const string& c, const string& p, float f)
		:Person(i, n, c, p), specialization(sp), fee(f){}
	const string& getSpecialization()const{ return specialization; }
	float         getFee()           const{ return fee; }
	void displayInfo()const override{
		cout << "ID:" << id << " Dr." << name << " [" << specialization << "] Fee:PKR " << myFloatToStr(fee) << "\n";
	}
	string toCSV()const{
		return myIntToStr(id) + "," + name + "," + specialization + "," + contact + "," + password + "," + myFloatToStr(fee);
	}
};

class Admin :public Person{
public:
	Admin() :Person(){}
	Admin(int i, const string& n, const string& p) :Person(i, n, "", p){}
	void displayInfo()const override{ cout << "Admin:" << name << "\n"; }
	string toCSV()const{ return myIntToStr(id) + "," + name + "," + password; }
};

class Appointment{
	int appointmentId, patientId, doctorId;
	string date, timeSlot, status;
public:
	Appointment() :appointmentId(0), patientId(0), doctorId(0){}
	Appointment(int ai, int pi, int di, const string& d, const string& sl, const string& st)
		:appointmentId(ai), patientId(pi), doctorId(di), date(d), timeSlot(sl), status(st){}
	int           getId()           const{ return appointmentId; }
	int           getAppointmentId()const{ return appointmentId; }
	int           getPatientId()    const{ return patientId; }
	int           getDoctorId()     const{ return doctorId; }
	const string& getDate()         const{ return date; }
	const string& getTimeSlot()     const{ return timeSlot; }
	const string& getStatus()       const{ return status; }
	void setStatus(const string& s){ status = s; }
	bool operator==(const Appointment& o)const{
		if (status == "cancelled" || o.status == "cancelled")return false;
		return doctorId == o.doctorId&&date == o.date&&timeSlot == o.timeSlot;
	}
	string toCSV()const{
		return myIntToStr(appointmentId) + "," + myIntToStr(patientId) + "," + myIntToStr(doctorId) + "," + date + "," + timeSlot + "," + status;
	}
};

class Bill{
	int billId, patientId, appointmentId; float amount; string status, date;
public:
	Bill() :billId(0), patientId(0), appointmentId(0), amount(0.f){}
	Bill(int bi, int pi, int ai, float a, const string& st, const string& d)
		:billId(bi), patientId(pi), appointmentId(ai), amount(a), status(st), date(d){}
	int           getId()           const{ return billId; }
	int           getBillId()       const{ return billId; }
	int           getPatientId()    const{ return patientId; }
	int           getAppointmentId()const{ return appointmentId; }
	float         getAmount()       const{ return amount; }
	const string& getStatus()       const{ return status; }
	const string& getDate()         const{ return date; }
	void setStatus(const string& s){ status = s; }
	string toCSV()const{
		return myIntToStr(billId) + "," + myIntToStr(patientId) + "," + myIntToStr(appointmentId) + "," + myFloatToStr(amount) + "," + status + "," + date;
	}
};

class Prescription{
	int prescriptionId, appointmentId, patientId, doctorId;
	string date, medicines, notes;
public:
	Prescription() :prescriptionId(0), appointmentId(0), patientId(0), doctorId(0){}
	Prescription(int pri, int ai, int pi, int di, const string& d, const string& m, const string& n)
		:prescriptionId(pri), appointmentId(ai), patientId(pi), doctorId(di), date(d), medicines(m), notes(n){}
	int           getId()            const{ return prescriptionId; }
	int           getPrescriptionId()const{ return prescriptionId; }
	int           getAppointmentId() const{ return appointmentId; }
	int           getPatientId()     const{ return patientId; }
	int           getDoctorId()      const{ return doctorId; }
	const string& getDate()          const{ return date; }
	const string& getMedicines()     const{ return medicines; }
	const string& getNotes()         const{ return notes; }
	string toCSV()const{
		return myIntToStr(prescriptionId) + "," + myIntToStr(appointmentId) + "," + myIntToStr(patientId) + "," + myIntToStr(doctorId) + "," + date + "," + medicines + "," + notes;
	}
};

// ============================================================
// File Handler
// ============================================================
class FileHandler{
public:
	static void ensureFile(const string& fn){ ifstream f(fn); if (!f){ ofstream o(fn); } }
	static void initFiles(){
		ensureFile("patients.txt"); ensureFile("doctors.txt"); ensureFile("appointments.txt");
		ensureFile("bills.txt"); ensureFile("prescriptions.txt"); ensureFile("security.log");
		ifstream af("admins.txt"); string line;
		bool has = (af&&getline(af, line) && !line.empty());
		if (!has){ ofstream o("admins.txt"); o << "1,Admin,admin123\n"; }
	}
	static void loadPatients(Storage<Patient>& s){
		ifstream f("patients.txt"); if (!f)return; string line;
		while (getline(f, line)){
			if (line.empty())continue;
			string t[8]; int n = splitCSV(line, t, 8); if (n<7)continue;
			s.add(Patient(myStrToInt(t[0]), t[1], myStrToInt(t[2]), t[3], t[4], t[5], myStrToFloat(t[6])));
		}
	}
	static void savePatients(Storage<Patient>& s){
		ofstream f("patients.txt", ios::trunc); for (int i = 0; i<s.size(); i++)f << s.get(i)->toCSV() << "\n";
	}
	static void loadDoctors(Storage<Doctor>& s){
		ifstream f("doctors.txt"); if (!f)return; string line;
		while (getline(f, line)){
			if (line.empty())continue;
			string t[7]; int n = splitCSV(line, t, 7); if (n<6)continue;
			s.add(Doctor(myStrToInt(t[0]), t[1], t[2], t[3], t[4], myStrToFloat(t[5])));
		}
	}
	static void saveDoctors(Storage<Doctor>& s){
		ofstream f("doctors.txt", ios::trunc); for (int i = 0; i<s.size(); i++)f << s.get(i)->toCSV() << "\n";
	}
	static void loadAdmins(Storage<Admin>& s){
		ifstream f("admins.txt"); if (!f)return; string line;
		while (getline(f, line)){
			if (line.empty())continue;
			string t[4]; int n = splitCSV(line, t, 4); if (n<3)continue;
			s.add(Admin(myStrToInt(t[0]), t[1], t[2]));
		}
	}
	static void loadAppointments(Storage<Appointment>& s){
		ifstream f("appointments.txt"); if (!f)return; string line;
		while (getline(f, line)){
			if (line.empty())continue;
			string t[7]; int n = splitCSV(line, t, 7); if (n<6)continue;
			s.add(Appointment(myStrToInt(t[0]), myStrToInt(t[1]), myStrToInt(t[2]), t[3], t[4], t[5]));
		}
	}
	static void saveAppointments(Storage<Appointment>& s){
		ofstream f("appointments.txt", ios::trunc); for (int i = 0; i<s.size(); i++)f << s.get(i)->toCSV() << "\n";
	}
	static void loadBills(Storage<Bill>& s){
		ifstream f("bills.txt"); if (!f)return; string line;
		while (getline(f, line)){
			if (line.empty())continue;
			string t[7]; int n = splitCSV(line, t, 7); if (n<6)continue;
			s.add(Bill(myStrToInt(t[0]), myStrToInt(t[1]), myStrToInt(t[2]), myStrToFloat(t[3]), t[4], t[5]));
		}
	}
	static void saveBills(Storage<Bill>& s){
		ofstream f("bills.txt", ios::trunc); for (int i = 0; i<s.size(); i++)f << s.get(i)->toCSV() << "\n";
	}
	static void loadPrescriptions(Storage<Prescription>& s){
		ifstream f("prescriptions.txt"); if (!f)return; string line;
		while (getline(f, line)){
			if (line.empty())continue;
			string t[8]; int n = splitCSV(line, t, 8); if (n<7)continue;
			s.add(Prescription(myStrToInt(t[0]), myStrToInt(t[1]), myStrToInt(t[2]), myStrToInt(t[3]), t[4], t[5], t[6]));
		}
	}
	static void savePrescriptions(Storage<Prescription>& s){
		ofstream f("prescriptions.txt", ios::trunc); for (int i = 0; i<s.size(); i++)f << s.get(i)->toCSV() << "\n";
	}
	static void appendLog(const string& entry){
		ofstream f("security.log", ios::app); f << getTodayStr() << " | " << entry << "\n";
	}
	static int readLog(string out[], int maxOut){
		ifstream f("security.log"); int c = 0; string line;
		while (c<maxOut&&getline(f, line))if (!line.empty())out[c++] = line; return c;
	}
	static void appendDischarged(const Patient& p) {
		ofstream f("discharged.txt", ios::app);
		f << getTodayStr() << "," << p.toCSV() << "\n";
	}
};

// ============================================================
// Hospital System 
// ============================================================
class HospitalSystem{
public:
	Storage<Patient>      patients;
	Storage<Doctor>       doctors;
	Storage<Admin>        admins;
	Storage<Appointment>  appointments;
	Storage<Bill>         bills;
	Storage<Prescription> prescriptions;
private:
	int nxtPat, nxtDoc, nxtAppt, nxtBill, nxtPresc;
	void calcNextIds(){
		nxtPat = nxtDoc = nxtAppt = nxtBill = nxtPresc = 1;
		for (int i = 0; i<patients.size(); i++)if (patients.get(i)->getId() >= nxtPat)nxtPat = patients.get(i)->getId() + 1;
		for (int i = 0; i<doctors.size(); i++)if (doctors.get(i)->getId() >= nxtDoc)nxtDoc = doctors.get(i)->getId() + 1;
		for (int i = 0; i<appointments.size(); i++)if (appointments.get(i)->getId() >= nxtAppt)nxtAppt = appointments.get(i)->getId() + 1;
		for (int i = 0; i<bills.size(); i++)if (bills.get(i)->getId() >= nxtBill)nxtBill = bills.get(i)->getId() + 1;
		for (int i = 0; i<prescriptions.size(); i++)if (prescriptions.get(i)->getId() >= nxtPresc)nxtPresc = prescriptions.get(i)->getId() + 1;
	}
public:
	HospitalSystem() :nxtPat(1), nxtDoc(1), nxtAppt(1), nxtBill(1), nxtPresc(1){}
	string getTodayDate()const{ return getTodayStr(); }

	void loadAll(){
		FileHandler::initFiles();
		FileHandler::loadPatients(patients);
		FileHandler::loadDoctors(doctors);
		FileHandler::loadAdmins(admins);
		FileHandler::loadAppointments(appointments);
		FileHandler::loadBills(bills);
		FileHandler::loadPrescriptions(prescriptions);
		calcNextIds();
		if (doctors.size() == 0){
			doctors.add(Doctor(nxtDoc++, "Ahmed Khan", "Cardiology", "03001234567", "doc123", 2000.f));
			doctors.add(Doctor(nxtDoc++, "Sara Malik", "Neurology", "03007654321", "doc123", 2500.f));
			doctors.add(Doctor(nxtDoc++, "Bilal Raza", "General", "03111111111", "doc123", 1500.f));
			doctors.add(Doctor(nxtDoc++, "Fatima Noor", "Dermatology", "03221234567", "doc123", 1800.f));
			FileHandler::saveDoctors(doctors);
		}
	}
	void saveAll(){
		FileHandler::savePatients(patients);
		FileHandler::saveDoctors(doctors);
		FileHandler::saveAppointments(appointments);
		FileHandler::saveBills(bills);
		FileHandler::savePrescriptions(prescriptions);
	}

	Patient* loginPatient(const string& id, const string& pw){
		Patient* p = patients.findById(myStrToInt(id));
		if (p&&p->getPassword() == pw)return p;
		FileHandler::appendLog("Failed patient login ID=" + id); return nullptr;
	}
	Doctor* loginDoctor(const string& id, const string& pw){
		Doctor* d = doctors.findById(myStrToInt(id));
		if (d&&d->getPassword() == pw)return d;
		FileHandler::appendLog("Failed doctor login ID=" + id); return nullptr;
	}
	Admin* loginAdmin(const string& id, const string& pw){
		Admin* a = admins.findById(myStrToInt(id));
		if (a&&a->getPassword() == pw)return a;
		FileHandler::appendLog("Failed admin login ID=" + id); return nullptr;
	}

	// --- Register Patient ---
	string registerPatient(const string& name, const string& ageStr, const string& gender,
		const string& contact, const string& pw, const string& pw2, int& outId){
		if (name.empty())return"Name cannot be empty.";
		if (!Validator::isPositiveInt(ageStr))return"Age must be a positive number.";
		int age = myStrToInt(ageStr);
		if (age<1 || age>120)return"Age 1-120 only.";
		if (gender != "Male"&&gender != "Female"&&gender != "Other")return"Select a gender.";
		if (!Validator::isValidContact(contact))return"Contact must be 11 digits.";
		for (int i = 0; i<patients.size(); i++)
		if (patients.get(i)->getContact() == contact)return"Contact already registered.";
		if (!Validator::isValidPassword(pw))return"Password min 6 chars.";
		if (pw != pw2)return"Passwords do not match.";
		outId = nxtPat;
		patients.add(Patient(nxtPat++, name, age, gender, contact, pw, 0.f));
		FileHandler::savePatients(patients);
		FileHandler::appendLog("Patient registered: " + name);
		return"";
	}

	// --- Patient ---
	string bookAppointment(Patient* p, int docId, const string& date, const string& slot){
		Doctor* doc = doctors.findById(docId);
		if (!doc)return"Doctor not found.";
		if (!Validator::isValidDate(date))return"Invalid date (DD-MM-YYYY).";
		if (!Validator::isValidSlot(slot))return"Invalid time slot.";
		Appointment probe(0, p->getId(), docId, date, slot, "pending");
		for (int i = 0; i<appointments.size(); i++)
		if (probe == *appointments.get(i))return"Slot already taken.";
		if (p->getBalance()<doc->getFee())return"Insufficient balance. Top up first.";
		int aid = nxtAppt++;
		appointments.add(Appointment(aid, p->getId(), docId, date, slot, "pending"));
		bills.add(Bill(nxtBill++, p->getId(), aid, doc->getFee(), "unpaid", getTodayStr()));
		saveAll();
		return"";
	}
	string cancelAppointment(Patient* p, int apId){
		Appointment* a = appointments.findById(apId);
		if (!a || a->getPatientId() != p->getId())return"Appointment not found.";
		if (a->getStatus() != "pending")return"Only pending appointments can be cancelled.";
		a->setStatus("cancelled");
		for (int i = 0; i<bills.size(); i++)
		if (bills.get(i)->getAppointmentId() == apId)bills.get(i)->setStatus("cancelled");
		Doctor* doc = doctors.findById(a->getDoctorId());
		float ref = doc ? doc->getFee() : 0.f;
		*p += ref;
		saveAll();
		return"Cancelled. Refunded PKR " + myFloatToStr(ref);
	}
	string payBill(Patient* p, int bId){
		Bill* b = bills.findById(bId);
		if (!b || b->getPatientId() != p->getId())return"Bill not found.";
		if (b->getStatus() != "unpaid")return"Bill not unpaid.";
		if (p->getBalance()<b->getAmount())return"Insufficient balance.";
		*p -= b->getAmount(); b->setStatus("paid");
		saveAll();
		return"Paid. Balance: PKR " + myFloatToStr(p->getBalance());
	}
	string topUp(Patient* p, const string& amtStr){
		if (!Validator::isPositiveFloat(amtStr))return"Enter a valid positive amount.";
		*p += myStrToFloat(amtStr);
		FileHandler::savePatients(patients);
		return"Balance: PKR " + myFloatToStr(p->getBalance());
	}

	// --- Doctor ---
	string markStatus(Doctor* d, int apId, const string& newSt){
		Appointment* a = appointments.findById(apId);
		if (!a || a->getDoctorId() != d->getId())return"Appointment not found.";
		if (a->getStatus() != "pending")return"Not pending.";
		a->setStatus(newSt);
		if (newSt == "noshow")
		for (int i = 0; i<bills.size(); i++)
		if (bills.get(i)->getAppointmentId() == apId)bills.get(i)->setStatus("cancelled");
		FileHandler::saveAppointments(appointments);
		FileHandler::saveBills(bills);
		return"Done.";
	}
	string writePrescription(Doctor* d, int apId, const string& meds, const string& notes){
		if (meds.empty())return"Medicines cannot be empty.";
		Appointment* a = appointments.findById(apId);
		if (!a || a->getDoctorId() != d->getId())return"Appointment not found.";
		if (a->getStatus() != "completed")return"Appointment not completed.";
		for (int i = 0; i<prescriptions.size(); i++)
		if (prescriptions.get(i)->getAppointmentId() == apId)return"Already written.";
		prescriptions.add(Prescription(nxtPresc++, apId, a->getPatientId(), d->getId(), a->getDate(), meds, notes));
		FileHandler::savePrescriptions(prescriptions);
		return"Saved.";
	}

	// --- Admin  ---
	string addDoctor(const string& name, const string& spec, const string& contact,
		const string& pw, const string& feeStr, int& outId){
		if (name.empty())return"Name empty.";
		if (spec.empty())return"Specialization empty.";
		if (!Validator::isValidContact(contact))return"Contact must be 11 digits.";
		if (!Validator::isValidPassword(pw))return"Password min 6 chars.";
		if (!Validator::isPositiveFloat(feeStr))return"Fee must be positive.";
		outId = nxtDoc;
		doctors.add(Doctor(nxtDoc++, name, spec, contact, pw, myStrToFloat(feeStr)));
		FileHandler::saveDoctors(doctors);
		return"";
	}
	string removeDoctor(int docId){
		for (int i = 0; i<appointments.size(); i++)
		if (appointments.get(i)->getDoctorId() == docId&&appointments.get(i)->getStatus() == "pending")
			return"Doctor has pending appointments.";
		if (!doctors.findById(docId))return"Doctor not found.";
		doctors.removeById(docId); FileHandler::saveDoctors(doctors); return"Removed.";
	}
	string dischargePatient(int pid){
		Patient* p = patients.findById(pid); if (!p)return"Patient not found.";
		for (int i = 0; i<bills.size(); i++)
		if (bills.get(i)->getPatientId() == pid&&bills.get(i)->getStatus() == "unpaid")
			return"Cannot discharge: unpaid bills.";
		for (int i = 0; i<appointments.size(); i++)
		if (appointments.get(i)->getPatientId() == pid&&appointments.get(i)->getStatus() == "pending")
			return"Cannot discharge: pending appointments.";
		FileHandler::appendDischarged(*p);
		patients.removeById(pid);
		for (int i = appointments.size() - 1; i >= 0; i--)
		if (appointments.get(i)->getPatientId() == pid)
			appointments.removeById(appointments.get(i)->getId());
		for (int i = bills.size() - 1; i >= 0; i--)
		if (bills.get(i)->getPatientId() == pid)
			bills.removeById(bills.get(i)->getId());
		for (int i = prescriptions.size() - 1; i >= 0; i--)
		if (prescriptions.get(i)->getPatientId() == pid)
			prescriptions.removeById(prescriptions.get(i)->getId());
		saveAll(); return"Discharged.";
	}
	int countUnpaid(int pid){
		int c = 0; for (int i = 0; i<bills.size(); i++)
		if (bills.get(i)->getPatientId() == pid&&bills.get(i)->getStatus() == "unpaid")c++; return c;
	}
	string availableSlots(int docId, const string& date){
		const string* all = Validator::getSlots(); string res = "Free: "; bool any = false;
		for (int i = 0; i<8; i++){
			Appointment probe(0, 0, docId, date, all[i], "pending"); bool taken = false;
			for (int j = 0; j<appointments.size(); j++)
			if (probe == *appointments.get(j)){ taken = true; break; }
			if (!taken){ res += all[i] + " "; any = true; }
		}
		return any ? res : "No free slots.";
	}
	int dailyReport(string out[], int maxOut){
		string today = getTodayStr();
		int total = 0, pend = 0, comp = 0, ns = 0, can = 0; float rev = 0.f;
		for (int i = 0; i<appointments.size(); i++){
			Appointment* a = appointments.get(i);
			if (a->getDate() == today){
				total++;
				if (a->getStatus() == "completed")comp++;
				else if (a->getStatus() == "pending")pend++;
				else if (a->getStatus() == "cancelled")can++;
				else if (a->getStatus() == "noshow")ns++;
			}
		}
		for (int i = 0; i<bills.size(); i++){
			Bill* b = bills.get(i);
			if (b->getDate() == today&&b->getStatus() == "paid")rev += b->getAmount();
		}
		int c = 0;
		if (c<maxOut)out[c++] = "=== Daily Report: " + today + " ===";
		if (c<maxOut)out[c++] = "Appts:" + myIntToStr(total) + " | Comp:" + myIntToStr(comp) + " | Pend:" + myIntToStr(pend) + " | NS:" + myIntToStr(ns) + " | Can:" + myIntToStr(can);
		if (c<maxOut)out[c++] = "Revenue today: PKR " + myFloatToStr(rev);
		if (c<maxOut)out[c++] = "--- Unpaid balances ---";
		for (int i = 0; i<patients.size() && c<maxOut; i++){
			float owed = 0.f;
			for (int j = 0; j<bills.size(); j++)
			if (bills.get(j)->getPatientId() == patients.get(i)->getId() && bills.get(j)->getStatus() == "unpaid")
				owed += bills.get(j)->getAmount();
			if (owed>0.f)out[c++] = "  " + patients.get(i)->getName() + " PKR " + myFloatToStr(owed);
		}
		if (c<maxOut)out[c++] = "--- Doctor summary ---";
		for (int i = 0; i<doctors.size() && c<maxOut; i++){
			int dc = 0, dp = 0, dn = 0, did = doctors.get(i)->getId();
			for (int j = 0; j<appointments.size(); j++){
				Appointment* a = appointments.get(j);
				if (a->getDoctorId() == did&&a->getDate() == today){
					if (a->getStatus() == "completed")dc++;
					if (a->getStatus() == "pending")dp++;
					if (a->getStatus() == "noshow")dn++;
				}
			}
			if (dc + dp + dn>0)out[c++] = "  Dr." + doctors.get(i)->getName() + " C:" + myIntToStr(dc) + " P:" + myIntToStr(dp) + " N:" + myIntToStr(dn);
		}
		return c;
	}
};

// ============================================================
// GUI Globals & Helpers
// ============================================================
static sf::Font gFont;


static const sf::Color C_BG(240, 244, 248);
static const sf::Color C_PANEL(255, 255, 255);
static const sf::Color C_ACCENT(22, 115, 117);
static const sf::Color C_DARK(14, 80, 82);
static const sf::Color C_TEXT(28, 38, 50);
static const sf::Color C_GRAY(110, 125, 140);
static const sf::Color C_BORDER(200, 215, 225);
static const sf::Color C_WHITE(255, 255, 255);
static const sf::Color C_ERR(195, 55, 55);
static const sf::Color C_OK(35, 150, 80);
static const sf::Color C_ROWALT(235, 243, 246);
static const sf::Color C_NAV(18, 95, 97);
static const sf::Color C_NAVSEL(10, 65, 67);
static const sf::Color C_NAVHOV(26, 125, 127);


static void gRect(sf::RenderTarget& rt, float x, float y, float w, float h,
	sf::Color fill, sf::Color out = sf::Color::Transparent, float thick = 0.f){
	sf::RectangleShape r({ w, h }); r.setPosition(x, y); r.setFillColor(fill);
	if (thick>0.f){ r.setOutlineThickness(thick); r.setOutlineColor(out); }
	rt.draw(r);
}
static sf::Text makeText(const string& s, int sz, sf::Color col){
	sf::Text t; t.setFont(gFont); t.setString(s); t.setCharacterSize(sz); t.setFillColor(col); return t;
}
static void gText(sf::RenderTarget& rt, const string& s, float x, float y,
	int sz, sf::Color col = sf::Color(28, 38, 50), bool bold = false){
	sf::Text t = makeText(s, sz, col); if (bold)t.setStyle(sf::Text::Bold); t.setPosition(x, y); rt.draw(t);
}
static void gTextC(sf::RenderTarget& rt, const string& s, float cx, float y,
	int sz, sf::Color col = sf::Color(28, 38, 50), bool bold = false){
	sf::Text t = makeText(s, sz, col); if (bold)t.setStyle(sf::Text::Bold);
	sf::FloatRect b = t.getLocalBounds(); t.setPosition(cx - b.width / 2.f, y); rt.draw(t);
}
static void gHeader(sf::RenderTarget& rt, float W, const string& title, const string& sub = ""){
	gRect(rt, 0, 0, W, 62, C_ACCENT);
	gText(rt, title, 18, 7, 24, C_WHITE, true);
	if (!sub.empty())gText(rt, sub, 18, 36, 12, sf::Color(180, 230, 232));
}
static void gCard(sf::RenderTarget& rt, float x, float y, float w, float h,
	sf::Color fill = sf::Color(255, 255, 255)){
	gRect(rt, x, y, w, h, fill, C_BORDER, 1.f);
}
static void gLine(sf::RenderTarget& rt, float x, float y, float w){
	gRect(rt, x, y, w, 1.f, C_BORDER);
}

// ============================================================
// UIButton
// ============================================================
struct UIButton{
	sf::RectangleShape sh;
	string label;
	sf::Color nc, hc;
	bool visible = true;
	UIButton() :nc(C_ACCENT), hc(C_DARK){}
	void init(float x, float y, float w, float h, const string& l,
		sf::Color n = sf::Color(22, 115, 117), sf::Color hv = sf::Color(14, 80, 82)){
		sh.setPosition(x, y); sh.setSize({ w, h }); label = l; nc = n; hc = hv;
	}
	void move(float x, float y){ sh.setPosition(x, y); }
	void resize(float w, float h){ sh.setSize({ w, h }); }
	bool hit(sf::Vector2f m)const{ return visible&&sh.getGlobalBounds().contains(m); }
	void draw(sf::RenderTarget& rt, sf::Vector2f m){
		if (!visible)return;
		sh.setFillColor(hit(m) ? hc : nc); rt.draw(sh);
		sf::Text t = makeText(label, 14, C_WHITE);
		sf::FloatRect b = t.getLocalBounds();
		t.setPosition(sh.getPosition().x + sh.getSize().x / 2.f - b.width / 2.f,
			sh.getPosition().y + sh.getSize().y / 2.f - b.height / 2.f - 2);
		rt.draw(t);
	}
};

// ============================================================
// UIInput
// ============================================================
struct UIInput{
	sf::RectangleShape box;
	string input, hint;
	bool active = false, pwd = false;
	float x = 0, y = 0, w = 0, h = 0;
	bool visible = true;
	void init(float _x, float _y, float _w, float _h, const string& _hint, bool _pwd = false){
		x = _x; y = _y; w = _w; h = _h; hint = _hint; pwd = _pwd;
		box.setPosition(x, y); box.setSize({ w, h });
		box.setFillColor(sf::Color(242, 246, 250));
		box.setOutlineThickness(1.5f); box.setOutlineColor(C_BORDER);
	}
	void move(float nx, float ny){ x = nx; y = ny; box.setPosition(x, y); }
	void clear(){ input = ""; }
	void set(const string& v){ input = v; }
	void handleEvent(sf::Event& ev, sf::RenderWindow& win){
		if (!visible)return;
		if (ev.type == sf::Event::MouseButtonPressed){
			sf::Vector2f m = win.mapPixelToCoords(sf::Mouse::getPosition(win));
			active = box.getGlobalBounds().contains(m);
			box.setOutlineColor(active ? C_ACCENT : C_BORDER);
		}
		if (active&&ev.type == sf::Event::TextEntered){
			if (ev.text.unicode == 8 && !input.empty())input.pop_back();
			else if (ev.text.unicode >= 32 && ev.text.unicode<128)input += (char)ev.text.unicode;
		}
	}
	void draw(sf::RenderTarget& rt){
		if (!visible)return;
		box.setOutlineColor(active ? C_ACCENT : C_BORDER); rt.draw(box);
		string disp = input.empty() ? hint : (pwd ? string(input.size(), '*') : input);
		sf::Text t = makeText(disp, 14, input.empty() ? C_GRAY : C_TEXT);
		t.setPosition(x + 10, y + h / 2.f - 9); rt.draw(t);
	}
};

// ============================================================
// UIDropdown  
// ============================================================
struct UIDropdown{
	string options[10]; int nOptions = 0, selected = -1;
	bool open = false;
	float x = 0, y = 0, w = 0, h = 0; string hint;
	void init(float _x, float _y, float _w, float _h, const string& _hint, const string* opts, int n){
		x = _x; y = _y; w = _w; h = _h; hint = _hint;
		nOptions = n<10 ? n : 10; for (int i = 0; i<nOptions; i++)options[i] = opts[i];
	}
	string value()const{ return selected >= 0 ? options[selected] : ""; }
	void clear(){ selected = -1; open = false; }
	void handleEvent(sf::Event& ev, sf::RenderWindow& win){
		if (ev.type != sf::Event::MouseButtonPressed)return;
		sf::Vector2f m = win.mapPixelToCoords(sf::Mouse::getPosition(win));
		
		if (sf::FloatRect(x, y, w, h).contains(m)){ open = !open; return; }
		
		if (open){
			for (int i = 0; i<nOptions; i++){
				float iy = y + h + i * h;
				if (sf::FloatRect(x, iy, w, h).contains(m)){ selected = i; open = false; return; }
			}
			open = false; 
		}
	}
	
	void drawHeader(sf::RenderTarget& rt){
		gRect(rt, x, y, w, h, sf::Color(242, 246, 250), open ? C_ACCENT : C_BORDER, 1.5f);
		string disp = (selected >= 0) ? options[selected] : hint;
		sf::Text t = makeText(disp, 14, (selected >= 0) ? C_TEXT : C_GRAY);
		t.setPosition(x + 10, y + (h - 16) / 2.f); rt.draw(t);
		gText(rt, "v", x + w - 18, y + (h - 16) / 2.f, 12, C_GRAY);
	}
	
	void drawPopup(sf::RenderTarget& rt, sf::Vector2f mouse){
		if (!open) return;
		for (int i = 0; i<nOptions; i++){
			float iy = y + h + i * h;
			bool hov = sf::FloatRect(x, iy, w, h).contains(mouse);
			gRect(rt, x, iy, w, h, hov ? sf::Color(210, 235, 237) : C_PANEL, C_BORDER, 1.f);
			sf::Text ot = makeText(options[i], 14, C_TEXT);
			ot.setPosition(x + 10, iy + (h - 16) / 2.f); rt.draw(ot);
		}
	}
	
	void draw(sf::RenderTarget& rt, sf::Vector2f mouse){
		drawHeader(rt);
		drawPopup(rt, mouse);
	}
};

// ============================================================
// ScrollList
// ============================================================
struct ScrollList{
	string rows[MAX_ROWS]; int nRows = 0;
	float x = 0, y = 0, w = 0, h = 0;
	int scroll = 0, sel = -1;
	static const int ROW_H = 22;
	void init(float _x, float _y, float _w, float _h){ x = _x; y = _y; w = _w; h = _h; }
	void clear(){ nRows = 0; scroll = 0; sel = -1; }
	void add(const string& s){ if (nRows<MAX_ROWS)rows[nRows++] = s; }
	void handleEvent(sf::Event& ev, sf::RenderWindow& win){
		if (ev.type == sf::Event::MouseWheelScrolled){
			sf::Vector2f m = win.mapPixelToCoords({ (int)ev.mouseWheelScroll.x, (int)ev.mouseWheelScroll.y });
			if (sf::FloatRect(x, y, w, h).contains(m)){
				scroll -= (int)ev.mouseWheelScroll.delta;
				int vis = (int)(h / ROW_H); int mx = nRows - vis; if (mx<0)mx = 0;
				if (scroll<0)scroll = 0; if (scroll>mx)scroll = mx;
			}
		}
		if (ev.type == sf::Event::MouseButtonPressed){
			sf::Vector2f m = win.mapPixelToCoords(sf::Mouse::getPosition(win));
			if (sf::FloatRect(x, y, w, h).contains(m)){
				int idx = (int)((m.y - y) / ROW_H) + scroll; if (idx >= 0 && idx<nRows)sel = idx;
			}
		}
	}
	void draw(sf::RenderTarget& rt){
		gRect(rt, x, y, w, h, C_PANEL, C_BORDER, 1.f);
		int vis = (int)(h / ROW_H); int end = scroll + vis; if (end>nRows)end = nRows;
		for (int i = scroll; i<end; i++){
			float ry = y + (i - scroll)*ROW_H;
			sf::Color fill = (i == sel) ? sf::Color(195, 228, 232) : (i % 2 == 1 ? C_ROWALT : C_PANEL);
			gRect(rt, x + 1, ry, w - 2, (float)ROW_H, fill);
			bool isHdr = (i == 0);
			gText(rt, rows[i], x + 5, ry + 3, 12, isHdr ? C_ACCENT : C_TEXT, isHdr);
		}
		if (nRows>vis){
			float tot = (float)nRows, sv = (float)vis;
			float bh = h*(sv / tot); float by = y + ((float)scroll / tot)*h;
			gRect(rt, x + w - 6, by, 5, bh, C_BORDER);
		}
	}
};

// ============================================================
// Toast
// ============================================================
struct Toast{
	string msg; bool ok = true; float timer = 0.f;
	void show(const string& m, bool isOk = true){ msg = m; ok = isOk; timer = 3.5f; }
	void update(float dt){ if (timer>0.f)timer -= dt; }
	void draw(sf::RenderTarget& rt, float W, float H){
		if (timer <= 0.f || msg.empty())return;
		sf::Color bg = ok ? sf::Color(35, 150, 80, 235) : sf::Color(195, 55, 55, 235);
		gRect(rt, 0, H - 38, W, 38, bg);
		gTextC(rt, msg, W / 2.f, H - 31, 14, C_WHITE, true);
	}
};

// ============================================================
// SCREEN ENUM
// ============================================================
enum class Screen{
	MAIN, REG, PAT_LOGIN, DOC_LOGIN, ADM_LOGIN,
	PAT_MENU, DOC_MENU, ADM_MENU
};
struct State{
	Screen scr = Screen::MAIN;
	Patient* pat = nullptr;
	Doctor*  doc = nullptr;
	int sub = 0;
};

// ============================================================
// ScreenBase
// ============================================================
class ScreenBase{
protected:
	sf::RenderWindow& win; HospitalSystem& hs; State& st;
	float W, H;
public:
	ScreenBase(sf::RenderWindow& w, HospitalSystem& h, State& s)
		:win(w), hs(h), st(s), W((float)w.getSize().x), H((float)w.getSize().y){}
	virtual void onEnter(){}
	virtual void handleEvent(sf::Event& ev) = 0;
	virtual void update(float dt){}
	virtual void draw(sf::Vector2f mouse) = 0;
	virtual ~ScreenBase(){}
};

// ============================================================
// MainScreen
// ============================================================
class MainScreen :public ScreenBase{
	UIButton bReg, bPat, bDoc, bAdm, bExit;
public:
	MainScreen(sf::RenderWindow& w, HospitalSystem& h, State& s) :ScreenBase(w, h, s){
		float by = 235.f;
		float gap2 = 64.f;

		bReg.init(W / 2.f - 160.f, by + gap2 * 0, 320.f, 50.f,
			"NEW PATIENT REGISTER", C_OK, sf::Color(25, 120, 60));
		bPat.init(W / 2.f - 160.f, by + gap2 * 1, 320.f, 50.f, "PATIENT LOGIN");
		bDoc.init(W / 2.f - 160.f, by + gap2 * 2, 320.f, 50.f, "DOCTOR LOGIN");
		bAdm.init(W / 2.f - 160.f, by + gap2 * 3, 320.f, 50.f, "ADMIN LOGIN");
		bExit.init(W / 2.f - 160.f, by + gap2 * 4 + 18.f, 320.f, 50.f,
			"EXIT", C_ERR, sf::Color(150, 35, 35));
	}
	void handleEvent(sf::Event& ev)override{
		if (ev.type != sf::Event::MouseButtonPressed)return;
		sf::Vector2f m = win.mapPixelToCoords(sf::Mouse::getPosition(win));
		if (bReg.hit(m)) st.scr = Screen::REG;
		if (bPat.hit(m)) st.scr = Screen::PAT_LOGIN;
		if (bDoc.hit(m)) st.scr = Screen::DOC_LOGIN;
		if (bAdm.hit(m)) st.scr = Screen::ADM_LOGIN;
		if (bExit.hit(m))win.close();
	}
	void draw(sf::Vector2f mouse)override{
		win.clear(C_BG);
		gHeader(win, W, "MediCore Hospital System", "Smart Healthcare Management");
		float panelW = 360.f;
		float panelH = 470.f;
		float panelX = (W - panelW) / 2.f;
		float panelY = 150.f;
		float panelCenterX = panelX + panelW / 2.f;
		gCard(win, panelX, panelY, panelW, panelH);
		gTextC(win, "Welcome", panelCenterX, panelY + 26.f, 26, C_ACCENT, true);
		gTextC(win, "Manage appointments, doctors, and patients easily",
			panelCenterX, panelY + 70.f, 11, C_GRAY);
		bReg.draw(win, mouse); bPat.draw(win, mouse);
		bDoc.draw(win, mouse); bAdm.draw(win, mouse);
		bExit.draw(win, mouse);
		gTextC(win, "Admin default: ID=1  Password=admin123", W / 2.f, 578, 11, C_GRAY);
		gTextC(win, "Doctors default password: doc456", W / 2.f, 596, 11, C_GRAY);
		win.display();
	}
};

// ============================================================
// RegisterScreen 
// ============================================================
class RegisterScreen : public ScreenBase {
	UIInput    iName, iAge, iContact, iPass, iConf;
	UIDropdown ddGender;
	UIButton   bReg, bBack;
	Toast      toast;
	int lastId = 0;
	bool success = false;

	
	static const int FW = 400;                       
	static const int FX = (WIN_W - FW) / 2;          
	static const int CARD_PAD = 28;                   
	static const int FIELD_H = 38;                  
	static const int ROW_GAP = 56;                   

	static const int FIRST_ROW = 170;

public:
	RegisterScreen(sf::RenderWindow& w, HospitalSystem& h, State& s)
		: ScreenBase(w, h, s)
	{
		const float fx = (float)FX;
		const float fw = (float)FW;
		const float r0 = (float)FIRST_ROW;
		const float rg = (float)ROW_GAP;
		const float fh = (float)FIELD_H;

		
		iName.init(fx, r0, fw, fh, "Full Name");

		
		float ageW = fw * 0.38f;
		float ddX = fx + ageW + fw * 0.04f;   
		float ddW = fw - ageW - fw * 0.04f;
		iAge.init(fx, r0 + rg, ageW, fh, "Age (1-120)");

		static const std::string genders[3] = { "Male", "Female", "Other" };
		ddGender.init(ddX, r0 + rg, ddW, fh, "Gender", genders, 3);

	
		iContact.init(fx, r0 + rg * 2, fw, fh,
			"Contact (11 digits, e.g. 03001234567)");
		iPass.init(fx, r0 + rg * 3, fw, fh,
			"Password (min 6 chars)", true);

		
		iConf.init(fx, r0 + rg * 4, fw, fh,
			"Confirm Password", true);

		
		bReg.init(fx, r0 + rg * 5 + 6.f, fw, 46.f,
			"CREATE ACCOUNT", C_OK, sf::Color(25, 120, 60));


		bBack.init(fx, r0 + rg * 5 + 60.f, fw, 36.f,
			"< BACK",
			sf::Color(130, 145, 160),
			sf::Color(100, 115, 130));
	}

	void onEnter() override {
		iName.clear(); iAge.clear(); iContact.clear();
		iPass.clear(); iConf.clear(); ddGender.clear();
		success = false; lastId = 0;
	}

	void update(float dt) override { toast.update(dt); }

	void handleEvent(sf::Event& ev) override {
		iName.handleEvent(ev, win);
		iAge.handleEvent(ev, win);
		iContact.handleEvent(ev, win);
		iPass.handleEvent(ev, win);
		iConf.handleEvent(ev, win);
		ddGender.handleEvent(ev, win);

		if (ev.type == sf::Event::MouseButtonPressed) {
			sf::Vector2f m = win.mapPixelToCoords(sf::Mouse::getPosition(win));

			if (bBack.hit(m)) { st.scr = Screen::MAIN; return; }

			if (bReg.hit(m) && !success) {
				int outId = 0;
				string err = hs.registerPatient(
					iName.input, iAge.input, ddGender.value(),
					iContact.input, iPass.input, iConf.input, outId);
				if (err.empty()) {
					lastId = outId; success = true;
					toast.show("Registered! Your ID: " + myIntToStr(outId) +
						" — please note it down.", true);
				}
				else {
					toast.show(err, false);
				}
			}
		}
	}

	void draw(sf::Vector2f mouse) override {
		win.clear(C_BG);
		gHeader(win, W, "Patient Registration", "Create your MediCore account");

		const float fx = (float)FX;
		const float fw = (float)FW;
		const float cp = (float)CARD_PAD;
		const float r0 = (float)FIRST_ROW;
		const float rg = (float)ROW_GAP;
		const float fh = (float)FIELD_H;

		
		float cardX = fx - cp;
		float cardW = fw + cp * 2.f;
		float cardY = r0 - 40.f;   

		
		float cardH_normal = (r0 + rg * 5.f + 60.f + 36.f + 20.f) - cardY;
		
		float cardH_success = cardH_normal;
		float cardH = success ? cardH_success : cardH_normal;

		gCard(win, cardX, cardY, cardW, cardH);

		
		gText(win, "New Patient Registration",
			fx, cardY + 10.f, 15, C_ACCENT, true);
		gLine(win, cardX, cardY + 34.f, cardW);

	
		gText(win, "Full Name", fx, r0 - 16.f, 11, C_GRAY);
		iName.draw(win);

		gText(win, "Age", fx, r0 + rg - 16.f, 11, C_GRAY);
		gText(win, "Gender", ddGender.x, r0 + rg - 16.f, 11, C_GRAY);
		iAge.draw(win);
		ddGender.drawHeader(win);

		gText(win, "Contact Number", fx, r0 + rg * 2 - 16.f, 11, C_GRAY);
		iContact.draw(win);

		gText(win, "Password", fx, r0 + rg * 3 - 16.f, 11, C_GRAY);
		iPass.draw(win);

		gText(win, "Confirm Password", fx, r0 + rg * 4 - 16.f, 11, C_GRAY);
		iConf.draw(win);

		
		if (success) {
			float sy = r0 + rg * 5 + 6.f;
			gCard(win, fx - cp, sy, fw + cp * 2.f, 110.f, sf::Color(240, 252, 240));
			gTextC(win, "Registration Successful!", W / 2.f, sy + 12.f, 16, C_OK, true);
			gTextC(win, "Your Patient ID:", W / 2.f, sy + 36.f, 13, C_GRAY);
			gTextC(win, myIntToStr(lastId), W / 2.f, sy + 54.f, 30, C_ACCENT, true);
			bBack.move(fx, sy + 92.f);
			bBack.draw(win, mouse);
		}
		else {
			bReg.draw(win, mouse);
			bBack.draw(win, mouse);
		}

		
		ddGender.drawPopup(win, mouse);

		toast.draw(win, W, H);
		win.display();
	}
};

// ============================================================
// LoginScreen
// ============================================================
class LoginScreen :public ScreenBase{
	string role;
	UIInput  iId, iPass;
	UIButton bLogin, bBack;
	Toast    toast;
public:
	LoginScreen(sf::RenderWindow& w, HospitalSystem& h, State& s, const string& r)
		:ScreenBase(w, h, s), role(r){
		float cx = W / 2.f - 170.f;
		iId.init(cx, 220, 340, 44, role + " ID (number)");
		iPass.init(cx, 284, 340, 44, "Password", true);
		bLogin.init(cx, 346, 340, 46, "LOGIN");
		bBack.init(cx, 406, 340, 36, "< BACK", sf::Color(130, 145, 160), sf::Color(100, 115, 130));
	}
	void onEnter()override{ iId.clear(); iPass.clear(); }
	void update(float dt)override{ toast.update(dt); }
	void handleEvent(sf::Event& ev)override{
		iId.handleEvent(ev, win); iPass.handleEvent(ev, win);
		if (ev.type == sf::Event::MouseButtonPressed){
			sf::Vector2f m = win.mapPixelToCoords(sf::Mouse::getPosition(win));
			if (bBack.hit(m)){ st.scr = Screen::MAIN; return; }
			if (bLogin.hit(m)){
				if (iId.input.empty() || iPass.input.empty()){ toast.show("Fill both fields.", false); return; }
				if (role == "Patient"){
					Patient* p = hs.loginPatient(iId.input, iPass.input);
					if (p){ st.pat = p; st.scr = Screen::PAT_MENU; st.sub = 0; }
					else toast.show("Invalid ID or password.", false);
				}
				else if (role == "Doctor"){
					Doctor* d = hs.loginDoctor(iId.input, iPass.input);
					if (d){ st.doc = d; st.scr = Screen::DOC_MENU; st.sub = 0; }
					else toast.show("Invalid ID or password.", false);
				}
				else{
					Admin* a = hs.loginAdmin(iId.input, iPass.input);
					if (a){ st.scr = Screen::ADM_MENU; st.sub = 0; }
					else toast.show("Invalid ID or password.", false);
				}
			}
		}
	}
	void draw(sf::Vector2f mouse)override{
		win.clear(C_BG);
		gHeader(win, W, "MediCore Hospital System", role + " Login");
		float cx = W / 2.f - 170.f;
		gCard(win, cx - 24, 162, 388, 318);
		gText(win, role + " Login", cx, 170, 17, C_ACCENT, true);
		gLine(win, cx - 24, 196, 388);
		gText(win, role + " ID", cx, 208, 11, C_GRAY);  iId.draw(win);
		gText(win, "Password", cx, 272, 11, C_GRAY);  iPass.draw(win);
		bLogin.draw(win, mouse); bBack.draw(win, mouse);
		if (role == "Patient")
			gTextC(win, "No account? Go back and Register.", W / 2.f, 455, 12, C_GRAY);
		if (role == "Doctor")
			gTextC(win, "Default doctors password: doc123", W / 2.f, 455, 12, C_GRAY);
		toast.draw(win, W, H);
		win.display();
	}
};

// ============================================================
// PatientMenuScreen  
// ============================================================
class PatientMenuScreen :public ScreenBase{
	UIButton mbtn[8], bBack;
	ScrollList lst;
	UIInput iSpec, iDocId, iDate, iSlot;
	UIButton bSearch, bBook;
	UIInput iCancelId; UIButton bCancel;
	UIInput iBillId; UIButton bPay;
	UIInput iAmt; UIButton bTopUp;
	Toast toast;

	static const int NW = NAV_W;
	float cx(){ return NW + 16.f; }
	float cw(){ return W - NW - 24.f; }

	void buildNav(){
		const char* lbl[] = { "Book Appointment", "Cancel Appointment", "My Appointments",
			"Medical Records", "My Bills", "Pay Bill", "Top Up Balance", "Logout" };
		for (int i = 0; i<8; i++)
			mbtn[i].init(4.f, 70.f + i*56.f, (float)NW - 8, 48, lbl[i], C_NAV, C_NAVHOV);
		bBack.init(4.f, H - 56.f, (float)NW - 8, 44, "< Back", sf::Color(90, 105, 120), sf::Color(70, 85, 100));
		bAction_dummy(); 
	}
	void bAction_dummy(){}

	void fillAppts(){
		lst.clear();
		lst.add("ApptID | Doctor                | Date       | Time  | Status");
		for (int i = 0; i<hs.appointments.size(); i++){
			Appointment* a = hs.appointments.get(i);
			if (a->getPatientId() != st.pat->getId())continue;
			Doctor* d = hs.doctors.findById(a->getDoctorId());
			lst.add(myIntToStr(a->getId()) + "      | " + (d ? d->getName() : "?")
				+ "     | " + a->getDate() + " | " + a->getTimeSlot() + " | " + a->getStatus());
		}
		if (lst.nRows == 1)lst.add("  (no appointments found)");
	}
	void fillPending(){
		lst.clear();
		lst.add("ApptID | Doctor                | Date       | Time  | Fee");
		for (int i = 0; i<hs.appointments.size(); i++){
			Appointment* a = hs.appointments.get(i);
			if (a->getPatientId() != st.pat->getId() || a->getStatus() != "pending")continue;
			Doctor* d = hs.doctors.findById(a->getDoctorId());
			lst.add(myIntToStr(a->getId()) + "      | " + (d ? d->getName() : "?")
				+ "     | " + a->getDate() + " | " + a->getTimeSlot()
				+ " | PKR " + (d ? myFloatToStr(d->getFee()) : "?"));
		}
		if (lst.nRows == 1)lst.add("  (no pending appointments)");
	}
	void fillRecords(){
		lst.clear();
		lst.add("Date       | Doctor              | Medicines                   | Notes");
		for (int i = 0; i<hs.prescriptions.size(); i++){
			Prescription* p = hs.prescriptions.get(i);
			if (p->getPatientId() != st.pat->getId())continue;
			Doctor* d = hs.doctors.findById(p->getDoctorId());
			lst.add(p->getDate() + " | " + (d ? d->getName() : "?")
				+ " | " + p->getMedicines() + " | " + p->getNotes());
		}
		if (lst.nRows == 1)lst.add("  (no medical records found)");
	}
	void fillBills(){
		lst.clear(); float total = 0.f;
		lst.add("BillID | ApptID | Amount (PKR) | Status   | Date");
		for (int i = 0; i<hs.bills.size(); i++){
			Bill* b = hs.bills.get(i);
			if (b->getPatientId() != st.pat->getId())continue;
			lst.add(myIntToStr(b->getBillId()) + "      | "
				+ myIntToStr(b->getAppointmentId()) + "      | PKR "
				+ myFloatToStr(b->getAmount()) + "       | "
				+ b->getStatus() + "  | " + b->getDate());
			if (b->getStatus() == "unpaid")total += b->getAmount();
		}
		if (lst.nRows == 1)lst.add("  (no bills found)");
		lst.add("  --- Total unpaid: PKR " + myFloatToStr(total) + " ---");
	}
	void fillUnpaidBills(){
		lst.clear();
		lst.add("BillID | Amount (PKR)  | Date");
		for (int i = 0; i<hs.bills.size(); i++){
			Bill* b = hs.bills.get(i);
			if (b->getPatientId() != st.pat->getId() || b->getStatus() != "unpaid")continue;
			lst.add(myIntToStr(b->getBillId()) + "      | PKR "
				+ myFloatToStr(b->getAmount()) + "      | " + b->getDate());
		}
		if (lst.nRows == 1)lst.add("  (no unpaid bills)");
	}
	void fillDoctors(const string& spec){
		lst.clear();
		lst.add("ID | Name                  | Specialization       | Fee (PKR)");
		for (int i = 0; i<hs.doctors.size(); i++){
			Doctor* d = hs.doctors.get(i);
			if (!spec.empty() && !myContains(d->getSpecialization(), spec) && !myContains(d->getName(), spec))continue;
			lst.add(myIntToStr(d->getId()) + "   | " + d->getName()
				+ "    | " + d->getSpecialization()
				+ "      | " + myFloatToStr(d->getFee()));
		}
		if (lst.nRows == 1)lst.add("  (no match found)");
	}

	float lstY(){ return 134.f; }
	float lstH(){ return H - lstY() - 50.f; }
	float inputRow1(){ return 106.f; }

public:
	PatientMenuScreen(sf::RenderWindow& w, HospitalSystem& h, State& s) :ScreenBase(w, h, s){
		buildNav();
		lst.init(cx(), lstY(), cw(), lstH());
		iSpec.init(cx(), inputRow1(), cw() - 170.f, 38, "Search by name or specialization...");
		bSearch.init(W - 175.f, inputRow1(), 160.f, 38, "Search Doctors");
		iDocId.init(cx(), inputRow1() + 48.f, 140.f, 38, "Doctor ID");
		iDate.init(cx() + 155.f, inputRow1() + 48.f, 200.f, 38, "Date DD-MM-YYYY");
		iSlot.init(cx() + 370.f, inputRow1() + 48.f, 140.f, 38, "Slot 09:00");
		bBook.init(cx() + 525.f, inputRow1() + 48.f, 150.f, 38, "Book Now", C_OK, sf::Color(25, 120, 60));
		iCancelId.init(cx(), inputRow1(), 180.f, 38, "Appointment ID");
		bCancel.init(cx() + 195.f, inputRow1(), 130.f, 38, "Cancel Appt", C_ERR, sf::Color(150, 35, 35));
		iBillId.init(cx(), inputRow1(), 180.f, 38, "Bill ID");
		bPay.init(cx() + 195.f, inputRow1(), 120.f, 38, "Pay Bill");
		iAmt.init(cx(), inputRow1(), 180.f, 38, "Amount (PKR)");
		bTopUp.init(cx() + 195.f, inputRow1(), 120.f, 38, "Top Up", C_OK, sf::Color(25, 120, 60));
	}
	void onEnter()override{
		st.sub = 0;
		iSpec.clear(); iDocId.clear(); iDate.clear(); iSlot.clear();
		iCancelId.clear(); iBillId.clear(); iAmt.clear(); lst.clear();
	}
	void update(float dt)override{ toast.update(dt); }

	void handleEvent(sf::Event& ev)override{
		sf::Vector2f m = win.mapPixelToCoords(sf::Mouse::getPosition(win));
		if (ev.type == sf::Event::MouseButtonPressed){
			for (int i = 0; i<8; i++)if (mbtn[i].hit(m)){
				if (i == 7){ st.scr = Screen::MAIN; return; }
				st.sub = i + 1;
				iSpec.clear(); iDocId.clear(); iDate.clear(); iSlot.clear();
				iCancelId.clear(); iBillId.clear(); iAmt.clear();
				if (i == 1)fillPending();
				if (i == 2)fillAppts();
				if (i == 3)fillRecords();
				if (i == 4)fillBills();
				if (i == 5)fillUnpaidBills();
				return;
			}
		}
		if (st.sub == 0)return;
		lst.handleEvent(ev, win);
		switch (st.sub){
		case 1:
			iSpec.handleEvent(ev, win); iDocId.handleEvent(ev, win);
			iDate.handleEvent(ev, win); iSlot.handleEvent(ev, win);
			if (ev.type == sf::Event::MouseButtonPressed){
				if (bSearch.hit(m)){ fillDoctors(iSpec.input); }
				if (bBook.hit(m)){
					if (iDocId.input.empty() || iDate.input.empty() || iSlot.input.empty()){
						toast.show("Fill Doctor ID, Date and Slot.", false); break;
					}
					string err = hs.bookAppointment(st.pat, myStrToInt(iDocId.input), iDate.input, iSlot.input);
					if (err.empty()){
						toast.show("Appointment booked!", true);
						iDocId.clear(); iDate.clear(); iSlot.clear();
					}
					else toast.show(err, false);
				}
			}
			break;
		case 2:
			iCancelId.handleEvent(ev, win);
			if (ev.type == sf::Event::MouseButtonPressed&&bCancel.hit(m)){
				if (iCancelId.input.empty()){ toast.show("Enter Appointment ID.", false); break; }
				string r = hs.cancelAppointment(st.pat, myStrToInt(iCancelId.input));
				bool ok = r.find("Cancelled") != string::npos;
				toast.show(r, ok); iCancelId.clear(); fillPending();
			}
			break;
		case 6:
			iBillId.handleEvent(ev, win);
			if (ev.type == sf::Event::MouseButtonPressed&&bPay.hit(m)){
				if (iBillId.input.empty()){ toast.show("Enter Bill ID.", false); break; }
				string r = hs.payBill(st.pat, myStrToInt(iBillId.input));
				toast.show(r, r.find("Paid") != string::npos);
				iBillId.clear(); fillUnpaidBills();
			}
			break;
		case 7:
			iAmt.handleEvent(ev, win);
			if (ev.type == sf::Event::MouseButtonPressed&&bTopUp.hit(m)){
				if (iAmt.input.empty()){ toast.show("Enter amount.", false); break; }
				string r = hs.topUp(st.pat, iAmt.input);
				toast.show(r, r.find("Balance") != string::npos&&r.find("valid") == string::npos);
				iAmt.clear();
			}
			break;
		}
	}

	void draw(sf::Vector2f mouse)override{
		win.clear(C_BG);
		string hdr = st.pat ? "Welcome, " + st.pat->getName()
			+ "   |   Balance: PKR " + myFloatToStr(st.pat->getBalance()) : "";
		gHeader(win, W, "Patient Portal", hdr);
		gRect(win, 0, 62, (float)NW, H - 62, C_NAV);
		for (int i = 0; i<8; i++){
			bool act = (st.sub == i + 1);
			mbtn[i].nc = act ? C_NAVSEL : C_NAV;
			mbtn[i].hc = act ? C_NAVSEL : C_NAVHOV;
			mbtn[i].draw(win, mouse);
		}
		bBack.visible = (st.sub>0);
		bBack.draw(win, mouse);
		gCard(win, (float)NW, 62, W - NW, H - 62);

		if (st.sub == 0){
			gTextC(win, "Patient Dashboard", NW + (W - NW) / 2.f, 78, 18, C_ACCENT, true);
			int unp = hs.countUnpaid(st.pat->getId());
			int pend = 0;
			for (int i = 0; i<hs.appointments.size(); i++)
			if (hs.appointments.get(i)->getPatientId() == st.pat->getId() &&
				hs.appointments.get(i)->getStatus() == "pending")pend++;
			float cardY = 115.f;
			gCard(win, NW + 20, cardY, 200, 82);
			gTextC(win, "Unpaid Bills", NW + 120, cardY + 10, 12, C_GRAY);
			gTextC(win, myIntToStr(unp), NW + 120, cardY + 30, 32, unp>0 ? C_ERR : C_OK);
			gCard(win, NW + 240, cardY, 200, 82);
			gTextC(win, "Pending Appts", NW + 340, cardY + 10, 12, C_GRAY);
			gTextC(win, myIntToStr(pend), NW + 340, cardY + 30, 32, C_ACCENT);
			gCard(win, NW + 460, cardY, 210, 82);
			gTextC(win, "Balance (PKR)", NW + 565, cardY + 10, 12, C_GRAY);
			gTextC(win, myFloatToStr(st.pat->getBalance()), NW + 565, cardY + 38, 20, C_TEXT);
			gCard(win, NW + 20, cardY + 100, 650, 80, sf::Color(248, 252, 255));
			gText(win, "Patient ID: " + myIntToStr(st.pat->getId()), NW + 34, cardY + 110, 13, C_ACCENT, true);
			gText(win, "Use the sidebar to book, cancel appointments, view bills and medical records.", NW + 34, cardY + 132, 12, C_GRAY);
			gText(win, "Tip: Top up your balance first, then book an appointment.", NW + 34, cardY + 150, 12, C_GRAY);
		}
		else {
			const char* titles[] = { "", "Book Appointment", "Cancel Appointment",
				"My Appointments", "Medical Records", "My Bills", "Pay Bill", "Top Up Balance" };
			gText(win, titles[st.sub], cx(), 72, 16, C_ACCENT, true);
			gLine(win, (float)NW, 95, W - (float)NW);

			if (st.sub == 1){
				gText(win, "Filter (name / spec):", cx(), inputRow1() - 16, 11, C_GRAY);
				iSpec.draw(win); bSearch.draw(win, mouse);
				if (lst.nRows == 0){ fillDoctors(""); }
				lst.y = inputRow1() + 44.f; lst.h = H - lst.y - 115.f; lst.draw(win);
				float brow = H - 108.f;
				gRect(win, (float)NW, brow - 8, W - (float)NW, 108, sf::Color(245, 250, 252), C_BORDER, 1.f);
				gText(win, "Doctor ID:", cx(), brow, 11, C_GRAY);
				iDocId.move(cx(), brow + 14.f); iDocId.draw(win);
				gText(win, "Date (DD-MM-YYYY):", cx() + 155.f, brow, 11, C_GRAY);
				iDate.move(cx() + 155.f, brow + 14.f); iDate.draw(win);
				gText(win, "Time Slot:", cx() + 370.f, brow, 11, C_GRAY);
				iSlot.move(cx() + 370.f, brow + 14.f); iSlot.draw(win);
				bBook.move(cx() + 525.f, brow + 14.f); bBook.draw(win, mouse);
				if (!iDocId.input.empty() && !iDate.input.empty()){
					Doctor* doc = hs.doctors.findById(myStrToInt(iDocId.input));
					if (doc&&Validator::isValidDate(iDate.input)){
						string avail = hs.availableSlots(doc->getId(), iDate.input);
						gText(win, avail, cx(), brow + 60.f, 11, C_OK);
					}
				}
				gText(win, "Slots: 09:00 10:00 11:00 12:00 14:00 15:00 16:00 17:00",
					cx(), brow + 78.f, 10, C_GRAY);
			}
			else if (st.sub == 2){
				gText(win, "Pending appointments (enter ID to cancel):", cx(), 100, 12, C_GRAY);
				lst.y = 120.f; lst.h = H - 230.f; lst.draw(win);
				float br = H - 100.f;
				gText(win, "Appointment ID to cancel:", cx(), br, 12, C_GRAY);
				iCancelId.move(cx(), br + 16.f); iCancelId.draw(win);
				bCancel.move(cx() + 195.f, br + 16.f); bCancel.draw(win, mouse);
			}
			else if (st.sub >= 3 && st.sub <= 5){
				lst.y = 102.f; lst.h = H - 154.f; lst.draw(win);
			}
			else if (st.sub == 6){
				gText(win, "Unpaid bills (enter Bill ID to pay):", cx(), 100, 12, C_GRAY);
				lst.y = 120.f; lst.h = H - 230.f; lst.draw(win);
				float br = H - 100.f;
				gText(win, "Bill ID to pay:", cx(), br, 12, C_GRAY);
				iBillId.move(cx(), br + 16.f); iBillId.draw(win);
				bPay.move(cx() + 195.f, br + 16.f); bPay.draw(win, mouse);
			}
			else if (st.sub == 7){
				float cy2 = 115.f;
				gText(win, "Current Balance: PKR " + myFloatToStr(st.pat->getBalance()),
					cx(), cy2, 16, C_TEXT, true);
				gText(win, "Enter amount to add to your balance:", cx(), cy2 + 40.f, 12, C_GRAY);
				iAmt.move(cx(), cy2 + 56.f); iAmt.draw(win);
				bTopUp.move(cx() + 195.f, cy2 + 56.f); bTopUp.draw(win, mouse);
			}
		}
		toast.draw(win, W, H);
		win.display();
	}
};

// ============================================================
// DoctorMenuScreen 
// ============================================================
class DoctorMenuScreen :public ScreenBase{
	UIButton mbtn[6], bBack, bAction;
	ScrollList lst;
	UIInput iAppt, iMeds, iNotes, iPatId;
	Toast toast;
	static const int NW = NAV_W;
	float cx(){ return NW + 16.f; }
	float cw(){ return W - NW - 24.f; }

	void buildNav(){
		const char* lbl[] = { "Today's Appts", "Mark Complete", "Mark No-Show",
			"Write Prescription", "Patient History", "Logout" };
		for (int i = 0; i<6; i++)
			mbtn[i].init(4.f, 70.f + i*62.f, (float)NW - 8, 54, lbl[i], C_NAV, C_NAVHOV);
		bBack.init(4.f, H - 56.f, (float)NW - 8, 44, "< Back", sf::Color(90, 105, 120), sf::Color(70, 85, 100));
		bAction.init(cx() + 220.f, 110.f, 180.f, 40.f, "Submit");
	}
	void fillToday(){
		lst.clear(); string today = hs.getTodayDate();
		lst.add("ApptID | Patient              | Time  | Status");
		for (int i = 0; i<hs.appointments.size(); i++){
			Appointment* a = hs.appointments.get(i);
			if (a->getDoctorId() != st.doc->getId() || a->getDate() != today)continue;
			Patient* p = hs.patients.findById(a->getPatientId());
			lst.add(myIntToStr(a->getId()) + "      | " + (p ? p->getName() : "?")
				+ "     | " + a->getTimeSlot() + " | " + a->getStatus());
		}
		if (lst.nRows == 1)lst.add("  (no appointments today)");
	}
	void fillAllAppts(){
		lst.clear();
		lst.add("ApptID | Patient              | Date       | Time  | Status");
		for (int i = 0; i<hs.appointments.size(); i++){
			Appointment* a = hs.appointments.get(i);
			if (a->getDoctorId() != st.doc->getId())continue;
			Patient* p = hs.patients.findById(a->getPatientId());
			lst.add(myIntToStr(a->getId()) + "      | " + (p ? p->getName() : "?")
				+ "     | " + a->getDate() + " | " + a->getTimeSlot() + " | " + a->getStatus());
		}
		if (lst.nRows == 1)lst.add("  (no appointments)");
	}
public:
	DoctorMenuScreen(sf::RenderWindow& w, HospitalSystem& h, State& s) :ScreenBase(w, h, s){
		buildNav();
		lst.init(cx(), 106.f, cw(), H - 160.f);
		iAppt.init(cx(), 110.f, 200.f, 40, "Appointment ID");
		iMeds.init(cx(), 156.f, cw(), 40, "Medicines (e.g. Paracetamol 500mg; Amoxicillin 250mg)");
		iNotes.init(cx(), 210.f, cw(), 40, "Notes / Instructions");
		iPatId.init(cx(), 110.f, 200.f, 40, "Patient ID");
	}
	void onEnter()override{
		st.sub = 0;
		iAppt.clear(); iMeds.clear(); iNotes.clear(); iPatId.clear(); lst.clear();
	}
	void update(float dt)override{ toast.update(dt); }
	void handleEvent(sf::Event& ev)override{
		sf::Vector2f m = win.mapPixelToCoords(sf::Mouse::getPosition(win));
		if (ev.type == sf::Event::MouseButtonPressed)
		for (int i = 0; i<6; i++)if (mbtn[i].hit(m)){
			if (i == 5){ st.scr = Screen::MAIN; return; }
			st.sub = i + 1;
			iAppt.clear(); iMeds.clear(); iNotes.clear(); iPatId.clear();
			if (i == 0)fillAllAppts();
			if (i == 1 || i == 2 || i == 3)fillToday();
			return;
		}
		if (st.sub == 0)return;
		lst.handleEvent(ev, win);
		iAppt.handleEvent(ev, win); iMeds.handleEvent(ev, win);
		iNotes.handleEvent(ev, win); iPatId.handleEvent(ev, win);
		if (ev.type == sf::Event::MouseButtonPressed&&bAction.hit(m)){
			string r;
			if (st.sub == 2){
				if (iAppt.input.empty()){ toast.show("Enter Appointment ID.", false); return; }
				r = hs.markStatus(st.doc, myStrToInt(iAppt.input), "completed");
				iAppt.clear(); fillToday();
			}
			else if (st.sub == 3){
				if (iAppt.input.empty()){ toast.show("Enter Appointment ID.", false); return; }
				r = hs.markStatus(st.doc, myStrToInt(iAppt.input), "noshow");
				iAppt.clear(); fillToday();
			}
			else if (st.sub == 4){
				if (iAppt.input.empty() || iMeds.input.empty()){
					toast.show("Enter Appointment ID and medicines.", false); return;
				}
				r = hs.writePrescription(st.doc, myStrToInt(iAppt.input), iMeds.input, iNotes.input);
				iAppt.clear(); iMeds.clear(); iNotes.clear();
			}
			else if (st.sub == 5){
				int pid = myStrToInt(iPatId.input);
				Patient* pp = hs.patients.findById(pid);
				bool access = false;
				for (int i = 0; i<hs.appointments.size(); i++){
					Appointment* a = hs.appointments.get(i);
					if (a->getPatientId() == pid&&a->getDoctorId() == st.doc->getId() && a->getStatus() == "completed"){ access = true; break; }
				}
				if (!pp || !access){ r = "Access denied or no completed appointment."; }
				else{
					lst.clear();
					lst.add("History for: " + pp->getName() + " (ID:" + myIntToStr(pid) + ")");
					lst.add("Date       | Medicines                        | Notes");
					for (int i = 0; i<hs.prescriptions.size(); i++){
						Prescription* rx = hs.prescriptions.get(i);
						if (rx->getPatientId() == pid&&rx->getDoctorId() == st.doc->getId())
							lst.add(rx->getDate() + " | " + rx->getMedicines() + " | " + rx->getNotes());
					}
					if (lst.nRows == 2)lst.add("  (no prescriptions yet)");
					r = "Loaded.";
				}
			}
			toast.show(r, r == "Done." || r == "Saved." || r == "Loaded.");
		}
	}
	void draw(sf::Vector2f mouse)override{
		win.clear(C_BG);
		string hdr = st.doc ? "Dr. " + st.doc->getName() + "   |   " + st.doc->getSpecialization()
			+ "   |   ID: " + myIntToStr(st.doc->getId()) : "";
		gHeader(win, W, "Doctor Portal", hdr);
		gRect(win, 0, 62, (float)NW, H - 62, C_NAV);
		for (int i = 0; i<6; i++){
			bool act = (st.sub == i + 1);
			mbtn[i].nc = act ? C_NAVSEL : C_NAV; mbtn[i].hc = act ? C_NAVSEL : C_NAVHOV;
			mbtn[i].draw(win, mouse);
		}
		bBack.visible = (st.sub>0); bBack.draw(win, mouse);
		gCard(win, (float)NW, 62, W - NW, H - 62);
		if (st.sub == 0){
			gTextC(win, "Doctor Dashboard", NW + (W - NW) / 2.f, 80, 18, C_ACCENT, true);
			gTextC(win, "Today: " + hs.getTodayDate(), NW + (W - NW) / 2.f, 108, 13, C_GRAY);
			string today = hs.getTodayDate(); int todayC = 0;
			for (int i = 0; i<hs.appointments.size(); i++){
				Appointment* a = hs.appointments.get(i);
				if (a->getDoctorId() == st.doc->getId() && a->getDate() == today)todayC++;
			}
			gCard(win, NW + 20, 130, 200, 82);
			gTextC(win, "Today's Appts", NW + 120, 140, 12, C_GRAY);
			gTextC(win, myIntToStr(todayC), NW + 120, 158, 32, C_ACCENT);
			gCard(win, NW + 240, 130, 200, 82, sf::Color(248, 252, 255));
			gTextC(win, "Fee Per Visit", NW + 340, 140, 12, C_GRAY);
			gTextC(win, "PKR " + myFloatToStr(st.doc->getFee()), NW + 340, 158, 18, C_TEXT);
			gCard(win, NW + 20, 225, 650, 68, sf::Color(248, 252, 255));
			gText(win, "Use sidebar to view appointments, mark status, and write prescriptions.", NW + 34, 235, 12, C_GRAY);
			gText(win, "Note: You can only mark appointments scheduled for today.", NW + 34, 255, 12, C_GRAY);
		}
		else {
			const char* titles[] = { "", "All Appointments", "Mark Complete", "Mark No-Show",
				"Write Prescription", "Patient History" };
			gText(win, titles[st.sub], cx(), 72, 16, C_ACCENT, true);
			gLine(win, (float)NW, 95, W - (float)NW);
			if (st.sub == 1){
				lst.y = 102.f; lst.h = H - 154.f; lst.draw(win);
			}
			else if (st.sub == 2 || st.sub == 3){
				gText(win, "Today's appointments:", cx(), 100, 12, C_GRAY);
				lst.y = 116.f; lst.h = H - 250.f; lst.draw(win);
				float br = H - 120.f;
				gText(win, "Appointment ID:", cx(), br, 12, C_GRAY);
				iAppt.move(cx(), br + 16.f); iAppt.draw(win);
				string lbl2 = st.sub == 2 ? "Mark Complete" : "Mark No-Show";
				sf::Color bc2 = st.sub == 2 ? C_OK : sf::Color(180, 90, 20);
				sf::Color bh2 = st.sub == 2 ? sf::Color(25, 120, 60) : sf::Color(140, 60, 10);
				bAction.nc = bc2; bAction.hc = bh2; bAction.label = lbl2;
				bAction.move(cx() + 210.f, br + 16.f); bAction.resize(180.f, 40.f);
				bAction.draw(win, mouse);
			}
			else if (st.sub == 4){
				gText(win, "Appointment ID (must be completed):", cx(), 100, 12, C_GRAY);
				iAppt.move(cx(), 116.f); iAppt.draw(win);
				gText(win, "Medicines:", cx(), 166.f, 12, C_GRAY);
				iMeds.move(cx(), 182.f); iMeds.draw(win);
				gText(win, "Notes / Instructions:", cx(), 232.f, 12, C_GRAY);
				iNotes.move(cx(), 248.f); iNotes.draw(win);
				bAction.nc = C_ACCENT; bAction.hc = C_DARK; bAction.label = "Save Prescription";
				bAction.move(cx(), 298.f); bAction.resize(200.f, 42.f);
				bAction.draw(win, mouse);
			}
			else if (st.sub == 5){
				gText(win, "Patient ID (must have completed appointment with you):", cx(), 100, 12, C_GRAY);
				iPatId.move(cx(), 116.f); iPatId.draw(win);
				bAction.nc = C_ACCENT; bAction.hc = C_DARK; bAction.label = "Load History";
				bAction.move(cx() + 210.f, 116.f); bAction.resize(165.f, 40.f);
				bAction.draw(win, mouse);
				lst.y = 168.f; lst.h = H - 222.f; lst.draw(win);
			}
		}
		toast.draw(win, W, H);
		win.display();
	}
};

// ============================================================
// AdminMenuScreen  
// ============================================================
class AdminMenuScreen :public ScreenBase{
	UIButton mbtn[10], bBack, bAction;
	ScrollList lst;
	UIInput iName, iSpec, iContact, iPass, iFee, iId;
	Toast toast;
	static const int NW = NAV_W;
	float cx(){ return NW + 16.f; }
	float cw(){ return W - NW - 24.f; }

	void buildNav(){
		const char* lbl[] = { "All Patients", "All Doctors", "All Appointments", "Unpaid Bills",
			"Add Doctor", "Remove Doctor", "Discharge Patient", "Security Log", "Daily Report", "Logout" };
		for (int i = 0; i<10; i++)
			mbtn[i].init(4.f, 70.f + i*48.f, (float)NW - 8, 42, lbl[i], C_NAV, C_NAVHOV);
		bBack.init(4.f, H - 56.f, (float)NW - 8, 44, "< Back", sf::Color(90, 105, 120), sf::Color(70, 85, 100));
		bAction.init(cx() + 220.f, 120.f, 170.f, 40, "Submit");
	}
	void fillPatients(){
		lst.clear();
		lst.add("ID | Name               | Age | Gender | Contact     | Balance    | Unpaid Bills");
		for (int i = 0; i<hs.patients.size(); i++){
			Patient* p = hs.patients.get(i);
			lst.add(myIntToStr(p->getId()) + "  | " + p->getName()
				+ "  | " + myIntToStr(p->getAge())
				+ "  | " + p->getGender()
				+ "  | " + p->getContact()
				+ "  | PKR " + myFloatToStr(p->getBalance())
				+ "  | " + myIntToStr(hs.countUnpaid(p->getId())));
		}
		if (hs.patients.size() == 0)lst.add("  (no patients yet)");
	}
	void fillDoctors(){
		lst.clear();
		lst.add("ID | Name               | Specialization       | Contact     | Fee (PKR)");
		for (int i = 0; i<hs.doctors.size(); i++){
			Doctor* d = hs.doctors.get(i);
			lst.add(myIntToStr(d->getId()) + "  | " + d->getName()
				+ "  | " + d->getSpecialization()
				+ "  | " + d->getContact()
				+ "  | " + myFloatToStr(d->getFee()));
		}
	}
	void fillAppts(){
		lst.clear();
		lst.add("ID | Patient             | Doctor              | Date       | Time  | Status");
		for (int i = 0; i<hs.appointments.size(); i++){
			Appointment* a = hs.appointments.get(i);
			Patient* p = hs.patients.findById(a->getPatientId());
			Doctor* d = hs.doctors.findById(a->getDoctorId());
			lst.add(myIntToStr(a->getId()) + "  | " + (p ? p->getName() : "?")
				+ "  | " + (d ? d->getName() : "?")
				+ "  | " + a->getDate() + " | " + a->getTimeSlot() + " | " + a->getStatus());
		}
		if (hs.appointments.size() == 0)lst.add("  (none)");
	}
	void fillUnpaid(){
		lst.clear();
		lst.add("BillID | Patient             | Amount (PKR) | Date       | Flag");
		for (int i = 0; i<hs.bills.size(); i++){
			Bill* b = hs.bills.get(i); if (b->getStatus() != "unpaid")continue;
			Patient* p = hs.patients.findById(b->getPatientId());
			bool ov = dateDiffDays(b->getDate())>7;
			lst.add(myIntToStr(b->getBillId()) + "      | " + (p ? p->getName() : "?")
				+ "     | PKR " + myFloatToStr(b->getAmount())
				+ "     | " + b->getDate() + (ov ? " [OVERDUE]" : ""));
		}
		if (lst.nRows == 1)lst.add("  (none)");
	}
	void fillLog(){
		lst.clear(); string lines[600]; int n = FileHandler::readLog(lines, 600);
		for (int i = 0; i<n; i++)lst.add(lines[i]);
		if (n == 0)lst.add("  (log empty)");
	}
	void fillReport(){
		lst.clear(); string lines[120]; int n = hs.dailyReport(lines, 120);
		for (int i = 0; i<n; i++)lst.add(lines[i]);
	}
public:
	AdminMenuScreen(sf::RenderWindow& w, HospitalSystem& h, State& s) :ScreenBase(w, h, s){
		buildNav();
		lst.init(cx(), 102.f, cw(), H - 154.f);
		float fx = cx();
		iName.init(fx, 120.f, 320.f, 38, "Doctor Full Name");
		iSpec.init(fx, 176.f, 320.f, 38, "Specialization");
		iContact.init(fx, 232.f, 320.f, 38, "Contact (11 digits)");
		iPass.init(fx, 288.f, 320.f, 38, "Password (min 6 chars)", true);
		iFee.init(fx, 344.f, 200.f, 38, "Consultation Fee PKR");
		iId.init(fx, 120.f, 220.f, 38, "ID");
	}
	void onEnter()override{
		st.sub = 0;
		iName.clear(); iSpec.clear(); iContact.clear(); iPass.clear(); iFee.clear(); iId.clear(); lst.clear();
	}
	void update(float dt)override{ toast.update(dt); }
	void handleEvent(sf::Event& ev)override{
		sf::Vector2f m = win.mapPixelToCoords(sf::Mouse::getPosition(win));
		if (ev.type == sf::Event::MouseButtonPressed)
		for (int i = 0; i<10; i++)if (mbtn[i].hit(m)){
			if (i == 9){ st.scr = Screen::MAIN; return; }
			st.sub = i + 1;
			iName.clear(); iSpec.clear(); iContact.clear(); iPass.clear(); iFee.clear(); iId.clear();
			if (i == 0)fillPatients(); if (i == 1)fillDoctors(); if (i == 2)fillAppts();
			if (i == 3)fillUnpaid(); if (i == 7)fillLog(); if (i == 8)fillReport();
			return;
		}
		if (st.sub == 0)return;
		lst.handleEvent(ev, win);
		iName.handleEvent(ev, win); iSpec.handleEvent(ev, win);
		iContact.handleEvent(ev, win); iPass.handleEvent(ev, win);
		iFee.handleEvent(ev, win); iId.handleEvent(ev, win);
		if (ev.type == sf::Event::MouseButtonPressed&&bAction.hit(m)){
			string r;
			if (st.sub == 5){
				int outId = 0;
				r = hs.addDoctor(iName.input, iSpec.input, iContact.input, iPass.input, iFee.input, outId);
				if (r.empty()){
					r = "Doctor added. ID=" + myIntToStr(outId) + " (they can now login with their ID & password)";
					iName.clear(); iSpec.clear(); iContact.clear(); iPass.clear(); iFee.clear();
				}
			}
			else if (st.sub == 6){
				r = hs.removeDoctor(myStrToInt(iId.input)); fillDoctors(); iId.clear();
			}
			else if (st.sub == 7){
				r = hs.dischargePatient(myStrToInt(iId.input)); fillPatients(); iId.clear();
			}
			bool ok = r.find("added") != string::npos || r.find("Removed") != string::npos || r.find("Discharged") != string::npos;
			toast.show(r, ok);
		}
	}
	void draw(sf::Vector2f mouse)override{
		win.clear(C_BG);
		gHeader(win, W, "Admin Panel", "MediCore Hospital System — Administrator");
		gRect(win, 0, 62, (float)NW, H - 62, C_NAV);
		for (int i = 0; i<10; i++){
			bool act = (st.sub == i + 1);
			mbtn[i].nc = act ? C_NAVSEL : C_NAV; mbtn[i].hc = act ? C_NAVSEL : C_NAVHOV;
			mbtn[i].draw(win, mouse);
		}
		bBack.visible = (st.sub>0); bBack.draw(win, mouse);
		gCard(win, (float)NW, 62, W - NW, H - 62);
		if (st.sub == 0){
			gTextC(win, "Admin Dashboard", NW + (W - NW) / 2.f, 80, 18, C_ACCENT, true);
			gCard(win, NW + 20, 118, 180, 80);
			gTextC(win, "Doctors", NW + 110, 128, 12, C_GRAY);
			gTextC(win, myIntToStr(hs.doctors.size()), NW + 110, 148, 32, C_ACCENT);
			gCard(win, NW + 220, 118, 180, 80);
			gTextC(win, "Patients", NW + 310, 128, 12, C_GRAY);
			gTextC(win, myIntToStr(hs.patients.size()), NW + 310, 148, 32, C_ACCENT);
			gCard(win, NW + 420, 118, 200, 80);
			gTextC(win, "Appointments", NW + 520, 128, 12, C_GRAY);
			gTextC(win, myIntToStr(hs.appointments.size()), NW + 520, 148, 32, C_ACCENT);
			gCard(win, NW + 20, 212, 730, 80, sf::Color(248, 252, 255));
			gText(win, "Admin ID=1 | Default Password=admin123", NW + 34, 222, 12, C_GRAY);
			gText(win, "Add doctors via 'Add Doctor'. Added doctors can login immediately with their ID & set password.", NW + 34, 242, 12, C_GRAY);
			gText(win, "All data auto-saved to .txt files in the working directory.", NW + 34, 260, 12, C_GRAY);
		}
		else {
			const char* titles[] = { "", "All Patients", "All Doctors", "All Appointments", "Unpaid Bills",
				"Add Doctor", "Remove Doctor", "Discharge Patient", "Security Log", "Daily Report" };
			gText(win, titles[st.sub], cx(), 72, 16, C_ACCENT, true);
			gLine(win, (float)NW, 95, W - (float)NW);
			if (st.sub <= 4 || st.sub >= 8){
				lst.y = 102.f; lst.h = H - 154.f; lst.draw(win);
			}
			else if (st.sub == 5){
				gText(win, "Name:", cx(), 108, 11, C_GRAY); iName.draw(win);
				gText(win, "Specialization:", cx(), 164, 11, C_GRAY); iSpec.draw(win);
				gText(win, "Contact:", cx(), 220, 11, C_GRAY); iContact.draw(win);
				gText(win, "Password:", cx(), 276, 11, C_GRAY); iPass.draw(win);
				gText(win, "Fee (PKR):", cx(), 332, 11, C_GRAY); iFee.draw(win);
				bAction.nc = C_ACCENT; bAction.hc = C_DARK; bAction.label = "Add Doctor";
				bAction.move(cx(), 392.f); bAction.resize(180.f, 44.f);
				bAction.draw(win, mouse);
				gText(win, "ID is auto-assigned. Doctor can login immediately after being added.",
					cx(), 444, 11, C_GRAY);
			}
			else if (st.sub == 6){
				gText(win, "Current doctors:", cx(), 100, 12, C_GRAY);
				lst.y = 116.f; lst.h = H - 260.f; lst.draw(win);
				float br = H - 130.f;
				gText(win, "Doctor ID to remove:", cx(), br, 12, C_GRAY);
				iId.move(cx(), br + 16.f); iId.draw(win);
				bAction.nc = C_ERR; bAction.hc = sf::Color(150, 35, 35); bAction.label = "Remove Doctor";
				bAction.move(cx() + 230.f, br + 16.f); bAction.resize(175.f, 40.f);
				bAction.draw(win, mouse);
			}
			else if (st.sub == 7){
				gText(win, "Current patients:", cx(), 100, 12, C_GRAY);
				lst.y = 116.f; lst.h = H - 260.f; lst.draw(win);
				float br = H - 130.f;
				gText(win, "Patient ID to discharge:", cx(), br, 12, C_GRAY);
				iId.move(cx(), br + 16.f); iId.draw(win);
				bAction.nc = C_ACCENT; bAction.hc = C_DARK; bAction.label = "Discharge";
				bAction.move(cx() + 230.f, br + 16.f); bAction.resize(160.f, 40.f);
				bAction.draw(win, mouse);
			}
		}
		toast.draw(win, W, H);
		win.display();
	}
};

// ============================================================
// Application
// ============================================================
class Application{
	sf::RenderWindow   win;
	HospitalSystem     hs;
	State              st;
	MainScreen*        scrMain = nullptr;
	RegisterScreen*    scrReg = nullptr;
	LoginScreen*       scrPL = nullptr;
	LoginScreen*       scrDL = nullptr;
	LoginScreen*       scrAL = nullptr;
	PatientMenuScreen* scrPat = nullptr;
	DoctorMenuScreen*  scrDoc = nullptr;
	AdminMenuScreen*   scrAdm = nullptr;
	Screen prev = Screen::MAIN;

	ScreenBase* cur(){
		switch (st.scr){
		case Screen::MAIN:     return scrMain;
		case Screen::REG:      return scrReg;
		case Screen::PAT_LOGIN:return scrPL;
		case Screen::DOC_LOGIN:return scrDL;
		case Screen::ADM_LOGIN:return scrAL;
		case Screen::PAT_MENU: return scrPat;
		case Screen::DOC_MENU: return scrDoc;
		case Screen::ADM_MENU: return scrAdm;
		}
		return scrMain;
	}
public:
	Application() :win(sf::VideoMode(WIN_W, WIN_H), "MediCore Hospital System"){
		win.setFramerateLimit(60);
		if (!gFont.loadFromFile("C:/Windows/Fonts/arial.ttf"))
		if (!gFont.loadFromFile("C:/Windows/Fonts/segoeui.ttf"))
		if (!gFont.loadFromFile("C:/Windows/Fonts/calibri.ttf"))
		if (!gFont.loadFromFile("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"))
		if (!gFont.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
			gFont.loadFromFile("arial.ttf");

		hs.loadAll();
		scrMain = new MainScreen(win, hs, st);
		scrReg = new RegisterScreen(win, hs, st);
		scrPL = new LoginScreen(win, hs, st, "Patient");
		scrDL = new LoginScreen(win, hs, st, "Doctor");
		scrAL = new LoginScreen(win, hs, st, "Admin");
		scrPat = new PatientMenuScreen(win, hs, st);
		scrDoc = new DoctorMenuScreen(win, hs, st);
		scrAdm = new AdminMenuScreen(win, hs, st);
	}
	~Application(){
		hs.saveAll();
		delete scrMain; delete scrReg;
		delete scrPL; delete scrDL; delete scrAL;
		delete scrPat; delete scrDoc; delete scrAdm;
	}
	void run(){
		sf::Clock clk;
		while (win.isOpen()){
			float dt = clk.restart().asSeconds();
			if (st.scr != prev){ cur()->onEnter(); prev = st.scr; }
			sf::Event ev;
			while (win.pollEvent(ev)){
				if (ev.type == sf::Event::Closed)win.close();
				cur()->handleEvent(ev);
			}
			sf::Vector2f mouse = win.mapPixelToCoords(sf::Mouse::getPosition(win));
			cur()->update(dt);
			cur()->draw(mouse);
		}
	}
};

// ============================================================
// main
// ============================================================
int main(){
	Application app;
	app.run();
	return 0;
}
