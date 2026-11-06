// SF-3 (doctors, appointments, schedules) through the website: TC-DOC-01, TC-APT-01..06, TC-WEB-04.
// The server is replaced by a fake that answers like the real one.
import { MantineProvider } from '@mantine/core';
import { Notifications } from '@mantine/notifications';
import { cleanup, configure, fireEvent, render, screen, waitFor } from '@testing-library/react';
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
type Reply = { status?: number; body?: unknown; error?: [string, string, string?] };
type Call = { method: string; url: string; body: Record<string, unknown> };
const calls: Call[] = [];
configure({ asyncUtilTimeout: 4000 });                                   // slower CI machines
vi.setConfig({ testTimeout: 20000 });
const emptyReport = { date: '2030-01-07', total: 0, perStatus: { SCHEDULED: 0, COMPLETED: 0, CANCELLED: 0, NO_SHOW: 0 }, perDoctor: [] };

// A fake HMS server: answers like the C++ server, including its {"error": {code, message, details}} shape.
function server(role: Role, handler: (c: Call) => Reply | undefined) {
  const user: CurrentUser = { userId: 'U00001', username: 'tester', role, doctorId: role === 'Doctor' ? 'D001' : '', permissions: perms[role] };
  vi.stubGlobal('fetch', vi.fn(async (url: string, init?: RequestInit) => {
    const c: Call = { method: init?.method ?? 'GET', url, body: init?.body ? JSON.parse(String(init.body)) : {} };
    calls.push(c);
    const r = url.startsWith('/api/me') ? { body: user } : handler(c) ?? (url.startsWith('/api/reports/daily') ? { body: emptyReport }
      : url.startsWith('/api/schedule') ? { body: { appointments: [] } } : { body: [] });
    if (r.error) {
      const [code, message, details] = r.error;
      return new Response(JSON.stringify({ error: { code, message, details: details ?? '' } }), { status: r.status ?? 400 });
    }
    return new Response(JSON.stringify(r.body ?? {}), { status: r.status ?? 200 });
  }));
  setToken('t1');
}
// Opens a page and waits until its heading is there (the user is loaded first).
const show = async (path: string) => {
  render(<MantineProvider><Notifications /><MemoryRouter initialEntries={[path]}><AuthProvider><App /></AuthProvider></MemoryRouter></MantineProvider>);
  await screen.findByRole('heading', { level: 2 });
};
const last = <T,>(a: T[]): T => a[a.length - 1];
const sent = (method: string, url: string) => calls.filter((c) => c.method === method && c.url.startsWith(url));
// Required fields show a " *" after their label.
const field = (label: string) => screen.getAllByLabelText(new RegExp(`^${label}\\s*\\*?$`))[0] as HTMLInputElement;
const type = (label: string, value: string) => fireEvent.change(field(label), { target: { value } });
const click = (name: string | RegExp) => fireEvent.click(screen.getByRole('button', { name }));
const clickSoon = async (name: string | RegExp) => fireEvent.click(await screen.findByRole('button', { name }));   // in a dialog that is opening
const pick = async (label: string, option: string | RegExp) => {          // a Mantine drop-down list
  fireEvent.click(field(label));
  fireEvent.click(await screen.findByRole('option', { name: option, hidden: true }));
};

afterEach(() => { cleanup(); vi.unstubAllGlobals(); setToken(null); calls.length = 0; });

const meera = { doctorId: 'D001', fullName: 'Meera Iyer', specialization: 'Cardiology', department: 'Cardiac', userId: 'U00003', windows: [{ weekday: 1, start: '09:00', end: '13:00' }] };
const anita = { patientId: 'P000001', fullName: 'Anita Rao', dob: '1990-05-14', gender: 'F', phone: '9876543210', address: 'MG Road', bloodGroup: 'B+', active: true, registeredOn: '2030-01-07' };
const appt = { appointmentId: 'A00000001', patientId: 'P000001', doctorId: 'D001', slotStart: '2030-01-07 09:00', bookedBy: 'U00002', status: 'SCHEDULED' };

describe('doctors page (TC-DOC-01, TC-WEB-04)', () => {
  it('filters by specialization; Administrator adds a doctor linked to a Doctor account', async () => {
    server('Administrator', (c) => {
      if (c.url.startsWith('/api/users')) return { body: [{ userId: 'U00003', username: 'doc01', role: 'Doctor', active: true }, { userId: 'U00002', username: 'rec01', role: 'Receptionist', active: true }] };
      if (c.method === 'POST' && c.url === '/api/doctors') return { status: 201, body: { ...meera, doctorId: 'D002' } };
      if (c.url.startsWith('/api/doctors')) return { body: [meera] };
    });
    await show('/doctors');
    expect(await screen.findByText('Meera Iyer')).toBeTruthy();
    expect(screen.getByText('Mon 09:00-13:00')).toBeTruthy();
    type('Specialization', 'Cardiology'); click('Filter');
    await waitFor(() => expect(last(sent('GET', '/api/doctors')).url).toBe('/api/doctors?specialization=Cardiology'));
    click('Add doctor');
    await screen.findByText('Create it first under User accounts (role Doctor)');
    fireEvent.change(screen.getAllByLabelText(/^Full name/)[0], { target: { value: 'Ravi Shah' } });
    fireEvent.change(last(screen.getAllByLabelText(/^Specialization/)), { target: { value: 'Orthopaedics' } });
    type('Department', 'Ortho');
    await pick('Doctor login account', 'doc01 (U00003)');
    expect(screen.queryByRole('option', { name: /rec01/, hidden: true })).toBeNull();        // only Doctor accounts are offered
    click('Save');
    await waitFor(() => expect(sent('POST', '/api/doctors')).toHaveLength(1));
    expect(sent('POST', '/api/doctors')[0].body).toEqual({ fullName: 'Ravi Shah', specialization: 'Orthopaedics', department: 'Ortho', userId: 'U00003' });
  });

  it('working hours: one block per ticked weekday; a refused change shows the rule (E010)', async () => {
    let n = 0;
    server('Administrator', (c) => {
      if (c.url.endsWith('/availability')) return ++n === 1 ? { error: ['E010', 'Working hours: start and end on a 30-minute boundary between 08:00 and 20:00, start before end.'] } : { body: meera };
      if (c.url.startsWith('/api/doctors')) return { body: [meera] };
    });
    await show('/doctors');
    await screen.findByText('Meera Iyer');
    click('Hours');
    fireEvent.click(await screen.findByLabelText('Wednesday'));
    click('Save hours');
    expect(await screen.findByText(/E010: Working hours/)).toBeTruthy();
    click('Save hours');
    await waitFor(() => expect(sent('PUT', '/api/doctors/D001/availability')).toHaveLength(2));
    expect(last(sent('PUT', '/api/doctors/D001/availability')).body).toEqual({ windows: [{ weekday: 1, start: '09:00', end: '13:00' }, { weekday: 3, start: '09:00', end: '13:00' }] });
  });

  it('a Receptionist sees the list but no Add, Edit or Hours buttons', async () => {
    server('Receptionist', (c) => c.url.startsWith('/api/doctors') ? { body: [meera] } : undefined);
    await show('/doctors');
    await screen.findByText('Meera Iyer');
    for (const b of ['Add doctor', 'Edit', 'Hours']) expect(screen.queryByRole('button', { name: b })).toBeNull();
  });
});

