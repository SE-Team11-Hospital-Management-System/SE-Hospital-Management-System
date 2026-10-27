// The shapes the C++ server sends (see src/api/api.cpp, toJson).
export type Role = 'Administrator' | 'Receptionist' | 'Doctor';
export type Permission =
  | 'changePassword' | 'manageUsers' | 'viewAuditLog' | 'registerPatient' | 'updatePatient' | 'searchPatient'
  | 'deactivatePatient' | 'manageDoctors' | 'listDoctors' | 'bookAppointment' | 'viewSchedule'
  | 'recordConsultation' | 'viewMedicalHistory' | 'dailyReport' | 'workloadReport' | 'backup';

export interface CurrentUser { userId: string; username: string; role: Role; doctorId: string; permissions: Permission[] }
export interface Patient {
  patientId: string; fullName: string; dob: string; gender: string; phone: string;
  address: string; bloodGroup: string; active: boolean; registeredOn: string;
}
export interface Window { weekday: number; start: string; end: string }
export interface Doctor { doctorId: string; fullName: string; specialization: string; department: string; userId: string; windows: Window[] }
export type AppointmentStatus = 'SCHEDULED' | 'COMPLETED' | 'CANCELLED' | 'NO_SHOW';
export interface Appointment {
  appointmentId: string; patientId: string; doctorId: string; slotStart: string; bookedBy: string;
  status: AppointmentStatus; patientName?: string;
}
export interface PrescriptionItem { medicine: string; dosage: string; days: number }
export interface Addendum { timestamp: string; authorUserId: string; text: string }
export interface Consultation {
  consultId: string; appointmentId: string; patientId: string; doctorId: string; createdAt: string;
  diagnosis: string; notes: string; items: PrescriptionItem[]; addenda: Addendum[];
}
export interface User { userId: string; username: string; role: Role; active: boolean }
export interface AuditEntry { timestamp: string; userId: string; action: string; targetId: string; outcome: 'SUCCESS' | 'FAILURE' }
export interface DailyReport {
  date: string; total: number; perStatus: Record<AppointmentStatus, number>;
  perDoctor: { doctorId: string; doctorName: string; appointments: Appointment[] }[];
}
export interface WorkloadRow { doctorId: string; doctorName: string; completed: number }
export interface Status { hasUsers: boolean; recovery: { needed: boolean; file?: string; latestBackup?: string } }
