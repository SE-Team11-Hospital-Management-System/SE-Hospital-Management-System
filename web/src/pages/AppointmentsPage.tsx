// Appointments: book (refused slots offer the next 3 free ones), then per patient: cancel, reschedule, no-show
// (HMS-F-030..033, F-035).
import { Button, Card, Group, Modal, Select, Stack, Table, Tabs, Text, TextInput } from '@mantine/core';
import { IconCalendarPlus, IconListCheck } from '@tabler/icons-react';
import { FormEvent, useEffect, useState } from 'react';
import { api, ApiError, qs } from '../api';
import { EmptyState, ErrorBox, notifyError, notifyOk, PageHeader, StatusBadge } from '../components/ui';
import type { Appointment, Doctor, Patient } from '../types';
import { alternatives, slotTimes, today } from '../util';

function useDoctors() {
  const [doctors, setDoctors] = useState<Doctor[]>([]);
  useEffect(() => { api.get<Doctor[]>('/api/doctors').then(setDoctors).catch(notifyError); }, []);
  return doctors.map((d) => ({ value: d.doctorId, label: `${d.fullName} — ${d.specialization} (${d.doctorId})` }));
}

// Slot picker shared by booking and rescheduling. Shows the reason and the free alternatives when refused.
function SlotForm({ doctorId: initialDoctor, submitLabel, onSubmit }: { doctorId?: string; submitLabel: string; onSubmit: (doctorId: string, slot: string) => Promise<void> }) {
  const doctors = useDoctors();
  const [doctorId, setDoctorId] = useState(initialDoctor ?? '');
  const [date, setDate] = useState(today());
  const [time, setTime] = useState<string | null>(null);
  const [error, setError] = useState<unknown>(null);
  const [alts, setAlts] = useState<string[]>([]);
  const send = async (slot: string) => {
    setError(null); setAlts([]);
    try { await onSubmit(doctorId, slot); } catch (e) {
      setError(e);
      if (e instanceof ApiError && (e.code === 'E030' || e.code === 'E031')) setAlts(alternatives(e.details));
    }
  };
  const submit = (e: FormEvent) => { e.preventDefault(); if (time) send(`${date} ${time}`); };
  return (
    <form onSubmit={submit}>
      <Stack>
        <Select label="Doctor" data={doctors} value={doctorId} onChange={(v) => setDoctorId(v ?? '')} searchable required />
        <Group grow>
          <TextInput label="Date" type="date" value={date} onChange={(e) => setDate(e.currentTarget.value)} required />
          <Select label="Time" data={slotTimes} value={time} onChange={setTime} placeholder="Pick a time" required />
        </Group>
        <ErrorBox error={error} />
        {alts.length > 0 && (
          <Group>
            <Text size="sm">Next free slots:</Text>
            {alts.map((a) => <Button key={a} size="xs" variant="light" onClick={() => send(a)}>{a}</Button>)}
          </Group>
        )}
        <Button type="submit" disabled={!doctorId || !time}>{submitLabel}</Button>
      </Stack>
    </form>
  );
}

function BookTab() {
  const [patientId, setPatientId] = useState('');
  const [patient, setPatient] = useState<Patient | null>(null);
  const [lookupError, setLookupError] = useState<unknown>(null);
  const find = async () => {
    setLookupError(null); setPatient(null);
    try { setPatient(await api.get<Patient>(`/api/patients/${patientId.trim()}`)); } catch (e) { setLookupError(e); }
  };
  const book = async (doctorId: string, slot: string) => {
    const a = await api.post<Appointment>('/api/appointments', { patientId: patient?.patientId, doctorId, slot });
    notifyOk(`Appointment ${a.appointmentId} booked for ${patient?.fullName} at ${a.slotStart}.`);
  };
  return (
    <Card withBorder maw={640}>
      <Stack>
        <Group align="flex-end">
          <TextInput style={{ flex: 1 }} label="Patient ID" placeholder="P000001" value={patientId} onChange={(e) => setPatientId(e.currentTarget.value)} />
          <Button variant="light" onClick={find}>Find</Button>
        </Group>
        <ErrorBox error={lookupError} />
        {patient && <Text>Patient: <b>{patient.fullName}</b>, {patient.gender}, born {patient.dob}{!patient.active && ' — Inactive'}</Text>}
        {patient && <SlotForm submitLabel="Book appointment" onSubmit={book} />}
      </Stack>
    </Card>
  );
}

