// The frame around every page: header with the user, and a sidebar with only the pages this role may use (HMS-F-003).
import { AppShell, Badge, Burger, Button, Group, NavLink, ScrollArea, Text, ThemeIcon, useMantineColorScheme, ActionIcon } from '@mantine/core';
import { useDisclosure } from '@mantine/hooks';
import {
  IconCalendarEvent, IconCalendarTime, IconChartBar, IconClipboardHeart, IconDatabaseExport, IconHome, IconKey,
  IconListDetails, IconLogout, IconMoon, IconStethoscope, IconSun, IconUserCog, IconUsers,
} from '@tabler/icons-react';
import { ReactNode } from 'react';
import { Link, useLocation } from 'react-router-dom';
import { useAuth } from '../auth';
import type { Permission } from '../types';

export interface NavItem { path: string; label: string; icon: ReactNode; permission?: Permission }

export const navItems: NavItem[] = [
  { path: '/', label: 'Dashboard', icon: <IconHome size={18} /> },
  { path: '/patients', label: 'Patients', icon: <IconUsers size={18} />, permission: 'searchPatient' },
  { path: '/doctors', label: 'Doctors', icon: <IconStethoscope size={18} />, permission: 'listDoctors' },
  { path: '/appointments', label: 'Appointments', icon: <IconCalendarEvent size={18} />, permission: 'bookAppointment' },
  { path: '/schedule', label: 'Doctor schedules', icon: <IconCalendarTime size={18} />, permission: 'viewSchedule' },
  { path: '/consultations', label: 'Consultations', icon: <IconClipboardHeart size={18} />, permission: 'recordConsultation' },
  { path: '/reports', label: 'Reports', icon: <IconChartBar size={18} />, permission: 'dailyReport' },
  { path: '/users', label: 'User accounts', icon: <IconUserCog size={18} />, permission: 'manageUsers' },
  { path: '/audit', label: 'Audit log', icon: <IconListDetails size={18} />, permission: 'viewAuditLog' },
  { path: '/backups', label: 'Backup and restore', icon: <IconDatabaseExport size={18} />, permission: 'backup' },
  { path: '/password', label: 'Change my password', icon: <IconKey size={18} />, permission: 'changePassword' },
];

export function Layout({ children }: { children: ReactNode }) {
  const { user, logout, can } = useAuth();
  const [opened, { toggle, close }] = useDisclosure();
  const { colorScheme, toggleColorScheme } = useMantineColorScheme();
  const location = useLocation();
  const items = navItems.filter((i) => !i.permission || can(i.permission));

  return (
    <AppShell header={{ height: 60 }} navbar={{ width: 250, breakpoint: 'sm', collapsed: { mobile: !opened } }} padding="lg">
      <AppShell.Header>
        <Group h="100%" px="md" justify="space-between">
          <Group gap="sm">
            <Burger opened={opened} onClick={toggle} hiddenFrom="sm" size="sm" />
            <ThemeIcon size="lg" radius="md" variant="gradient" gradient={{ from: 'teal', to: 'cyan' }}>
              <IconStethoscope size={20} />
            </ThemeIcon>
            <Text fw={700} size="lg">HMS</Text>
            <Text c="dimmed" size="sm" visibleFrom="md">Hospital Management System</Text>
          </Group>
          <Group gap="sm">
            <Text size="sm" fw={500} visibleFrom="xs">{user?.username}</Text>
            <Badge variant="light">{user?.role}</Badge>
            <ActionIcon variant="default" size="lg" onClick={toggleColorScheme} aria-label="Light or dark">
              {colorScheme === 'dark' ? <IconSun size={18} /> : <IconMoon size={18} />}
            </ActionIcon>
            <Button variant="default" leftSection={<IconLogout size={16} />} onClick={() => logout()}>Log out</Button>
          </Group>
        </Group>
      </AppShell.Header>
      <AppShell.Navbar p="sm">
        <ScrollArea>
          {items.map((i) => (
            <NavLink key={i.path} component={Link} to={i.path} label={i.label} leftSection={i.icon}
              active={i.path === '/' ? location.pathname === '/' : location.pathname.startsWith(i.path)} onClick={close} />
          ))}
        </ScrollArea>
      </AppShell.Navbar>
      <AppShell.Main>{children}</AppShell.Main>
    </AppShell>
  );
}
