// SF-2 (patients, website foundation, data integrity) through the website: TC-PAT-01..06, TC-SEC-06, TC-WEB-03.
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
const submit = (button: string) => fireEvent.submit(screen.getByRole('button', { name: button }).closest('form')!);   // the server checks the fields
const clickSoon = async (name: string | RegExp) => fireEvent.click(await screen.findByRole('button', { name }));   // in a dialog that is opening
const pick = async (label: string, option: string | RegExp) => {          // a Mantine drop-down list
  fireEvent.click(field(label));
  fireEvent.click(await screen.findByRole('option', { name: option, hidden: true }));
};

afterEach(() => { cleanup(); vi.unstubAllGlobals(); setToken(null); calls.length = 0; });

const anita = { patientId: 'P000001', fullName: 'Anita Rao', dob: '1990-05-14', gender: 'F', phone: '9876543210', address: 'MG Road', bloodGroup: 'B+', active: true, registeredOn: '2030-01-07' };

describe('patients page (TC-PAT-01..06, TC-WEB-03)', () => {
  it('search shows results with paging; an empty search sends nothing', async () => {
    server('Receptionist', (c) => c.url.startsWith('/api/patients?')
      ? { body: { items: [anita], page: c.url.includes('page=2') ? 2 : 1, hasNext: !c.url.includes('page=2') } } : undefined);
    await show('/patients');
    await screen.findByRole('heading', { name: 'Patients' });
    click('Search');
    expect(sent('GET', '/api/patients?')).toHaveLength(0);
    fireEvent.change(screen.getByPlaceholderText('e.g. anita'), { target: { value: 'anita' } });
    click('Search');
    expect(await screen.findByText('Anita Rao')).toBeTruthy();
    expect(last(sent('GET', '/api/patients?')).url).toContain('by=name&term=anita&page=1');
    click('Next');
    await screen.findByText('Page 2');
    click('Previous');
    await screen.findByText('Page 1');
  });

  it('register: a duplicate asks first (HMS-F-015); No saves nothing, Yes registers a second record', async () => {
    let n = 0;
    server('Receptionist', (c) => {
      if (c.method === 'POST' && c.url === '/api/patients') {
        if (++n === 1) return { status: 409, error: ['E011', 'A matching patient exists (ID P000001). Confirm to continue.', 'P000001'] };
        return { status: 201, body: { ...anita, patientId: 'P000002' } };
      }
    });
    await show('/patients');
    click('Register patient');
    await screen.findByLabelText(/^Full name/);
    type('Full name', 'Anita Rao'); type('Date of birth', '1990-05-14'); type('Phone', '9876543210'); type('Address', 'MG Road');
    await pick('Gender', 'Female');
    await pick('Blood group', 'B+');
    submit('Save');
    expect(await screen.findByText('P000001')).toBeTruthy();
    await clickSoon('Yes, register anyway');
    expect(await screen.findByText('P000002')).toBeTruthy();
    expect(last(sent('POST', '/api/patients')).body).toMatchObject({ fullName: 'Anita Rao', gender: 'F', bloodGroup: 'B+', confirmDuplicate: true });
  });

  it('register shows the field rule from the server (E010); edit keeps ID, date of birth and gender', async () => {
    server('Administrator', (c) => {
      if (c.method === 'POST' && c.url === '/api/patients') return { error: ['E010', 'Phone: must be exactly 10 digits.'] };
      if (c.method === 'PUT') return { body: { ...anita, phone: '9000000001' } };
      if (c.url.startsWith('/api/patients?')) return { body: { items: [anita], page: 1, hasNext: false } };
    });
    await show('/patients');
    click('Register patient');
    await screen.findByLabelText(/^Full name/);
    type('Phone', '98765'); submit('Save');
    expect(await screen.findByText('E010: Phone: must be exactly 10 digits.')).toBeTruthy();
    click('Cancel');
    fireEvent.change(screen.getByPlaceholderText('e.g. anita'), { target: { value: 'anita' } });
    click('Search');
    await screen.findByText('Anita Rao');
    click('Edit');
    await screen.findByText('Patient ID, date of birth and gender cannot be changed.');
    expect(field('Date of birth').disabled).toBe(true);
    fireEvent.change(last(screen.getAllByLabelText(/^Phone/)), { target: { value: '9000000001' } });
    fireEvent.submit(last(screen.getAllByRole('button', { name: 'Save' })).closest('form')!);
    await waitFor(() => expect(sent('PUT', '/api/patients/P000001')).toHaveLength(1));
    expect(sent('PUT', '/api/patients/P000001')[0].body).toEqual({ fullName: 'Anita Rao', phone: '9000000001', address: 'MG Road', bloodGroup: 'B+' });
  });

  it('only the Administrator deactivates, after confirming; the record stays and shows Inactive (HMS-F-014)', async () => {
    server('Administrator', (c) => {
      if (c.url.endsWith('/deactivate')) return { body: { ...anita, active: false } };
      if (c.url.startsWith('/api/patients?')) return { body: { items: [anita], page: 1, hasNext: false } };
    });
    await show('/patients');
    fireEvent.change(await screen.findByPlaceholderText('e.g. anita'), { target: { value: 'anita' } });
    click('Search');
    await screen.findByText('Anita Rao');
    click('Deactivate');
    await screen.findByText(/cannot get new appointments/);
    fireEvent.click(screen.getAllByRole('button', { name: 'Deactivate' }).filter((b) => b.closest('[role="dialog"]'))[0]);
    expect(await screen.findByText('Inactive')).toBeTruthy();
    expect(sent('POST', '/api/patients/P000001/deactivate')).toHaveLength(1);
  });

  it('a Receptionist sees no Deactivate button; a Doctor can only search', async () => {
    server('Doctor', (c) => c.url.startsWith('/api/patients?') ? { body: { items: [anita], page: 1, hasNext: false } } : undefined);
    await show('/patients');
    fireEvent.change(await screen.findByPlaceholderText('e.g. anita'), { target: { value: 'anita' } });
    click('Search');
    await screen.findByText('Anita Rao');
    expect(screen.queryByRole('button', { name: 'Register patient' })).toBeNull();
    expect(screen.queryByRole('button', { name: 'Edit' })).toBeNull();
    expect(screen.queryByRole('button', { name: 'Deactivate' })).toBeNull();
  });
});

