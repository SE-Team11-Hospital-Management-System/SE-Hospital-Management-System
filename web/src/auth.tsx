// Who is logged in, plus the 10-minute idle logout (HMS-F-005). The server checks the same rule on every request.
import { createContext, ReactNode, useCallback, useContext, useEffect, useRef, useState } from 'react';
import { notifications } from '@mantine/notifications';
import { api, hasToken, onSessionExpired, setToken } from './api';
import type { CurrentUser, Permission } from './types';

const IDLE_LIMIT_MS = 10 * 60 * 1000;

interface AuthState {
  user: CurrentUser | null;
  ready: boolean;
  login: (username: string, password: string) => Promise<void>;
  logout: (reason?: string) => Promise<void>;
  can: (p: Permission) => boolean;
}

const AuthContext = createContext<AuthState | null>(null);

export function AuthProvider({ children }: { children: ReactNode }) {
  const [user, setUser] = useState<CurrentUser | null>(null);
  const [ready, setReady] = useState(false);
  const lastActivity = useRef(Date.now());

  const endSession = useCallback((reason?: string) => {
    setToken(null);
    setUser(null);
    if (reason) notifications.show({ color: 'orange', title: 'Logged out', message: reason, autoClose: 8000 });
  }, []);

  useEffect(() => {                                     // reopen the page: keep the session if it is still valid
    onSessionExpired(() => endSession('Session expired. Please log in again.'));
    if (!hasToken()) { setReady(true); return; }
    api.get<CurrentUser>('/api/me').then(setUser).catch(() => setToken(null)).finally(() => setReady(true));
  }, [endSession]);

  useEffect(() => {                                     // HMS-F-005: 10 minutes without any activity
    if (!user) return;
    const touch = () => { lastActivity.current = Date.now(); };
    const events = ['mousemove', 'mousedown', 'keydown', 'scroll', 'touchstart'];
    events.forEach((e) => window.addEventListener(e, touch, { passive: true }));
    const timer = window.setInterval(() => {
      if (Date.now() - lastActivity.current > IDLE_LIMIT_MS) {
        api.post('/api/logout').catch(() => undefined);
        endSession('Session expired after 10 minutes without activity. Please log in again.');
      }
    }, 15000);
    return () => { events.forEach((e) => window.removeEventListener(e, touch)); window.clearInterval(timer); };
  }, [user, endSession]);

  const login = async (username: string, password: string) => {
    const r = await api.post<{ token: string; user: CurrentUser }>('/api/login', { username, password });
    setToken(r.token);
    lastActivity.current = Date.now();
    setUser(r.user);
  };
  const logout = async (reason?: string) => {
    await api.post('/api/logout').catch(() => undefined);
    endSession(reason);
  };
  const can = (p: Permission) => !!user?.permissions.includes(p);

  return <AuthContext.Provider value={{ user, ready, login, logout, can }}>{children}</AuthContext.Provider>;
}

export function useAuth() {
  const ctx = useContext(AuthContext);
  if (!ctx) throw new Error('useAuth outside AuthProvider');
  return ctx;
}
