// Daily appointment report with CSV download; doctor workload for the Administrator (HMS-F-050, F-051).
import { Button, Card, Group, SimpleGrid, Stack, Table, Tabs, Text, TextInput } from '@mantine/core';
import { IconDownload } from '@tabler/icons-react';
import { useState } from 'react';
import { api, qs } from '../api';
import { useAuth } from '../auth';
import { EmptyState, notifyError, notifyOk, PageHeader, StatusBadge } from '../components/ui';
import type { AppointmentStatus, DailyReport, WorkloadRow } from '../types';
import { today } from '../util';

const statuses: AppointmentStatus[] = ['SCHEDULED', 'COMPLETED', 'CANCELLED', 'NO_SHOW'];

function DailyTab() {
  const [date, setDate] = useState(today());
  const [report, setReport] = useState<DailyReport | null>(null);
  const load = async () => { try { setReport(await api.get<DailyReport>('/api/reports/daily' + qs({ date }))); } catch (e) { notifyError(e); } };
  const csv = async () => {
    try { await api.download('/api/reports/daily/csv' + qs({ date }), `daily_${date}.csv`); notifyOk(`daily_${date}.csv downloaded (a copy is also in the reports folder).`); } catch (e) { notifyError(e); }
  };
  return (
    <Stack>
      <Card withBorder>
        <Group align="flex-end">
          <TextInput label="Date" type="date" value={date} onChange={(e) => setDate(e.currentTarget.value)} />
          <Button onClick={load}>Show report</Button>
          {report && <Button variant="light" leftSection={<IconDownload size={16} />} onClick={csv}>Download CSV</Button>}
        </Group>
      </Card>
      {report && (
        <>
          <SimpleGrid cols={{ base: 2, md: 5 }}>
            <Card withBorder><Text c="dimmed" size="sm">Total</Text><Text fw={700} size="xl">{report.total}</Text></Card>
            {statuses.map((s) => <Card withBorder key={s}><StatusBadge status={s} /><Text fw={700} size="xl" mt={4}>{report.perStatus[s] ?? 0}</Text></Card>)}
          </SimpleGrid>
          {report.perDoctor.length === 0 && <EmptyState>No appointments on {report.date}.</EmptyState>}
          {report.perDoctor.map((d) => (
            <Card withBorder key={d.doctorId}>
              <Text fw={600} mb="xs">{d.doctorName} ({d.doctorId}) — {d.appointments.length} appointments</Text>
              <Table striped>
                <Table.Tbody>
                  {d.appointments.map((a) => (
                    <Table.Tr key={a.appointmentId}><Table.Td>{a.slotStart.slice(11)}</Table.Td><Table.Td>{a.appointmentId}</Table.Td><Table.Td>{a.patientName} ({a.patientId})</Table.Td><Table.Td><StatusBadge status={a.status} /></Table.Td></Table.Tr>
                  ))}
                </Table.Tbody>
              </Table>
            </Card>
          ))}
        </>
      )}
    </Stack>
  );
}

function WorkloadTab() {
  const [from, setFrom] = useState(today());
  const [to, setTo] = useState(today());
  const [rows, setRows] = useState<WorkloadRow[] | null>(null);
  const load = async () => { try { setRows(await api.get<WorkloadRow[]>('/api/reports/workload' + qs({ from, to }))); } catch (e) { notifyError(e); } };
  return (
    <Card withBorder>
      <Group align="flex-end" mb="md">
        <TextInput label="From" type="date" value={from} onChange={(e) => setFrom(e.currentTarget.value)} />
        <TextInput label="To" type="date" value={to} onChange={(e) => setTo(e.currentTarget.value)} />
        <Button onClick={load}>Show workload</Button>
      </Group>
      {rows && (rows.length === 0 ? <EmptyState>No doctors.</EmptyState> : (
        <Table striped>
          <Table.Thead><Table.Tr><Table.Th>Doctor</Table.Th><Table.Th>Completed appointments</Table.Th></Table.Tr></Table.Thead>
          <Table.Tbody>{rows.map((r) => <Table.Tr key={r.doctorId}><Table.Td>{r.doctorName} ({r.doctorId})</Table.Td><Table.Td>{r.completed}</Table.Td></Table.Tr>)}</Table.Tbody>
        </Table>
      ))}
    </Card>
  );
}

export default function ReportsPage() {
  const { can } = useAuth();
  return (
    <>
      <PageHeader title="Reports" subtitle="Reports never contain medical information" />
      <Tabs defaultValue="daily">
        <Tabs.List mb="md">
          <Tabs.Tab value="daily">Daily appointments</Tabs.Tab>
          {can('workloadReport') && <Tabs.Tab value="workload">Doctor workload</Tabs.Tab>}
        </Tabs.List>
        <Tabs.Panel value="daily"><DailyTab /></Tabs.Panel>
        {can('workloadReport') && <Tabs.Panel value="workload"><WorkloadTab /></Tabs.Panel>}
      </Tabs>
    </>
  );
}
