// Administrator only, read-only: who did what and when, filtered by date range and username (HMS-F-062, SEC-003).
import { Badge, Button, Card, Group, Select, Table, Text, TextInput } from '@mantine/core';
import { useEffect, useState } from 'react';
import { api, qs } from '../api';
import { EmptyState, notifyError, PageHeader } from '../components/ui';
import type { AuditEntry, User } from '../types';
import { today } from '../util';

export default function AuditPage() {
  const [from, setFrom] = useState(today());
  const [to, setTo] = useState(today());
  const [username, setUsername] = useState<string | null>(null);
  const [users, setUsers] = useState<User[]>([]);
  const [rows, setRows] = useState<AuditEntry[] | null>(null);
  useEffect(() => { api.get<User[]>('/api/users').then(setUsers).catch(() => setUsers([])); }, []);
  const load = async () => { try { setRows(await api.get<AuditEntry[]>('/api/audit' + qs({ from, to, username: username ?? '' }))); } catch (e) { notifyError(e); } };
  useEffect(() => { load(); }, []);   // today's entries on first open
  const nameOf = (id: string) => users.find((u) => u.userId === id)?.username ?? id;
  return (
    <>
      <PageHeader title="Audit log" subtitle="Every login, change and refused action. It can be read, never edited, and never holds passwords or medical text." />
      <Card withBorder mb="md">
        <Group align="flex-end">
          <TextInput label="From" type="date" value={from} onChange={(e) => setFrom(e.currentTarget.value)} />
          <TextInput label="To" type="date" value={to} onChange={(e) => setTo(e.currentTarget.value)} />
          <Select label="User" placeholder="Everyone" clearable data={users.map((u) => u.username)} value={username} onChange={setUsername} />
          <Button onClick={load}>Show</Button>
        </Group>
      </Card>
      {rows && (
        <Card withBorder>
          <Text size="sm" c="dimmed" mb="xs">{rows.length} entries</Text>
          {rows.length === 0 ? <EmptyState>No entries.</EmptyState> : (
            <Table striped>
              <Table.Thead><Table.Tr><Table.Th>Time</Table.Th><Table.Th>User</Table.Th><Table.Th>Action</Table.Th><Table.Th>Target</Table.Th><Table.Th>Outcome</Table.Th></Table.Tr></Table.Thead>
              <Table.Tbody>
                {[...rows].reverse().map((e, i) => (
                  <Table.Tr key={i}>
                    <Table.Td>{e.timestamp.replace('T', ' ')}</Table.Td><Table.Td>{e.userId === '-' ? '-' : `${nameOf(e.userId)} (${e.userId})`}</Table.Td>
                    <Table.Td>{e.action}</Table.Td><Table.Td>{e.targetId}</Table.Td>
                    <Table.Td><Badge color={e.outcome === 'SUCCESS' ? 'teal' : 'red'} variant="light">{e.outcome}</Badge></Table.Td>
                  </Table.Tr>
                ))}
              </Table.Tbody>
            </Table>
          )}
        </Card>
      )}
    </>
  );
}
