// Doctors only: record a consultation (patient confirmed before saving), view a patient's history, add an addendum.
// Saved records are never edited or deleted (HMS-F-040..043, NF-010).
import { ActionIcon, Badge, Button, Card, Group, Modal, NumberInput, Stack, Table, Tabs, Text, Textarea, TextInput, Title } from '@mantine/core';
import { IconClipboardPlus, IconHistory, IconPlus, IconTrash } from '@tabler/icons-react';
import { useEffect, useState } from 'react';
import { api, qs } from '../api';
import { EmptyState, ErrorBox, notifyError, notifyOk, PageHeader, StatusBadge } from '../components/ui';
import type { Appointment, Consultation, PrescriptionItem } from '../types';
import { today } from '../util';

interface Summary { patientId: string; fullName: string; age: number }

function RecordForm({ appointment, onSaved, onClose }: { appointment: Appointment; onSaved: () => void; onClose: () => void }) {
  const [diagnosis, setDiagnosis] = useState('');
  const [notes, setNotes] = useState('');
  const [items, setItems] = useState<PrescriptionItem[]>([]);
  const [who, setWho] = useState<Summary | null>(null);
  const [error, setError] = useState<unknown>(null);
  const setItem = (i: number, patch: Partial<PrescriptionItem>) => setItems(items.map((x, j) => (j === i ? { ...x, ...patch } : x)));
  const review = async () => {                         // HMS-NF-010: show who the patient is before saving
    setError(null);
    try { setWho(await api.get<Summary>('/api/consultations/prepare' + qs({ appointmentId: appointment.appointmentId }))); } catch (e) { setError(e); }
  };
  const save = async () => {
    try {
      const r = await api.post<{ consultId: string }>('/api/consultations', { appointmentId: appointment.appointmentId, diagnosis, notes, items, confirmed: true });
      notifyOk(`Consultation ${r.consultId} saved. Appointment ${appointment.appointmentId} is now Completed.`);
      onSaved();
    } catch (e) { setWho(null); setError(e); }
  };
  if (who) return (
    <Stack>
      <Title order={4}>Is this the right patient?</Title>
      <Card withBorder><Text>Patient ID: <b>{who.patientId}</b></Text><Text>Name: <b>{who.fullName}</b></Text><Text>Age: <b>{who.age}</b></Text></Card>
      <Group justify="flex-end">
        <Button variant="default" onClick={() => { notifyOk('Discarded. Nothing was saved.'); onClose(); }}>No — discard</Button>
        <Button onClick={save}>Yes — save consultation</Button>
      </Group>
    </Stack>
  );
  return (
    <Stack>
      <Textarea label="Diagnosis" description={`${diagnosis.length}/500`} maxLength={500} autosize minRows={2} value={diagnosis} onChange={(e) => setDiagnosis(e.currentTarget.value)} required />
      <Text fw={500} size="sm">Prescription ({items.length}/10)</Text>
      {items.map((it, i) => (
        <Group key={i} align="flex-end" wrap="nowrap">
          <TextInput label={i === 0 ? 'Medicine' : undefined} style={{ flex: 2 }} value={it.medicine} onChange={(e) => setItem(i, { medicine: e.currentTarget.value })} />
          <TextInput label={i === 0 ? 'Dosage' : undefined} style={{ flex: 2 }} placeholder="5 mg twice a day" value={it.dosage} onChange={(e) => setItem(i, { dosage: e.currentTarget.value })} />
          <NumberInput label={i === 0 ? 'Days' : undefined} style={{ flex: 1 }} min={1} max={365} value={it.days} onChange={(v) => setItem(i, { days: Number(v) || 0 })} />
          <ActionIcon variant="subtle" color="red" onClick={() => setItems(items.filter((_, j) => j !== i))} aria-label="Remove"><IconTrash size={16} /></ActionIcon>
        </Group>
      ))}
      <Button variant="light" leftSection={<IconPlus size={16} />} disabled={items.length >= 10} onClick={() => setItems([...items, { medicine: '', dosage: '', days: 5 }])}>Add medicine</Button>
      <Textarea label="Notes" description={`${notes.length}/1000 (optional)`} maxLength={1000} autosize minRows={2} value={notes} onChange={(e) => setNotes(e.currentTarget.value)} />
      <ErrorBox error={error} />
      <Group justify="flex-end"><Button variant="default" onClick={onClose}>Cancel</Button><Button onClick={review} disabled={!diagnosis.trim()}>Continue</Button></Group>
    </Stack>
  );
}

