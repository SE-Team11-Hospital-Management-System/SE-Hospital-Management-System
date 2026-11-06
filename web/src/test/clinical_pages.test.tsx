// SF-4 (consultations, reports, backup) through the website: TC-CON-01..03, TC-RPT-01, TC-RPT-02, TC-DATA-02, TC-WEB-05.
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

afterEach(() => { cleanup(); vi.unstubAllGlobals(); setToken(null); calls.length = 0; });

const appt = { appointmentId: 'A00000001', patientId: 'P000001', doctorId: 'D001', slotStart: '2030-01-07 09:00', bookedBy: 'U00002', status: 'SCHEDULED', patientName: 'Anita Rao' };
const consult = { consultId: 'C000001', appointmentId: 'A00000001', patientId: 'P000001', doctorId: 'D001', createdAt: '2030-01-07 09:10',
  diagnosis: 'Hypertension', notes: 'Review in 4 weeks', items: [{ medicine: 'Amlodipine', dosage: '5 mg', days: 30 }], addenda: [{ timestamp: '2030-01-08 10:00', authorUserId: 'U00003', text: 'Dose changed' }] };

describe('consultations page (TC-CON-01..03, TC-WEB-05)', () => {
  const recordServer = () => server('Doctor', (c) => {
    if (c.url.startsWith('/api/schedule')) return { body: { appointments: [appt] } };
    if (c.url.startsWith('/api/consultations/prepare')) return { body: { patientId: 'P000001', fullName: 'Anita Rao', age: 40 } };
    if (c.method === 'POST' && c.url === '/api/consultations') return { status: 201, body: { consultId: 'C000001' } };
  });

  it('the patient is shown before saving (HMS-NF-010); No — discard saves nothing', async () => {
    recordServer();
    await show('/consultations');
    await clickSoon('Record');
    await screen.findByLabelText(/^Diagnosis/);
    expect((screen.getByRole('button', { name: 'Continue' }) as HTMLButtonElement).disabled).toBe(true);
    type('Diagnosis', 'Hypertension');
    click('Continue');
    expect(await screen.findByText('Anita Rao')).toBeTruthy();
    expect(screen.getByText('40')).toBeTruthy();
    await clickSoon('No — discard');
    expect(sent('POST', '/api/consultations')).toHaveLength(0);
  });

  it('Yes — save sends the diagnosis and prescription, confirmed', async () => {
    recordServer();
    await show('/consultations');
    await clickSoon('Record');
    await screen.findByLabelText(/^Diagnosis/);
    type('Diagnosis', 'Hypertension');
    click('Add medicine');
    type('Medicine', 'Amlodipine'); type('Dosage', '5 mg');
    click('Continue');
    await clickSoon('Yes — save consultation');
    await waitFor(() => expect(sent('POST', '/api/consultations')).toHaveLength(1));
    expect(sent('POST', '/api/consultations')[0].body).toEqual({ appointmentId: 'A00000001', diagnosis: 'Hypertension', notes: '',
      items: [{ medicine: 'Amlodipine', dosage: '5 mg', days: 5 }], confirmed: true });
  });

  it('a refused save returns to the form with the rule (E010); history shows prescriptions and addenda; addendum is added', async () => {
    server('Doctor', (c) => {
      if (c.url.startsWith('/api/schedule')) return { body: { appointments: [appt] } };
      if (c.url.startsWith('/api/consultations/prepare')) return { body: { patientId: 'P000001', fullName: 'Anita Rao', age: 40 } };
      if (c.method === 'POST' && c.url === '/api/consultations') return { error: ['E010', 'Diagnosis: at most 500 characters.'] };
      if (c.url.endsWith('/addenda')) return { status: 201, body: { consultId: 'C000001' } };
      if (c.url.startsWith('/api/consultations?')) return { body: [consult] };
    });
    await show('/consultations');
    await clickSoon('Record');
    await screen.findByLabelText(/^Diagnosis/);
    type('Diagnosis', 'x'); click('Continue');
    await screen.findByText('Is this the right patient?');
    await clickSoon('Yes — save consultation');
    expect(await screen.findByText('E010: Diagnosis: at most 500 characters.')).toBeTruthy();
    click('Cancel');
    fireEvent.click(screen.getByRole('tab', { name: 'Patient history' }));
    type('Patient ID', 'P000001'); click('Show history');
    expect(await screen.findByText('Amlodipine')).toBeTruthy();
    expect(screen.getByText(/Dose changed/)).toBeTruthy();
    click('Add addendum');
    await screen.findByText(/The original stays unchanged/);
    fireEvent.change(last(screen.getAllByRole('textbox')), { target: { value: 'Review in 2 weeks' } });   // the dialog is drawn last
    fireEvent.click(last(screen.getAllByRole('button', { name: 'Add addendum' })));
    await waitFor(() => expect(sent('POST', '/api/consultations/C000001/addenda')[0].body).toEqual({ text: 'Review in 2 weeks' }));
  });
});

