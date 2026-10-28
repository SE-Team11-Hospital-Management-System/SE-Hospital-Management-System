// Patients: search (ID / part of name / phone, 20 per page), register with duplicate check, update, deactivate
// (HMS-F-010..015). Doctors can only search.
import { Button, Card, Group, Modal, SegmentedControl, Select, Stack, Table, Text, TextInput } from '@mantine/core';
import { useDisclosure } from '@mantine/hooks';
import { IconSearch, IconUserPlus } from '@tabler/icons-react';
import { FormEvent, useState } from 'react';
import { api, ApiError, qs } from '../api';
import { useAuth } from '../auth';
import { ActiveBadge, EmptyState, ErrorBox, notifyError, notifyOk, PageHeader } from '../components/ui';
import type { Patient } from '../types';
import { bloodGroups, genders } from '../util';

type Form = Omit<Patient, 'patientId' | 'active' | 'registeredOn'>;
const empty: Form = { fullName: '', dob: '', gender: '', phone: '', address: '', bloodGroup: '' };

function PatientForm({ initial, editing, onSaved, onClose }: { initial: Patient | null; editing: boolean; onSaved: (p: Patient) => void; onClose: () => void }) {
  const [f, setF] = useState<Form>(initial ? { ...initial } : empty);
  const [error, setError] = useState<unknown>(null);
  const [duplicateOf, setDuplicateOf] = useState('');
  const set = (k: keyof Form) => (v: string | null) => setF({ ...f, [k]: v ?? '' });

  const save = async (confirmDuplicate = false) => {
    setError(null);
    try {
      const p = editing && initial
        ? await api.put<Patient>(`/api/patients/${initial.patientId}`, { fullName: f.fullName, phone: f.phone, address: f.address, bloodGroup: f.bloodGroup })
        : await api.post<Patient>('/api/patients', { ...f, confirmDuplicate });
      onSaved(p);
    } catch (e) {
      if (e instanceof ApiError && e.code === 'E011') setDuplicateOf(e.details);    // HMS-F-015
      else setError(e);
    }
  };
  const submit = (e: FormEvent) => { e.preventDefault(); save(); };

  if (duplicateOf) return (
    <Stack>
      <Text>A patient with the same name, date of birth and phone already exists: <b>{duplicateOf}</b>.</Text>
      <Text size="sm" c="dimmed">Register a second record anyway?</Text>
      <Group justify="flex-end">
        <Button variant="default" onClick={onClose}>No, nothing is saved</Button>
        <Button onClick={() => { setDuplicateOf(''); save(true); }}>Yes, register anyway</Button>
      </Group>
    </Stack>
  );
  return (
    <form onSubmit={submit}>
      <Stack>
        <TextInput label="Full name" value={f.fullName} onChange={(e) => set('fullName')(e.currentTarget.value)} required />
        <Group grow>
          <TextInput label="Date of birth" type="date" value={f.dob} onChange={(e) => set('dob')(e.currentTarget.value)} disabled={editing} required />
          <Select label="Gender" data={genders} value={f.gender} onChange={set('gender')} disabled={editing} required />
        </Group>
        <Group grow>
          <TextInput label="Phone" description="10 digits" value={f.phone} onChange={(e) => set('phone')(e.currentTarget.value)} required />
          <Select label="Blood group" data={bloodGroups} value={f.bloodGroup} onChange={set('bloodGroup')} required />
        </Group>
        <TextInput label="Address" value={f.address} onChange={(e) => set('address')(e.currentTarget.value)} required />
        {editing && <Text size="xs" c="dimmed">Patient ID, date of birth and gender cannot be changed.</Text>}
        <ErrorBox error={error} />
        <Group justify="flex-end"><Button variant="default" onClick={onClose}>Cancel</Button><Button type="submit">Save</Button></Group>
      </Stack>
    </form>
  );
}

