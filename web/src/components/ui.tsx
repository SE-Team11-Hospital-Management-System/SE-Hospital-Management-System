// Shared building blocks so every page looks and behaves the same (HMS-NF-006).
import { ReactNode } from 'react';
import { Alert, Badge, Card, Group, Stack, Text, Title } from '@mantine/core';
import { notifications } from '@mantine/notifications';
import { IconAlertCircle, IconCheck } from '@tabler/icons-react';
import { ApiError } from '../api';
import type { AppointmentStatus } from '../types';

export function PageHeader({ title, subtitle, actions }: { title: string; subtitle?: string; actions?: ReactNode }) {
  return (
    <Group justify="space-between" align="flex-end" mb="lg">
      <Stack gap={2}>
        <Title order={2}>{title}</Title>
        {subtitle && <Text c="dimmed" size="sm">{subtitle}</Text>}
      </Stack>
      {actions && <Group>{actions}</Group>}
    </Group>
  );
}

export const errorText = (e: unknown) => (e instanceof ApiError ? e.text : 'E099: Unexpected error.');

// Error shown inside a form, with the HMS code and the message naming the field and the rule.
export function ErrorBox({ error }: { error: unknown }) {
  if (!error) return null;
  return <Alert color="red" icon={<IconAlertCircle size={18} />} title="Not saved">{errorText(error)}</Alert>;
}

export const notifyOk = (message: string) => notifications.show({ color: 'teal', icon: <IconCheck size={18} />, title: 'Done', message });
export const notifyError = (e: unknown) => notifications.show({ color: 'red', title: 'Error', message: errorText(e), autoClose: 8000 });

const statusColor: Record<AppointmentStatus, string> = { SCHEDULED: 'blue', COMPLETED: 'teal', CANCELLED: 'gray', NO_SHOW: 'orange' };
const statusLabel: Record<AppointmentStatus, string> = { SCHEDULED: 'Scheduled', COMPLETED: 'Completed', CANCELLED: 'Cancelled', NO_SHOW: 'No-show' };
export function StatusBadge({ status }: { status: AppointmentStatus }) {
  return <Badge color={statusColor[status]} variant="light">{statusLabel[status]}</Badge>;
}

export function ActiveBadge({ active }: { active: boolean }) {
  return <Badge color={active ? 'teal' : 'gray'} variant="light">{active ? 'Active' : 'Inactive'}</Badge>;
}

export function EmptyState({ children }: { children: ReactNode }) {
  return <Text c="dimmed" ta="center" py="xl">{children}</Text>;
}

// Placeholder for screens that a teammate's pull request adds later this week.
export function ComingSoon({ title, owner }: { title: string; owner: string }) {
  return (
    <Card withBorder padding="xl" radius="md">
      <Title order={3}>{title}</Title>
      <Text c="dimmed" mt="sm">This screen arrives with {owner}'s pull request this week.</Text>
    </Card>
  );
}