describe('appointments page (TC-APT-01..06, TC-WEB-04)', () => {
  it('book: unknown patient is E020; a taken slot offers 3 free slots and one click books it', async () => {
    let tries = 0;
    server('Receptionist', (c) => {
      if (c.url === '/api/patients/P999999') return { status: 404, error: ['E020', 'Patient not found.'] };
      if (c.url.startsWith('/api/patients/')) return { body: anita };
      if (c.url.startsWith('/api/doctors')) return { body: [meera] };
      if (c.method === 'POST' && c.url === '/api/appointments') return ++tries === 1
        ? { status: 409, error: ['E030', 'Slot taken. Next free slots: 2030-01-07 09:30, 2030-01-07 10:00, 2030-01-07 10:30', '2030-01-07 09:30, 2030-01-07 10:00, 2030-01-07 10:30'] }
        : { status: 201, body: { ...appt, slotStart: '2030-01-07 10:00' } };
    });
    await show('/appointments');
    type('Patient ID', 'P999999'); click('Find');
    expect(await screen.findByText('E020: Patient not found.')).toBeTruthy();
    type('Patient ID', 'P000001'); click('Find');
    expect(await screen.findByText('Anita Rao')).toBeTruthy();
    expect((screen.getByRole('button', { name: 'Book appointment' }) as HTMLButtonElement).disabled).toBe(true);
    await pick('Doctor', /Meera Iyer/);
    type('Date', '2030-01-07');
    await pick('Time', '09:00');
    click('Book appointment');
    expect(await screen.findByText(/E030: Slot taken/)).toBeTruthy();
    click('2030-01-07 10:00');
    await waitFor(() => expect(sent('POST', '/api/appointments')).toHaveLength(2));
    expect(sent('POST', '/api/appointments')[0].body).toEqual({ patientId: 'P000001', doctorId: 'D001', slot: '2030-01-07 09:00' });
    expect(sent('POST', '/api/appointments')[1].body).toEqual({ patientId: 'P000001', doctorId: 'D001', slot: '2030-01-07 10:00' });
  });

  it('manage: cancel and no-show ask first; a refused change shows E032; reschedule moves it', async () => {
    let list = [appt, { ...appt, appointmentId: 'A00000002', status: 'COMPLETED' }];
    server('Receptionist', (c) => {
      if (c.url.endsWith('/no-show')) return { status: 409, error: ['E032', 'Action not allowed: No-Show only after the start time.'] };
      if (c.url.endsWith('/cancel')) { list = [{ ...appt, status: 'CANCELLED' }, list[1]]; return { body: list[0] }; }
      if (c.url.endsWith('/reschedule')) return { body: { ...appt, slotStart: c.body.slot } };
      if (c.url.startsWith('/api/doctors')) return { body: [meera] };
      if (c.url.startsWith('/api/appointments?')) return { body: list };
    });
    await show('/appointments');
    fireEvent.click(screen.getByRole('tab', { name: /Cancel \/ reschedule/ }));
    type('Patient ID', 'P000001'); click('Show appointments');
    expect(await screen.findByText('A00000002')).toBeTruthy();
    expect(screen.getAllByRole('button', { name: 'Reschedule' })).toHaveLength(1);          // only the Scheduled one
    click('No-show'); await clickSoon('Yes');
    await waitFor(() => expect(sent('POST', '/api/appointments/A00000001/no-show')).toHaveLength(1));
    click('Reschedule');
    await screen.findByRole('button', { name: 'Move appointment' });
    type('Date', '2030-01-08');
    await pick('Time', '11:00');
    click('Move appointment');
    await waitFor(() => expect(sent('POST', '/api/appointments/A00000001/reschedule')).toHaveLength(1));
    expect(sent('POST', '/api/appointments/A00000001/reschedule')[0].body).toEqual({ doctorId: 'D001', slot: '2030-01-08 11:00' });
    click('Cancel'); await clickSoon('Yes');
    await waitFor(() => expect(screen.queryAllByRole('button', { name: 'Reschedule' })).toHaveLength(0));   // cancelled: no more actions
  });
});
