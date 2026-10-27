// Doctors: list with specialization filter; Administrator adds / edits doctors and sets weekly hours (HMS-F-020..022).
import { Badge, Button, Card, Checkbox, Group, Modal, Select, Stack, Table, Text, TextInput } from '@mantine/core';
import { IconPlus } from '@tabler/icons-react';
import { FormEvent, useEffect, useState } from 'react';
import { api, qs } from '../api';
import { useAuth } from '../auth';
import { EmptyState, ErrorBox, notifyError, notifyOk, PageHeader } from '../components/ui';
import type { Doctor, User, Window } from '../types';
import { hourMarks, weekdays } from '../util';

const short = (w: Window) => `${weekdays[w.weekday].slice(0, 3)} ${w.start}-${w.end}`;

function DoctorForm({ doctor, onSaved, onClose }: { doctor: Doctor | null; onSaved: () => void; onClose: () => void }) {
  const [f, setF] = useState({ fullName: doctor?.fullName ?? '', specialization: doctor?.specialization ?? '', department: doctor?.department ?? '', userId: doctor?.userId ?? '' });
  const [users, setUsers] = useState<User[]>([]);
  const [error, setError] = useState<unknown>(null);
  useEffect(() => { api.get<User[]>('/api/users').then((u) => setUsers(u.filter((x) => x.role === 'Doctor' && x.active))).catch(() => setUsers([])); }, []);
  const submit = async (e: FormEvent) => {
    e.preventDefault();
    try {
      if (doctor) await api.put(`/api/doctors/${doctor.doctorId}`, f); else await api.post('/api/doctors', f);
      onSaved();
    } catch (err) { setError(err); }
  };
  return (
    <form onSubmit={submit}>
      <Stack>
        <TextInput label="Full name" value={f.fullName} onChange={(e) => setF({ ...f, fullName: e.currentTarget.value })} required />
        <Group grow>
          <TextInput label="Specialization" placeholder="e.g. Cardiology" value={f.specialization} onChange={(e) => setF({ ...f, specialization: e.currentTarget.value })} required />
          <TextInput label="Department" value={f.department} onChange={(e) => setF({ ...f, department: e.currentTarget.value })} required />
        </Group>
        <Select label="Doctor login account" description="Create it first under User accounts (role Doctor)"
          data={users.map((u) => ({ value: u.userId, label: `${u.username} (${u.userId})` }))} value={f.userId} onChange={(v) => setF({ ...f, userId: v ?? '' })} required />
        <ErrorBox error={error} />
        <Group justify="flex-end"><Button variant="default" onClick={onClose}>Cancel</Button><Button type="submit">Save</Button></Group>
      </Stack>
    </form>
  );
}

// One working block per weekday, 08:00-20:00 in 30-minute steps (HMS-F-021).
function HoursForm({ doctor, onSaved, onClose }: { doctor: Doctor; onSaved: () => void; onClose: () => void }) {
  const initial = weekdays.map((_, d) => doctor.windows.find((w) => w.weekday === d) ?? null);
  const [rows, setRows] = useState(initial.map((w) => ({ on: !!w, start: w?.start ?? '09:00', end: w?.end ?? '13:00' })));
  const [error, setError] = useState<unknown>(null);
  const update = (i: number, patch: Partial<(typeof rows)[number]>) => setRows(rows.map((r, j) => (j === i ? { ...r, ...patch } : r)));
  const save = async () => {
    try {
      const windows = rows.flatMap((r, d) => (r.on ? [{ weekday: d, start: r.start, end: r.end }] : []));
      await api.put(`/api/doctors/${doctor.doctorId}/availability`, { windows });
      onSaved();
    } catch (err) { setError(err); }
  };
  return (
    <Stack>
      {rows.map((r, d) => (
        <Group key={d} grow>
          <Checkbox label={weekdays[d]} checked={r.on} onChange={(e) => update(d, { on: e.currentTarget.checked })} />
          <Select data={hourMarks} value={r.start} onChange={(v) => update(d, { start: v ?? r.start })} disabled={!r.on} aria-label="Start" />
          <Select data={hourMarks} value={r.end} onChange={(v) => update(d, { end: v ?? r.end })} disabled={!r.on} aria-label="End" />
        </Group>
      ))}
      <ErrorBox error={error} />
      <Group justify="flex-end"><Button variant="default" onClick={onClose}>Cancel</Button><Button onClick={save}>Save hours</Button></Group>
    </Stack>
  );
}

