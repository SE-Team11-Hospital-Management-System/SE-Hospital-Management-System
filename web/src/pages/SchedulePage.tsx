// One doctor's appointments for one date, in time order, all statuses (HMS-F-034). Doctors see only their own.
import { Button, Card, Group, Select, Table, TextInput } from '@mantine/core';
import { useEffect, useState } from 'react';
import { api, qs } from '../api';
import { useAuth } from '../auth';
import { EmptyState, notifyError, PageHeader, StatusBadge } from '../components/ui';
import type { Appointment, Doctor } from '../types';
import { today } from '../util';

export default function SchedulePage() {
  const { user } = useAuth();
  const isDoctor = user?.role === 'Doctor';
  const [doctors, setDoctors] = useState<Doctor[]>([]);
  const [doctorId, setDoctorId] = useState(isDoctor ? user?.doctorId ?? '' : '');
  const [date, setDate] = useState(today());
  const [list, setList] = useState<Appointment[] | null>(null);
  useEffect(() => { if (!isDoctor) api.get<Doctor[]>('/api/doctors').then(setDoctors).catch(notifyError); }, [isDoctor]);
  const load = async () => {
    try { setList((await api.get<{ appointments: Appointment[] }>('/api/schedule' + qs({ doctorId, date }))).appointments); } catch (e) { notifyError(e); }
  };
  useEffect(() => { if (isDoctor) load(); }, []);
  return (
    <>
      <PageHeader title={isDoctor ? 'My schedule' : 'Doctor schedules'} subtitle="Appointments for one day, in time order" />
      <Card withBorder mb="md">
        <form onSubmit={(e) => { e.preventDefault(); load(); }}>
          <Group align="flex-end">
            {!isDoctor && <Select style={{ flex: 1 }} label="Doctor" searchable data={doctors.map((d) => ({ value: d.doctorId, label: `${d.fullName} (${d.doctorId})` }))} value={doctorId} onChange={(v) => setDoctorId(v ?? '')} />}
            <TextInput label="Date" type="date" value={date} onChange={(e) => setDate(e.currentTarget.value)} />
            <Button type="submit" disabled={!doctorId}>Show</Button>
          </Group>
        </form>
      </Card>
      {list && (
        <Card withBorder>
          {list.length === 0 ? <EmptyState>No appointments on {date}.</EmptyState> : (
            <Table striped>
              <Table.Thead><Table.Tr><Table.Th>Time</Table.Th><Table.Th>Appointment</Table.Th><Table.Th>Patient</Table.Th><Table.Th>Status</Table.Th></Table.Tr></Table.Thead>
              <Table.Tbody>
                {list.map((a) => (
                  <Table.Tr key={a.appointmentId}>
                    <Table.Td>{a.slotStart.slice(11)}</Table.Td><Table.Td>{a.appointmentId}</Table.Td>
                    <Table.Td>{a.patientName ? `${a.patientName} (${a.patientId})` : a.patientId}</Table.Td><Table.Td><StatusBadge status={a.status} /></Table.Td>
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
