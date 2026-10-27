// Administrator only: list, create, deactivate and reactivate user accounts (HMS-F-004).
import { Badge, Button, Card, Group, Modal, PasswordInput, Select, Stack, Table, TextInput } from '@mantine/core';
import { IconUserPlus } from '@tabler/icons-react';
import { FormEvent, useEffect, useState } from 'react';
import { api } from '../api';
import { useAuth } from '../auth';
import { ActiveBadge, ErrorBox, notifyError, notifyOk, PageHeader } from '../components/ui';
import type { Role, User } from '../types';

function CreateForm({ onSaved, onClose }: { onSaved: (id: string) => void; onClose: () => void }) {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [role, setRole] = useState<Role>('Receptionist');
  const [error, setError] = useState<unknown>(null);
  const submit = async (e: FormEvent) => {
    e.preventDefault();
    try { const r = await api.post<{ userId: string }>('/api/users', { username, password, role }); onSaved(r.userId); } catch (err) { setError(err); }
  };
  return (
    <form onSubmit={submit}>
      <Stack>
        <TextInput label="Username" description="4-20 letters or digits" value={username} onChange={(e) => setUsername(e.currentTarget.value)} required />
        <PasswordInput label="Temporary password" description="8-32 characters, at least one letter and one digit" value={password} onChange={(e) => setPassword(e.currentTarget.value)} required />
        <Select label="Role" data={['Administrator', 'Receptionist', 'Doctor']} value={role} onChange={(v) => setRole((v as Role) ?? 'Receptionist')} />
        <ErrorBox error={error} />
        <Group justify="flex-end"><Button variant="default" onClick={onClose}>Cancel</Button><Button type="submit">Create user</Button></Group>
      </Stack>
    </form>
  );
}

export default function UsersPage() {
  const { user } = useAuth();
  const [users, setUsers] = useState<User[]>([]);
  const [creating, setCreating] = useState(false);
  const load = () => api.get<User[]>('/api/users').then(setUsers).catch(notifyError);
  useEffect(() => { load(); }, []);
  const setActive = async (u: User, active: boolean) => {
    try { await api.post(`/api/users/${u.userId}/status`, { active }); notifyOk(`${u.username} is now ${active ? 'active' : 'inactive'}.`); load(); } catch (e) { notifyError(e); }
  };
  return (
    <>
      <PageHeader title="User accounts" subtitle="Who can log in, and with which role"
        actions={<Button leftSection={<IconUserPlus size={16} />} onClick={() => setCreating(true)}>Create user</Button>} />
      <Card withBorder>
        <Table striped highlightOnHover>
          <Table.Thead><Table.Tr><Table.Th>User ID</Table.Th><Table.Th>Username</Table.Th><Table.Th>Role</Table.Th><Table.Th>Status</Table.Th><Table.Th /></Table.Tr></Table.Thead>
          <Table.Tbody>
            {users.map((u) => (
              <Table.Tr key={u.userId}>
                <Table.Td>{u.userId}</Table.Td><Table.Td>{u.username}</Table.Td><Table.Td><Badge variant="outline">{u.role}</Badge></Table.Td><Table.Td><ActiveBadge active={u.active} /></Table.Td>
                <Table.Td><Group justify="flex-end">{u.userId !== user?.userId && (u.active
                  ? <Button size="xs" variant="light" color="red" onClick={() => setActive(u, false)}>Deactivate</Button>
                  : <Button size="xs" variant="light" onClick={() => setActive(u, true)}>Reactivate</Button>)}</Group></Table.Td>
              </Table.Tr>
            ))}
          </Table.Tbody>
        </Table>
      </Card>
      <Modal opened={creating} onClose={() => setCreating(false)} title="Create user">
        {creating && <CreateForm onSaved={(id) => { setCreating(false); notifyOk(`User created with ID ${id}.`); load(); }} onClose={() => setCreating(false)} />}
      </Modal>
    </>
  );
}
