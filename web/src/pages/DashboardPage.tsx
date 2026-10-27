// Start page: a greeting, today's numbers and shortcuts for this role.
import { Card, Group, SimpleGrid, Stack, Table, Text, ThemeIcon, UnstyledButton } from '@mantine/core';
import { useEffect, useState } from 'react';
import { Link } from 'react-router-dom';
import { api, qs } from '../api';
import { useAuth } from '../auth';
import { navItems } from '../components/Layout';
import { EmptyState, PageHeader, StatusBadge } from '../components/ui';
import type { Appointment, DailyReport } from '../types';
import { today } from '../util';

function TodayNumbers() {
  const [report, setReport] = useState<DailyReport | null>(null);
  useEffect(() => { api.get<DailyReport>('/api/reports/daily' + qs({ date: today() })).then(setReport).catch(() => setReport(null)); }, []);
  if (!report) return null;
  const tiles = [['Appointments today', report.total], ['Scheduled', report.perStatus.SCHEDULED], ['Completed', report.perStatus.COMPLETED], ['No-show', report.perStatus.NO_SHOW]];
  return (
    <SimpleGrid cols={{ base: 2, md: 4 }} mb="lg">
      {tiles.map(([label, n]) => (
        <Card withBorder key={label}><Text c="dimmed" size="sm">{label}</Text><Text fw={700} size="xl">{n}</Text></Card>
      ))}
    </SimpleGrid>
  );
}

function MyDay() {
  const [list, setList] = useState<Appointment[] | null>(null);
  useEffect(() => { api.get<{ appointments: Appointment[] }>('/api/schedule' + qs({ date: today() })).then((r) => setList(r.appointments)).catch(() => setList(null)); }, []);
  if (list === null) return null;
  return (
    <Card withBorder mb="lg">
      <Text fw={600} mb="sm">My appointments today</Text>
      {list.length === 0 ? <EmptyState>No appointments today.</EmptyState> : (
        <Table>
          <Table.Tbody>
            {list.map((a) => (
              <Table.Tr key={a.appointmentId}>
                <Table.Td>{a.slotStart.slice(11)}</Table.Td><Table.Td>{a.patientName ?? a.patientId}</Table.Td><Table.Td><StatusBadge status={a.status} /></Table.Td>
              </Table.Tr>
            ))}
          </Table.Tbody>
        </Table>
      )}
    </Card>
  );
}

export default function DashboardPage() {
  const { user, can } = useAuth();
  const shortcuts = navItems.filter((i) => i.path !== '/' && i.permission && can(i.permission));
  return (
    <Stack gap={0}>
      <PageHeader title={`Welcome, ${user?.username}`} subtitle={`${user?.role} · ${today()}`} />
      {user?.role === 'Doctor' ? <MyDay /> : can('dailyReport') && <TodayNumbers />}
      <SimpleGrid cols={{ base: 1, sm: 2, lg: 3 }}>
        {shortcuts.map((i) => (
          <UnstyledButton key={i.path} component={Link} to={i.path}>
            <Card withBorder padding="lg">
              <Group><ThemeIcon variant="light" size="lg">{i.icon}</ThemeIcon><Text fw={600}>{i.label}</Text></Group>
            </Card>
          </UnstyledButton>
        ))}
      </SimpleGrid>
    </Stack>
  );
}
