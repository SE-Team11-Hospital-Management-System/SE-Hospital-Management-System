// SF-1 (login, roles, user accounts, audit log) through the website: TC-AUTH-01, TC-AUTH-02, TC-AUTH-04, TC-AUTH-05,
// TC-SEC-03, TC-SEC-04, TC-WEB-01, TC-WEB-02. The server is replaced by a fake that answers like the real one.
import { MantineProvider } from '@mantine/core';
import { Notifications } from '@mantine/notifications';
import { act, cleanup, configure, fireEvent, render, screen, waitFor } from '@testing-library/react';
import { afterEach, describe, expect, it, vi } from 'vitest';
import { MemoryRouter } from 'react-router-dom';
import App from '../App';
import { api, ApiError, setToken } from '../api';
import { AuthProvider } from '../auth';
import type { CurrentUser } from '../types';

const admin: CurrentUser = {
  userId: 'U00001', username: 'admin01', role: 'Administrator', doctorId: '',
  permissions: ['changePassword', 'manageUsers', 'viewAuditLog', 'searchPatient', 'listDoctors', 'viewSchedule', 'dailyReport', 'backup'],
};
type Reply = { status?: number; body?: unknown; error?: [string, string, string?] };
type Call = { method: string; url: string; body: Record<string, unknown> };
const calls: Call[] = [];
configure({ asyncUtilTimeout: 4000 });                                   // slower CI machines
vi.setConfig({ testTimeout: 20000 });

// Answers for pages the tests do not look at (the dashboard of the logged-in user).
const fallback = (c: Call): Reply => c.url.startsWith('/api/reports/daily')
  ? { body: { date: '2030-01-07', total: 0, perStatus: { SCHEDULED: 0, COMPLETED: 0, CANCELLED: 0, NO_SHOW: 0 }, perDoctor: [] } }
  : { body: [] };

// handler(method, url, body) -> Reply; an error reply looks exactly like the C++ server's {"error": {...}}
function server(handler: (c: Call) => Reply | undefined) {
  vi.stubGlobal('fetch', vi.fn(async (url: string, init?: RequestInit) => {
    const c: Call = { method: init?.method ?? 'GET', url, body: init?.body ? JSON.parse(String(init.body)) : {} };
    calls.push(c);
    const r = handler(c) ?? fallback(c);
    if (r.error) {
      const [code, message, details] = r.error;
      return new Response(JSON.stringify({ error: { code, message, details: details ?? '' } }), { status: r.status ?? 400 });
    }
    return new Response(JSON.stringify(r.body ?? {}), { status: r.status ?? 200 });
  }));
}
const last = <T,>(a: T[]): T => a[a.length - 1];
const posted = (url: string) => calls.filter((c) => c.method === 'POST' && c.url.startsWith(url));
const show = (path = '/') =>
  render(<MantineProvider><Notifications /><MemoryRouter initialEntries={[path]}><AuthProvider><App /></AuthProvider></MemoryRouter></MantineProvider>);
// Required fields show a " *" after their label.
const field = (label: string) => screen.getByLabelText(new RegExp(`^${label}\\s*\\*?$`)) as HTMLInputElement;
const type = (label: string, value: string) => fireEvent.change(field(label), { target: { value } });
const click = (name: string) => fireEvent.click(screen.getByRole('button', { name }));
const status = (s: object) => ({ body: { hasUsers: true, recovery: { needed: false }, ...s } });

afterEach(() => { cleanup(); vi.unstubAllGlobals(); vi.useRealTimers(); setToken(null); calls.length = 0; });