describe('damaged data file (TC-SEC-06)', () => {
  const status = (s: object) => ({ body: { hasUsers: true, recovery: { needed: false }, ...s } });
  it('only an Administrator restores the latest backup, then the login comes back', async () => {
    let damaged = true;
    vi.stubGlobal('fetch', vi.fn(async (url: string, init?: RequestInit) => {
      const body = init?.body ? JSON.parse(String(init.body)) : {};
      calls.push({ method: init?.method ?? 'GET', url, body });
      if (url === '/api/recovery/restore') {
        if (body.username !== 'admin01') return new Response(JSON.stringify({ error: { code: 'E004', message: 'You are not authorised to perform this action.', details: '' } }), { status: 403 });
        damaged = false;
        return new Response(JSON.stringify({ restored: '20301107_090000' }));
      }
      const s = status(damaged ? { recovery: { needed: true, file: 'patients.dat', latestBackup: '20301107_090000' } } : {});
      return new Response(JSON.stringify(s.body));
    }));
    await show('/');
    expect(await screen.findByText('E040: Data file damaged')).toBeTruthy();
    expect(screen.getByText('patients.dat')).toBeTruthy();
    type('Administrator username', 'rec01'); type('Password', 'Rec@1234'); click('Restore latest backup');
    expect(await screen.findByText('E004: You are not authorised to perform this action.')).toBeTruthy();
    type('Administrator username', 'admin01'); type('Password', 'Admin1234'); click('Restore latest backup');
    expect(await screen.findByRole('button', { name: 'Log in' })).toBeTruthy();
  });

  it('cannot restore when there is no backup', async () => {
    vi.stubGlobal('fetch', vi.fn(async () => new Response(JSON.stringify(status({ recovery: { needed: true, file: 'users.dat', latestBackup: '' } }).body))));
    await show('/');
    expect(await screen.findByText(/No backup is available/)).toBeTruthy();
    expect((screen.getByRole('button', { name: 'Restore latest backup' }) as HTMLButtonElement).disabled).toBe(true);
  });
});