export default function DoctorsPage() {
  const { can } = useAuth();
  const [spec, setSpec] = useState('');
  const [doctors, setDoctors] = useState<Doctor[]>([]);
  const [editing, setEditing] = useState<Doctor | null | undefined>(undefined);   // undefined = closed, null = new
  const [hoursFor, setHoursFor] = useState<Doctor | null>(null);
  const load = () => api.get<Doctor[]>('/api/doctors' + qs({ specialization: spec })).then(setDoctors).catch(notifyError);
  useEffect(() => { load(); }, []);
  const admin = can('manageDoctors');
  const done = (msg: string) => { setEditing(undefined); setHoursFor(null); notifyOk(msg); load(); };

  return (
    <>
      <PageHeader title="Doctors" subtitle="Doctors, their specialization and weekly working hours"
        actions={admin && <Button leftSection={<IconPlus size={16} />} onClick={() => setEditing(null)}>Add doctor</Button>} />
      <Card withBorder mb="md">
        <form onSubmit={(e) => { e.preventDefault(); load(); }}>
          <Group align="flex-end">
            <TextInput style={{ flex: 1 }} label="Specialization" placeholder="All specializations" value={spec} onChange={(e) => setSpec(e.currentTarget.value)} />
            <Button type="submit">Filter</Button>
          </Group>
        </form>
      </Card>
      <Card withBorder>
        {doctors.length === 0 ? <EmptyState>No doctors found.</EmptyState> : (
          <Table striped highlightOnHover>
            <Table.Thead><Table.Tr><Table.Th>ID</Table.Th><Table.Th>Name</Table.Th><Table.Th>Specialization</Table.Th><Table.Th>Department</Table.Th><Table.Th>Working hours</Table.Th><Table.Th /></Table.Tr></Table.Thead>
            <Table.Tbody>
              {doctors.map((d) => (
                <Table.Tr key={d.doctorId}>
                  <Table.Td>{d.doctorId}</Table.Td><Table.Td>{d.fullName}</Table.Td><Table.Td>{d.specialization}</Table.Td><Table.Td>{d.department}</Table.Td>
                  <Table.Td>{d.windows.length === 0 ? <Text size="sm" c="dimmed">not set</Text> : <Group gap={4}>{d.windows.map((w) => <Badge key={w.weekday} variant="light">{short(w)}</Badge>)}</Group>}</Table.Td>
                  <Table.Td>{admin && <Group gap="xs" justify="flex-end"><Button size="xs" variant="light" onClick={() => setEditing(d)}>Edit</Button><Button size="xs" variant="light" onClick={() => setHoursFor(d)}>Hours</Button></Group>}</Table.Td>
                </Table.Tr>
              ))}
            </Table.Tbody>
          </Table>
        )}
      </Card>
      <Modal opened={editing !== undefined} onClose={() => setEditing(undefined)} title={editing ? `Edit ${editing.doctorId}` : 'Add doctor'} size="lg">
        {editing !== undefined && <DoctorForm doctor={editing} onSaved={() => done(editing ? 'Doctor updated.' : 'Doctor added. Set the working hours next.')} onClose={() => setEditing(undefined)} />}
      </Modal>
      <Modal opened={!!hoursFor} onClose={() => setHoursFor(null)} title={`Working hours — ${hoursFor?.fullName}`} size="lg">
        {hoursFor && <HoursForm doctor={hoursFor} onSaved={() => done('Working hours saved.')} onClose={() => setHoursFor(null)} />}
      </Modal>
    </>
  );
}