export default function PatientsPage() {
  const { can } = useAuth();
  const [by, setBy] = useState('name');
  const [term, setTerm] = useState('');
  const [page, setPage] = useState(1);
  const [result, setResult] = useState<{ items: Patient[]; hasNext: boolean } | null>(null);
  const [editing, setEditing] = useState<Patient | null>(null);
  const [opened, { open, close }] = useDisclosure(false);
  const [toDeactivate, setToDeactivate] = useState<Patient | null>(null);

  const search = async (p = 1) => {
    if (!term.trim()) return;
    try { setResult(await api.get('/api/patients' + qs({ by, term: term.trim(), page: p }))); setPage(p); } catch (e) { notifyError(e); }
  };
  const saved = (p: Patient) => {
    close();
    notifyOk(editing ? `Patient ${p.patientId} updated.` : `Patient registered with ID ${p.patientId}.`);
    setBy('id'); setTerm(p.patientId);
    setResult({ items: [p], hasNext: false });
  };
  const deactivate = async () => {
    if (!toDeactivate) return;
    try {
      const p = await api.post<Patient>(`/api/patients/${toDeactivate.patientId}/deactivate`);
      notifyOk(`Patient ${p.patientId} is now Inactive.`);
      setResult(result && { ...result, items: result.items.map((x) => (x.patientId === p.patientId ? p : x)) });
    } catch (e) { notifyError(e); }
    setToDeactivate(null);
  };

  return (
    <>
      <PageHeader title="Patients" subtitle="Search by patient ID, part of the name, or phone number"
        actions={can('registerPatient') && <Button leftSection={<IconUserPlus size={16} />} onClick={() => { setEditing(null); open(); }}>Register patient</Button>} />
      <Card withBorder mb="md">
        <form onSubmit={(e) => { e.preventDefault(); search(1); }}>
          <Group align="flex-end">
            <SegmentedControl value={by} onChange={setBy} data={[{ value: 'name', label: 'Name' }, { value: 'id', label: 'Patient ID' }, { value: 'phone', label: 'Phone' }]} />
            <TextInput style={{ flex: 1 }} placeholder={by === 'id' ? 'P000001' : by === 'phone' ? '9876543210' : 'e.g. anita'} value={term} onChange={(e) => setTerm(e.currentTarget.value)} />
            <Button type="submit" leftSection={<IconSearch size={16} />}>Search</Button>
          </Group>
        </form>
      </Card>
      {result && (
        <Card withBorder>
          {result.items.length === 0 ? <EmptyState>No patients found.</EmptyState> : (
            <Table striped highlightOnHover>
              <Table.Thead><Table.Tr><Table.Th>ID</Table.Th><Table.Th>Name</Table.Th><Table.Th>Date of birth</Table.Th><Table.Th>Gender</Table.Th><Table.Th>Phone</Table.Th><Table.Th>Blood</Table.Th><Table.Th>Status</Table.Th><Table.Th /></Table.Tr></Table.Thead>
              <Table.Tbody>
                {result.items.map((p) => (
                  <Table.Tr key={p.patientId}>
                    <Table.Td>{p.patientId}</Table.Td><Table.Td>{p.fullName}</Table.Td><Table.Td>{p.dob}</Table.Td><Table.Td>{p.gender}</Table.Td>
                    <Table.Td>{p.phone}</Table.Td><Table.Td>{p.bloodGroup}</Table.Td><Table.Td><ActiveBadge active={p.active} /></Table.Td>
                    <Table.Td>
                      <Group gap="xs" justify="flex-end">
                        {can('updatePatient') && <Button size="xs" variant="light" onClick={() => { setEditing(p); open(); }}>Edit</Button>}
                        {can('deactivatePatient') && p.active && <Button size="xs" variant="light" color="red" onClick={() => setToDeactivate(p)}>Deactivate</Button>}
                      </Group>
                    </Table.Td>
                  </Table.Tr>
                ))}
              </Table.Tbody>
            </Table>
          )}
          <Group justify="flex-end" mt="md">
            <Button variant="default" size="xs" disabled={page <= 1} onClick={() => search(page - 1)}>Previous</Button>
            <Text size="sm">Page {page}</Text>
            <Button variant="default" size="xs" disabled={!result.hasNext} onClick={() => search(page + 1)}>Next</Button>
          </Group>
        </Card>
      )}
      <Modal opened={opened} onClose={close} title={editing ? `Edit patient ${editing.patientId}` : 'Register patient'} size="lg">
        <PatientForm key={editing?.patientId ?? 'new'} initial={editing} editing={!!editing} onSaved={saved} onClose={close} />
      </Modal>
      <Modal opened={!!toDeactivate} onClose={() => setToDeactivate(null)} title="Deactivate patient">
        <Stack>
          <Text>Deactivate <b>{toDeactivate?.fullName}</b> ({toDeactivate?.patientId})? The record is kept, but marked Inactive and cannot get new appointments.</Text>
          <Group justify="flex-end"><Button variant="default" onClick={() => setToDeactivate(null)}>Cancel</Button><Button color="red" onClick={deactivate}>Deactivate</Button></Group>
        </Stack>
      </Modal>
    </>
  );
}