describe('login page (TC-AUTH-01, TC-AUTH-02, TC-WEB-01)', () => {
  it('shows the generic E001 message, clears the password, then logs in', async () => {
    let tries = 0;
    server((c) => {
      if (c.url.startsWith('/api/status')) return status({});
      if (c.url === '/api/login') return ++tries === 1 ? { status: 401, error: ['E001', 'Invalid username or password.'] } : { body: { token: 't1', user: admin } };
    });
    show();
    await screen.findByRole('button', { name: 'Log in' });
    type('Username', 'admin01'); type('Password', 'Wrong9999'); click('Log in');
    expect(await screen.findByText('E001: Invalid username or password.')).toBeTruthy();
    expect(field('Password').value).toBe('');
    type('Password', 'Admin1234'); click('Log in');
    expect(await screen.findByText('Welcome, admin01')).toBeTruthy();
    expect(posted('/api/login')[1].body).toEqual({ username: 'admin01', password: 'Admin1234' });
  });

  it('shows the lock-out message with the minutes left (E002)', async () => {
    server((c) => c.url.startsWith('/api/status') ? status({}) : { status: 423, error: ['E002', 'Account locked. Try again in 15 minutes.'] });
    show();
    await screen.findByRole('button', { name: 'Log in' });
    type('Username', 'rec01'); type('Password', 'Rec@1234'); click('Log in');
    expect(await screen.findByText('E002: Account locked. Try again in 15 minutes.')).toBeTruthy();
  });

  it('first run: the two passwords must match, then the Administrator is created', async () => {
    let users = false;
    server((c) => {
      if (c.url.startsWith('/api/status')) return status({ hasUsers: users });
      if (c.url === '/api/setup') { users = true; return { status: 201, body: { userId: 'U00001' } }; }
    });
    show();
    await screen.findByText('First run');
    type('Administrator username', 'admin01'); type('Password', 'Admin1234'); type('Repeat password', 'Admin9999');
    expect(screen.getByText('E010: Password: the two entries do not match.')).toBeTruthy();
    click('Create Administrator');
    expect(posted('/api/setup')).toHaveLength(0);
    type('Repeat password', 'Admin1234'); click('Create Administrator');
    expect(await screen.findByRole('button', { name: 'Log in' })).toBeTruthy();
    expect(posted('/api/setup')[0].body).toEqual({ username: 'admin01', password: 'Admin1234' });
  });

  it('says so when the HMS server cannot be reached (E099)', async () => {
    vi.stubGlobal('fetch', vi.fn(async () => { throw new TypeError('Failed to fetch'); }));
    show();
    expect(await screen.findByText(/Cannot reach the HMS server/)).toBeTruthy();
  });
});