function RecordTab() {
  const [date, setDate] = useState(today());
  const [list, setList] = useState<Appointment[]>([]);
  const [current, setCurrent] = useState<Appointment | null>(null);
  const load = () => api.get<{ appointments: Appointment[] }>('/api/schedule' + qs({ date })).then((r) => setList(r.appointments)).catch(notifyError);
  useEffect(() => { load(); }, [date]);   // reload when the date changes
  return (
    <Card withBorder>
      <Group mb="md"><TextInput label="Appointments on" type="date" value={date} onChange={(e) => setDate(e.currentTarget.value)} /></Group>
      {list.length === 0 ? <EmptyState>No appointments on {date}.</EmptyState> : (
        <Table striped>
          <Table.Thead><Table.Tr><Table.Th>Time</Table.Th><Table.Th>Appointment</Table.Th><Table.Th>Patient</Table.Th><Table.Th>Status</Table.Th><Table.Th /></Table.Tr></Table.Thead>
          <Table.Tbody>
            {list.map((a) => (
              <Table.Tr key={a.appointmentId}>
                <Table.Td>{a.slotStart.slice(11)}</Table.Td><Table.Td>{a.appointmentId}</Table.Td><Table.Td>{a.patientName ?? a.patientId}</Table.Td>
                <Table.Td><StatusBadge status={a.status} /></Table.Td>
                <Table.Td>{a.status === 'SCHEDULED' && <Button size="xs" onClick={() => setCurrent(a)}>Record</Button>}</Table.Td>
              </Table.Tr>
            ))}
          </Table.Tbody>
        </Table>
      )}
      <Modal opened={!!current} onClose={() => setCurrent(null)} title={`Consultation — ${current?.appointmentId}`} size="xl">
        {current && <RecordForm appointment={current} onSaved={() => { setCurrent(null); load(); }} onClose={() => setCurrent(null)} />}
      </Modal>
    </Card>
  );
}

function HistoryTab() {
  const [patientId, setPatientId] = useState('');
  const [list, setList] = useState<Consultation[] | null>(null);
  const [addTo, setAddTo] = useState<Consultation | null>(null);
  const [text, setText] = useState('');
  const [error, setError] = useState<unknown>(null);
  const load = async () => { try { setList(await api.get<Consultation[]>('/api/consultations' + qs({ patientId: patientId.trim() }))); } catch (e) { notifyError(e); } };
  const addAddendum = async () => {
    try { await api.post(`/api/consultations/${addTo?.consultId}/addenda`, { text }); notifyOk('Addendum added.'); setAddTo(null); setText(''); load(); } catch (e) { setError(e); }
  };
  return (
    <Stack>
      <Card withBorder>
        <form onSubmit={(e) => { e.preventDefault(); load(); }}>
          <Group align="flex-end"><TextInput style={{ flex: 1 }} label="Patient ID" placeholder="P000001" value={patientId} onChange={(e) => setPatientId(e.currentTarget.value)} /><Button type="submit">Show history</Button></Group>
        </form>
      </Card>
      {list && list.length === 0 && <EmptyState>No consultations yet.</EmptyState>}
      {list?.map((c) => (
        <Card withBorder key={c.consultId}>
          <Group justify="space-between" mb="xs">
            <Group><Badge>{c.consultId}</Badge><Text fw={600}>{c.createdAt}</Text><Text c="dimmed" size="sm">{c.doctorId} · {c.appointmentId}</Text></Group>
            <Button size="xs" variant="light" onClick={() => { setAddTo(c); setError(null); }}>Add addendum</Button>
          </Group>
          <Text><b>Diagnosis:</b> {c.diagnosis}</Text>
          {c.items.length > 0 && (
            <Table mt="xs" withTableBorder>
              <Table.Thead><Table.Tr><Table.Th>Medicine</Table.Th><Table.Th>Dosage</Table.Th><Table.Th>Days</Table.Th></Table.Tr></Table.Thead>
              <Table.Tbody>{c.items.map((i, k) => <Table.Tr key={k}><Table.Td>{i.medicine}</Table.Td><Table.Td>{i.dosage}</Table.Td><Table.Td>{i.days}</Table.Td></Table.Tr>)}</Table.Tbody>
            </Table>
          )}
          {c.notes && <Text mt="xs"><b>Notes:</b> {c.notes}</Text>}
          {c.addenda.map((a, k) => <Text key={k} mt="xs" size="sm" c="dimmed">Addendum {a.timestamp} by {a.authorUserId}: {a.text}</Text>)}
        </Card>
      ))}
      <Modal opened={!!addTo} onClose={() => setAddTo(null)} title={`Addendum to ${addTo?.consultId}`}>
        <Stack>
          <Text size="sm" c="dimmed">The original stays unchanged; the addendum is shown below it with the time and your user ID.</Text>
          <Textarea autosize minRows={3} maxLength={500} value={text} onChange={(e) => setText(e.currentTarget.value)} description={`${text.length}/500`} />
          <ErrorBox error={error} />
          <Group justify="flex-end"><Button variant="default" onClick={() => setAddTo(null)}>Cancel</Button><Button onClick={addAddendum} disabled={!text.trim()}>Add addendum</Button></Group>
        </Stack>
      </Modal>
    </Stack>
  );
}

export default function ConsultationsPage() {
  return (
    <>
      <PageHeader title="Consultations" subtitle="Visible to doctors only. Saved records cannot be changed — add an addendum instead." />
      <Tabs defaultValue="record">
        <Tabs.List mb="md">
          <Tabs.Tab value="record" leftSection={<IconClipboardPlus size={16} />}>Record</Tabs.Tab>
          <Tabs.Tab value="history" leftSection={<IconHistory size={16} />}>Patient history</Tabs.Tab>
        </Tabs.List>
        <Tabs.Panel value="record"><RecordTab /></Tabs.Panel>
        <Tabs.Panel value="history"><HistoryTab /></Tabs.Panel>
      </Tabs>
    </>
  );
}