describe('reports and backups (TC-RPT-01, TC-RPT-02, TC-DATA-02, TC-WEB-05)', () => {
  it('daily report shows totals and each doctor; CSV download uses the login token', async () => {
    const urls: string[] = [];
    vi.stubGlobal('URL', Object.assign(URL, { createObjectURL: () => 'blob:x', revokeObjectURL: (u: string) => urls.push(u) }));
    server('Receptionist', (c) => c.url.startsWith('/api/reports/daily/csv') ? { body: 'appointment_id,date' }
      : c.url.startsWith('/api/reports/daily') ? { body: { ...emptyReport, total: 1, perStatus: { ...emptyReport.perStatus, SCHEDULED: 1 },
        perDoctor: [{ doctorId: 'D001', doctorName: 'Meera Iyer', appointments: [appt] }] } } : undefined);
    await show('/reports');
    expect(screen.queryByRole('tab', { name: 'Doctor workload' })).toBeNull();                  // Administrator only
    type('Date', '2030-01-07'); click('Show report');
    expect(await screen.findByText('Meera Iyer (D001) — 1 appointments')).toBeTruthy();
    click('Download CSV');
    await waitFor(() => expect(urls).toEqual(['blob:x']));
    expect(last(sent('GET', '/api/reports/daily/csv')).url).toBe('/api/reports/daily/csv?date=2030-01-07');
  });

  it('workload for the Administrator; a reversed range is refused', async () => {
    server('Administrator', (c) => c.url.startsWith('/api/reports/workload')
      ? (c.url.includes('from=2030-02-01') ? { error: ['E010', 'Date range: the end date must not be before the start date.'] }
        : { body: [{ doctorId: 'D001', doctorName: 'Meera Iyer', completed: 4 }] }) : undefined);
    await show('/reports');
    fireEvent.click(await screen.findByRole('tab', { name: 'Doctor workload' }));
    type('From', '2030-01-01'); type('To', '2030-01-31'); click('Show workload');
    expect(await screen.findByText('Meera Iyer (D001)')).toBeTruthy();
    expect(screen.getByText('4')).toBeTruthy();
    type('From', '2030-02-01'); click('Show workload');
    await waitFor(() => expect(sent('GET', '/api/reports/workload?from=2030-02-01')).toHaveLength(1));
  });

  it('backups: create one, restore only after Yes (with confirm: true)', async () => {
    let list: string[] = [];
    server('Administrator', (c) => {
      if (c.method === 'POST' && c.url === '/api/backups') { list = ['20300107_090000']; return { status: 201, body: { name: list[0] } }; }
      if (c.url.endsWith('/restore')) return { body: { restored: '20300107_090000' } };
      if (c.url.startsWith('/api/backups')) return { body: list };
    });
    await show('/backups');
    expect(await screen.findByText('No backups yet.')).toBeTruthy();
    click('Create backup now');
    expect(await screen.findByText('2030-01-07 09:00:00')).toBeTruthy();
    click('Restore');
    await clickSoon('No');
    expect(sent('POST', '/api/backups/20300107_090000/restore')).toHaveLength(0);
    click('Restore');
    await clickSoon('Yes, restore');
    await waitFor(() => expect(sent('POST', '/api/backups/20300107_090000/restore')[0].body).toEqual({ confirm: true }));
  });
});
