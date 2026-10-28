// Shown instead of the login when a data file failed its checksum at start-up (HMS-SEC-007).
// An Administrator of the latest backup logs in to restore it.
import { Alert, Button, PasswordInput, Stack, Text, TextInput } from '@mantine/core';
import { FormEvent, useState } from 'react';
import { api } from '../api';
import { ErrorBox, notifyOk } from '../components/ui';

export function RecoveryPanel({ file, latestBackup, onDone }: { file: string; latestBackup: string; onDone: () => void }) {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState<unknown>(null);
  const [busy, setBusy] = useState(false);
  const submit = async (e: FormEvent) => {
    e.preventDefault();
    setBusy(true); setError(null);
    try {
      const r = await api.post<{ restored: string }>('/api/recovery/restore', { username, password });
      notifyOk(`Backup ${r.restored} restored. Please log in.`);
      onDone();
    } catch (err) { setError(err); } finally { setBusy(false); }
  };
  return (
    <form onSubmit={submit}>
      <Stack>
        <Alert color="red" title="E040: Data file damaged">
          The file <b>{file || 'a data file'}</b> was changed outside HMS or is damaged, so HMS did not load it.
        </Alert>
        {latestBackup
          ? <Text size="sm">Restore the latest backup <b>{latestBackup}</b>? An Administrator must log in to confirm.</Text>
          : <Text size="sm" c="red">No backup is available. HMS cannot start with damaged data.</Text>}
        <TextInput label="Administrator username" value={username} onChange={(e) => setUsername(e.currentTarget.value)} required />
        <PasswordInput label="Password" value={password} onChange={(e) => setPassword(e.currentTarget.value)} required />
        {error !== null && <ErrorBox error={error} />}
        <Button type="submit" color="red" loading={busy} disabled={!latestBackup}>Restore latest backup</Button>
      </Stack>
    </form>
  );
}
