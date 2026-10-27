// Login (HMS-F-001, SEC-004, SEC-008), first-run Administrator set-up (SRS 2.6) and the damaged-data screen (HMS-SEC-007).
import { Alert, Button, Card, Center, Loader, PasswordInput, Stack, Text, TextInput, ThemeIcon, Title } from '@mantine/core';
import { IconStethoscope } from '@tabler/icons-react';
import { FormEvent, useEffect, useState } from 'react';
import { api } from '../api';
import { useAuth } from '../auth';
import { ErrorBox, notifyOk } from '../components/ui';
import type { Status } from '../types';
import { RecoveryPanel } from './RecoveryPage';

function Brand({ subtitle }: { subtitle: string }) {
  return (
    <Stack align="center" gap={4} mb="md">
      <ThemeIcon size={56} radius="xl" variant="gradient" gradient={{ from: 'teal', to: 'cyan' }}>
        <IconStethoscope size={30} />
      </ThemeIcon>
      <Title order={2}>HMS</Title>
      <Text c="dimmed" size="sm">{subtitle}</Text>
    </Stack>
  );
}

function SetupForm({ onDone }: { onDone: () => void }) {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [again, setAgain] = useState('');
  const [error, setError] = useState<unknown>(null);
  const submit = async (e: FormEvent) => {
    e.preventDefault();
    setError(null);
    if (password !== again) { setError(new Error('mismatch')); return; }
    try {
      await api.post('/api/setup', { username, password });
      notifyOk('Administrator created. Please log in.');
      onDone();
    } catch (err) { setError(err); }
  };
  return (
    <form onSubmit={submit}>
      <Stack>
        <Alert color="teal" title="First run">No accounts exist yet. Create the first Administrator account.</Alert>
        <TextInput label="Administrator username" description="4-20 letters or digits" value={username} onChange={(e) => setUsername(e.currentTarget.value)} required />
        <PasswordInput label="Password" description="8-32 characters, at least one letter and one digit" value={password} onChange={(e) => setPassword(e.currentTarget.value)} required />
        <PasswordInput label="Repeat password" value={again} onChange={(e) => setAgain(e.currentTarget.value)} required
          error={again && again !== password ? 'E010: Password: the two entries do not match.' : undefined} />
        {error instanceof Error && error.message !== 'mismatch' && <ErrorBox error={error} />}
        <Button type="submit">Create Administrator</Button>
      </Stack>
    </form>
  );
}

function LoginForm() {
  const { login } = useAuth();
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState<unknown>(null);
  const [busy, setBusy] = useState(false);
  const submit = async (e: FormEvent) => {
    e.preventDefault();
    setBusy(true); setError(null);
    try { await login(username, password); } catch (err) { setError(err); setPassword(''); } finally { setBusy(false); }
  };
  return (
    <form onSubmit={submit}>
      <Stack>
        <TextInput label="Username" value={username} onChange={(e) => setUsername(e.currentTarget.value)} autoFocus required />
        <PasswordInput label="Password" value={password} onChange={(e) => setPassword(e.currentTarget.value)} required />
        {error !== null && <ErrorBox error={error} />}
        <Button type="submit" loading={busy}>Log in</Button>
      </Stack>
    </form>
  );
}

export default function LoginPage() {
  const [status, setStatus] = useState<Status | null>(null);
  const [failed, setFailed] = useState(false);
  const load = () => api.get<Status>('/api/status').then(setStatus).catch(() => setFailed(true));
  useEffect(() => { load(); }, []);

  let body;
  if (failed) body = <Alert color="red">Cannot reach the HMS server. Start it with  python sprint.py run.</Alert>;
  else if (!status) body = <Center><Loader /></Center>;
  else if (status.recovery.needed) body = <RecoveryPanel file={status.recovery.file ?? ''} latestBackup={status.recovery.latestBackup ?? ''} onDone={load} />;
  else if (!status.hasUsers) body = <SetupForm onDone={load} />;
  else body = <LoginForm />;

  return (
    <Center mih="100vh" bg="var(--mantine-color-gray-light)">
      <Card withBorder shadow="md" radius="lg" padding="xl" w={420}>
        <Brand subtitle="Hospital Management System — Team 11" />
        {body}
      </Card>
    </Center>
  );
}
