// Every role: change your own password (HMS-F-006). The server checks the rules and names the broken one.
import { Button, Card, PasswordInput, Stack } from '@mantine/core';
import { FormEvent, useState } from 'react';
import { api } from '../api';
import { ErrorBox, notifyOk, PageHeader } from '../components/ui';

export default function PasswordPage() {
  const [current, setCurrent] = useState('');
  const [next, setNext] = useState('');
  const [again, setAgain] = useState('');
  const [error, setError] = useState<unknown>(null);
  const submit = async (e: FormEvent) => {
    e.preventDefault();
    setError(null);
    try {
      await api.post('/api/password', { current, new: next });
      notifyOk('Password changed. Use the new password next time you log in.');
      setCurrent(''); setNext(''); setAgain('');
    } catch (err) { setError(err); }
  };
  return (
    <>
      <PageHeader title="Change my password" />
      <Card withBorder maw={480}>
        <form onSubmit={submit}>
          <Stack>
            <PasswordInput label="Current password" value={current} onChange={(e) => setCurrent(e.currentTarget.value)} required />
            <PasswordInput label="New password" description="8-32 characters, at least one letter and one digit, different from the current one" value={next} onChange={(e) => setNext(e.currentTarget.value)} required />
            <PasswordInput label="Repeat new password" value={again} onChange={(e) => setAgain(e.currentTarget.value)} required
              error={again && again !== next ? 'E010: New password: the two entries do not match.' : undefined} />
            <ErrorBox error={error} />
            <Button type="submit" disabled={!next || next !== again}>Change password</Button>
          </Stack>
        </form>
      </Card>
    </>
  );
}