function ManageTab() {
  const [patientId, setPatientId] = useState('');
  const [list, setList] = useState<Appointment[] | null>(null);
  const [moving, setMoving] = useState<Appointment | null>(null);
  const [confirm, setConfirm] = useState<{ a: Appointment; action: 'cancel' | 'no-show' } | null>(null);
  const load = async () => { try { setList(await api.get<Appointment[]>('/api/appointments' + qs({ patientId: patientId.trim() }))); } catch (e) { notifyError(e); } };
  const act = async () => {
    if (!confirm) return;
    try { await api.post(`/api/appointments/${confirm.a.appointmentId}/${confirm.action}`); notifyOk(`Appointment ${confirm.a.appointmentId} updated.`); load(); } catch (e) { notifyError(e); }
    setConfirm(null);
  };
  const reschedule = async (doctorId: string, slot: string) => {
    await api.post(`/api/appointments/${moving?.appointmentId}/reschedule`, { doctorId, slot });
    notifyOk(`Appointment ${moving?.appointmentId} moved to ${slot}.`);
    setMoving(null); load();
  };
  return (
    <Card withBorder>
      <form onSubmit={(e) => { e.preventDefault(); load(); }}>
        <Group align="flex-end" mb="md">
          <TextInput style={{ flex: 1 }} label="Patient ID" placeholder="P000001" value={patientId} onChange={(e) => setPatientId(e.currentTarget.value)} />
          <Button type="submit">Show appointments</Button>
        </Group>
      </form>
      {list && (list.length === 0 ? <EmptyState>No appointments.</EmptyState> : (
        <Table striped>
          <Table.Thead><Table.Tr><Table.Th>ID</Table.Th><Table.Th>When</Table.Th><Table.Th>Doctor</Table.Th><Table.Th>Status</Table.Th><Table.Th /></Table.Tr></Table.Thead>
          <Table.Tbody>
            {list.map((a) => (
              <Table.Tr key={a.appointmentId}>
                <Table.Td>{a.appointmentId}</Table.Td><Table.Td>{a.slotStart}</Table.Td><Table.Td>{a.doctorId}</Table.Td><Table.Td><StatusBadge status={a.status} /></Table.Td>
                <Table.Td>{a.status === 'SCHEDULED' && (
                  <Group gap="xs" justify="flex-end">
                    <Button size="xs" variant="light" onClick={() => setMoving(a)}>Reschedule</Button>
                    <Button size="xs" variant="light" color="red" onClick={() => setConfirm({ a, action: 'cancel' })}>Cancel</Button>
                    <Button size="xs" variant="light" color="orange" onClick={() => setConfirm({ a, action: 'no-show' })}>No-show</Button>
                  </Group>
                )}</Table.Td>
              </Table.Tr>
            ))}
          </Table.Tbody>
        </Table>
      ))}
      <Modal opened={!!moving} onClose={() => setMoving(null)} title={`Reschedule ${moving?.appointmentId} (now ${moving?.slotStart})`}>
        {moving && <SlotForm doctorId={moving.doctorId} submitLabel="Move appointment" onSubmit={reschedule} />}
      </Modal>
      <Modal opened={!!confirm} onClose={() => setConfirm(null)} title={confirm?.action === 'cancel' ? 'Cancel appointment' : 'Mark as no-show'}>
        <Stack>
          <Text>{confirm?.action === 'cancel' ? 'Cancel' : 'Mark as no-show'} appointment <b>{confirm?.a.appointmentId}</b> at {confirm?.a.slotStart}?</Text>
          <Group justify="flex-end"><Button variant="default" onClick={() => setConfirm(null)}>Back</Button><Button color="red" onClick={act}>Yes</Button></Group>
        </Stack>
      </Modal>
    </Card>
  );
}

export default function AppointmentsPage() {
  return (
    <>
      <PageHeader title="Appointments" subtitle="Book in 30-minute slots; double booking is impossible" />
      <Tabs defaultValue="book">
        <Tabs.List mb="md">
          <Tabs.Tab value="book" leftSection={<IconCalendarPlus size={16} />}>Book</Tabs.Tab>
          <Tabs.Tab value="manage" leftSection={<IconListCheck size={16} />}>Cancel / reschedule / no-show</Tabs.Tab>
        </Tabs.List>
        <Tabs.Panel value="book"><BookTab /></Tabs.Panel>
        <Tabs.Panel value="manage"><ManageTab /></Tabs.Panel>
      </Tabs>
    </>
  );
}