describe('pages of a logged-in Administrator (TC-AUTH-04, TC-AUTH-05, TC-SEC-03, TC-WEB-02)', () => {
  const loggedIn = (extra: (c: Call) => Reply | undefined) => {
    server((c) => {
      if (c.url.startsWith('/api/me')) return { body: admin };
      if (c.url.startsWith('/api/status')) return status({});
      return extra(c);
    });
    setToken('t1');
  };
  const users = [{ userId: 'U00001', username: 'admin01', role: 'Administrator', active: true },
    { userId: 'U00002', username: 'rec01', role: 'Receptionist', active: true },
    { userId: 'U00003', username: 'doc01', role: 'Doctor', active: false }];

  it('user accounts: create (duplicate refused), deactivate and reactivate; no button for my own account', async () => {
    let created = 0;
    loggedIn((c) => {
      if (c.url === '/api/users' && c.method === 'POST') return ++created === 1 ? { status: 409, error: ['E011', 'Username already exists.'] } : { status: 201, body: { userId: 'U00004' } };
      if (c.url.startsWith('/api/users/')) return { body: { active: c.body.active } };
      if (c.url.startsWith('/api/users')) return { body: users };
    });
    show('/users');
    await screen.findByText('rec01');
    expect(screen.getAllByRole('button', { name: 'Deactivate' })).toHaveLength(1);      // rec01 only, never admin01 itself
    click('Deactivate');
    await waitFor(() => expect(posted('/api/users/U00002/status')[0].body).toEqual({ active: false }));
    click('Reactivate');
    await waitFor(() => expect(posted('/api/users/U00003/status')[0].body).toEqual({ active: true }));
    fireEvent.click(screen.getAllByRole('button', { name: 'Create user' })[0]);
    await screen.findByLabelText(/^Temporary password/);
    type('Username', 'rec01'); type('Temporary password', 'Rec@5678');
    fireEvent.click(last(screen.getAllByRole('button', { name: 'Create user' })));
    expect(await screen.findByText('E011: Username already exists.')).toBeTruthy();
    type('Username', 'rec02');
    fireEvent.click(last(screen.getAllByRole('button', { name: 'Create user' })));
    await waitFor(() => expect(screen.queryByLabelText(/^Temporary password/)).toBeNull());   // the form closes
    expect(last(posted('/api/users').filter((c) => c.url === '/api/users')).body).toEqual({ username: 'rec02', password: 'Rec@5678', role: 'Receptionist' });
  });

  it('audit log: shows names, filters by date range and reports a bad range (E010)', async () => {
    loggedIn((c) => {
      if (c.url.startsWith('/api/users')) return { body: users };
      if (c.url.startsWith('/api/audit')) {
        if (c.url.includes('from=2030-02-01')) return { status: 400, error: ['E010', 'Date range: the end date must not be before the start date.'] };
        if (c.url.includes('from=2030-01-07')) return { body: [] };
        return { body: [{ timestamp: '2030-01-07T09:00:00', userId: 'U00002', action: 'LOGIN_FAIL', targetId: 'U00002', outcome: 'FAILURE' },
          { timestamp: '2030-01-07T09:01:00', userId: '-', action: 'INTEGRITY_FAIL', targetId: 'patients.dat', outcome: 'FAILURE' }] };
      }
    });
    show('/audit');
    expect(await screen.findByText('rec01 (U00002)')).toBeTruthy();
    expect(screen.getByText('2 entries')).toBeTruthy();
    expect(screen.getByText('2030-01-07 09:00:00')).toBeTruthy();
    type('From', '2030-01-07'); type('To', '2030-01-07'); click('Show');
    expect(await screen.findByText('No entries.')).toBeTruthy();
    type('From', '2030-02-01'); click('Show');
    await waitFor(() => expect(calls.some((c) => c.url.includes('from=2030-02-01&to=2030-01-07'))).toBe(true));
    expect(screen.getByText('No entries.')).toBeTruthy();            // the refused range (E010) leaves the last result shown
  });

  it('change my password: repeat must match, server rule is shown, success clears the form', async () => {
    let n = 0;
    loggedIn((c) => c.url === '/api/password'
      ? (++n === 1 ? { status: 401, error: ['E001', 'Current password: incorrect.'] } : { body: { changed: true } }) : undefined);
    show('/password');
    await screen.findByText('Change my password', { selector: 'h2' });
    type('Current password', 'Wrong1234'); type('New password', 'NewPass99'); type('Repeat new password', 'NewPass98');
    expect(screen.getByText('E010: New password: the two entries do not match.')).toBeTruthy();
    expect((screen.getByRole('button', { name: 'Change password' }) as HTMLButtonElement).disabled).toBe(true);
    type('Repeat new password', 'NewPass99'); click('Change password');
    expect(await screen.findByText('E001: Current password: incorrect.')).toBeTruthy();
    type('Current password', 'Admin1234'); click('Change password');
    await waitFor(() => expect(field('New password').value).toBe(''));   // success empties the form
    expect(last(posted('/api/password')).body).toEqual({ current: 'Admin1234', new: 'NewPass99' });
  });

  it('an expired session (E003 from the server) goes back to the login page', async () => {
    loggedIn((c) => c.url.startsWith('/api/users') ? { status: 401, error: ['E003', 'Session expired. Please log in again.'] } : undefined);
    show('/users');
    expect(await screen.findByRole('button', { name: 'Log in' })).toBeTruthy();
    expect(sessionStorage.getItem('hms.token')).toBeNull();               // the token is forgotten
  });

  it('logs out by itself after 10 minutes without activity (TC-SEC-04)', async () => {
    loggedIn(() => undefined);
    show('/');
    await screen.findByText('Welcome, admin01');
    vi.useFakeTimers({ toFake: ['setInterval', 'clearInterval', 'Date'], now: Date.now() + 60_000 });
    cleanup();
    show('/');
    await waitFor(() => expect(screen.getByText('Welcome, admin01')).toBeTruthy());
    await act(async () => { vi.advanceTimersByTime(9 * 60_000); });
    expect(screen.queryByRole('button', { name: 'Log in' })).toBeNull();
    fireEvent.keyDown(window);                                        // activity resets the 10 minutes
    await act(async () => { vi.advanceTimersByTime(9 * 60_000); });
    expect(screen.queryByRole('button', { name: 'Log in' })).toBeNull();
    await act(async () => { vi.advanceTimersByTime(2 * 60_000); });
    vi.useRealTimers();
    expect(await screen.findByRole('button', { name: 'Log in' })).toBeTruthy();
    expect(posted('/api/logout').length).toBeGreaterThan(0);
  });
});

describe('api helper', () => {
  it('turns server errors into ApiError with the HMS code and downloads files with the token', async () => {
    server((c) => c.url.startsWith('/api/bad') ? { status: 500, body: 'not json' } : { body: { ok: true } });
    setToken('t9');
    await expect(api.get('/api/bad')).rejects.toMatchObject({ code: 'E099' });
    const err = new ApiError('E030', 'Slot taken.', 'a, b', 409);
    expect(err.text).toBe('E030: Slot taken.');
    const created: string[] = [];
    vi.stubGlobal('URL', Object.assign(URL, { createObjectURL: () => 'blob:x', revokeObjectURL: (u: string) => created.push(u) }));
    await api.download('/api/reports/daily/csv?date=2030-01-07', 'daily.csv');
    expect(created).toEqual(['blob:x']);
    const sent = last((fetch as unknown as { mock: { calls: [string, RequestInit][] } }).mock.calls);
    expect((sent[1].headers as Record<string, string>).Authorization).toBe('Bearer t9');
    server(() => ({ status: 403, error: ['E004', 'You are not authorised to perform this action.'] }));
    await expect(api.download('/api/reports/daily/csv', 'x.csv')).rejects.toMatchObject({ code: 'E004' });
  });
});
