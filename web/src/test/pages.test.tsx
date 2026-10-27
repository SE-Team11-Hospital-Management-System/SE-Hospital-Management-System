// Every page opens without errors for the roles that may use it, against a fake server.
import { MantineProvider } from '@mantine/core';
import { Notifications } from '@mantine/notifications';
import { cleanup, render, screen } from '@testing-library/react';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { MemoryRouter } from 'react-router-dom';
import App from '../App';
import { setToken } from '../api';
import { AuthProvider } from '../auth';
import type { CurrentUser, Role } from '../types';

const perms: Record<Role, CurrentUser['permissions']> = {
  Administrator: ['changePassword', 'manageUsers', 'viewAuditLog', 'registerPatient', 'updatePatient', 'searchPatient', 'deactivatePatient',
    'manageDoctors', 'listDoctors', 'bookAppointment', 'viewSchedule', 'dailyReport', 'workloadReport', 'backup'],
  Receptionist: ['changePassword', 'registerPatient', 'updatePatient', 'searchPatient', 'listDoctors', 'bookAppointment', 'viewSchedule', 'dailyReport'],
  Doctor: ['changePassword', 'searchPatient', 'listDoctors', 'viewSchedule', 'recordConsultation', 'viewMedicalHistory'],
};

const appointment = { appointmentId: 'A00000001', patientId: 'P000001', doctorId: 'D001', slotStart: '2030-01-07 09:00', bookedBy: 'U00002', status: 'SCHEDULED', patientName: 'Anita Rao' };
function fakeServer(role: Role) {
  const user: CurrentUser = { userId: 'U00001', username: 'tester', role, doctorId: role === 'Doctor' ? 'D001' : '', permissions: perms[role] };
  const answer = (url: string): unknown => {
    if (url.startsWith('/api/me')) return user;
    if (url.startsWith('/api/status')) return { hasUsers: true, recovery: { needed: false } };
    if (url.startsWith('/api/schedule')) return { doctorId: 'D001', date: '2030-01-07', appointments: [appointment] };
    if (url.startsWith('/api/reports/daily')) return { date: '2030-01-07', total: 1, perStatus: { SCHEDULED: 1, COMPLETED: 0, CANCELLED: 0, NO_SHOW: 0 }, perDoctor: [{ doctorId: 'D001', doctorName: 'Meera Iyer', appointments: [appointment] }] };
    if (url.startsWith('/api/doctors')) return [{ doctorId: 'D001', fullName: 'Meera Iyer', specialization: 'Cardiology', department: 'Cardiac', userId: 'U00003', windows: [{ weekday: 1, start: '09:00', end: '13:00' }] }];
    if (url.startsWith('/api/users')) return [{ userId: 'U00001', username: 'tester', role, active: true }];
    if (url.startsWith('/api/audit')) return [{ timestamp: '2030-01-07T09:00:00', userId: 'U00001', action: 'LOGIN_OK', targetId: 'U00001', outcome: 'SUCCESS' }];
    if (url.startsWith('/api/backups')) return ['20300107_090000'];
    return [];
  };
  vi.stubGlobal('fetch', vi.fn(async (url: string) => new Response(JSON.stringify(answer(url)), { status: 200 })));
  setToken('test-token');
}

const show = (path: string) =>
  render(<MantineProvider><Notifications /><MemoryRouter initialEntries={[path]}><AuthProvider><App /></AuthProvider></MemoryRouter></MantineProvider>);

// [page, the page's own heading, role]
const pages: [string, RegExp, Role][] = [
  ['/', /^Welcome, tester$/, 'Receptionist'], ['/', /^Welcome, tester$/, 'Doctor'],
  ['/patients', /^Patients$/, 'Receptionist'], ['/doctors', /^Doctors$/, 'Administrator'],
  ['/appointments', /^Appointments$/, 'Receptionist'], ['/schedule', /schedule/i, 'Receptionist'], ['/schedule', /schedule/i, 'Doctor'],
  ['/consultations', /^Consultations$/, 'Doctor'], ['/reports', /^Reports$/, 'Administrator'], ['/users', /^User accounts$/, 'Administrator'],
  ['/audit', /^Audit log$/, 'Administrator'], ['/backups', /^Backup and restore$/, 'Administrator'], ['/password', /^Change my password$/, 'Doctor'],
];

afterEach(() => { cleanup(); vi.unstubAllGlobals(); setToken(null); });

describe('every page opens', () => {
  it.each(pages)('%s shows heading %s for %s', async (path, heading, role) => {
    fakeServer(role);
    show(path);
    expect(await screen.findByRole('heading', { name: heading })).toBeTruthy();
  });

  it('a Receptionist has no Consultations or Audit log page in the menu (HMS-F-003, F-042)', async () => {
    fakeServer('Receptionist');
    show('/');
    await screen.findByRole('heading', { name: /^Welcome, tester$/ });
    expect(screen.queryByText('Consultations')).toBeNull();
    expect(screen.queryByText('Audit log')).toBeNull();
  });

  it('a page the role may not use sends the user back to the dashboard', async () => {
    fakeServer('Doctor');
    show('/backups');
    expect(await screen.findByRole('heading', { name: /^Welcome, tester$/ })).toBeTruthy();
  });
});
