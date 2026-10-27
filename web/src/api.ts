// Talks to the C++ server. Every error comes back as ApiError with the HMS error code (E001 ... E099).
export class ApiError extends Error {
  constructor(public code: string, message: string, public details: string = '', public status = 0) {
    super(message);
  }
  get text() { return `${this.code}: ${this.message}`; }
}

const KEY = 'hms.token';
let token: string | null = sessionStorage.getItem(KEY);
let onExpired: (() => void) | null = null;

export function setToken(t: string | null) {
  token = t;
  if (t) sessionStorage.setItem(KEY, t); else sessionStorage.removeItem(KEY);
}
export function hasToken() { return token !== null; }
export function onSessionExpired(fn: () => void) { onExpired = fn; }

async function request<T>(method: string, path: string, body?: unknown): Promise<T> {
  let res: Response;
  try {
    res = await fetch(path, {
      method,
      headers: { 'Content-Type': 'application/json', ...(token ? { Authorization: `Bearer ${token}` } : {}) },
      body: body === undefined ? undefined : JSON.stringify(body),
    });
  } catch {
    throw new ApiError('E099', 'Cannot reach the HMS server. Is the HMS window still open?');
  }
  const text = await res.text();
  let data: unknown = null;
  try { data = text ? JSON.parse(text) : null; } catch { data = null; }
  if (!res.ok) {
    const e = (data as { error?: { code: string; message: string; details: string } } | null)?.error;
    const err = new ApiError(e?.code ?? 'E099', e?.message ?? `Unexpected error (${res.status}).`, e?.details ?? '', res.status);
    if (err.code === 'E003' && token) onExpired?.();
    throw err;
  }
  return data as T;
}

export const api = {
  get: <T>(path: string) => request<T>('GET', path),
  post: <T>(path: string, body: unknown = {}) => request<T>('POST', path, body),
  put: <T>(path: string, body: unknown = {}) => request<T>('PUT', path, body),
  // Downloads a file (e.g. a CSV report) using the login token.
  async download(path: string, fileName: string) {
    const res = await fetch(path, { headers: token ? { Authorization: `Bearer ${token}` } : {} });
    if (!res.ok) {
      const data = await res.json().catch(() => null);
      throw new ApiError(data?.error?.code ?? 'E099', data?.error?.message ?? 'Download failed.');
    }
    const url = URL.createObjectURL(await res.blob());
    const a = document.createElement('a');
    a.href = url; a.download = fileName; a.click();
    URL.revokeObjectURL(url);
  },
};

export const qs = (params: Record<string, string | number | undefined>) =>
  '?' + Object.entries(params).filter(([, v]) => v !== undefined && v !== '').map(([k, v]) => `${k}=${encodeURIComponent(String(v))}`).join('&');
