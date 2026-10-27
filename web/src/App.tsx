// Pages and who may open them. A page a role may not use simply does not exist for that role (HMS-F-003);
// the server still checks every request (HMS-SEC-002).
import { Center, Loader } from '@mantine/core';
import { ReactElement } from 'react';
import { Navigate, Route, Routes } from 'react-router-dom';
import { useAuth } from './auth';
import { Layout } from './components/Layout';
import type { Permission } from './types';
import AppointmentsPage from './pages/AppointmentsPage';
import AuditPage from './pages/AuditPage';
import BackupPage from './pages/BackupPage';
import ConsultationsPage from './pages/ConsultationsPage';
import DashboardPage from './pages/DashboardPage';
import DoctorsPage from './pages/DoctorsPage';
import LoginPage from './pages/LoginPage';
import PasswordPage from './pages/PasswordPage';
import PatientsPage from './pages/PatientsPage';
import ReportsPage from './pages/ReportsPage';
import SchedulePage from './pages/SchedulePage';
import UsersPage from './pages/UsersPage';

const pages: { path: string; element: ReactElement; permission?: Permission }[] = [
  { path: '/', element: <DashboardPage /> },
  { path: '/patients', element: <PatientsPage />, permission: 'searchPatient' },
  { path: '/doctors', element: <DoctorsPage />, permission: 'listDoctors' },
  { path: '/appointments', element: <AppointmentsPage />, permission: 'bookAppointment' },
  { path: '/schedule', element: <SchedulePage />, permission: 'viewSchedule' },
  { path: '/consultations', element: <ConsultationsPage />, permission: 'recordConsultation' },
  { path: '/reports', element: <ReportsPage />, permission: 'dailyReport' },
  { path: '/users', element: <UsersPage />, permission: 'manageUsers' },
  { path: '/audit', element: <AuditPage />, permission: 'viewAuditLog' },
  { path: '/backups', element: <BackupPage />, permission: 'backup' },
  { path: '/password', element: <PasswordPage />, permission: 'changePassword' },
];

export default function App() {
  const { user, ready, can } = useAuth();
  if (!ready) return <Center h="100vh"><Loader /></Center>;
  if (!user) return <LoginPage />;                      // HMS-F-001: nothing is reachable before login
  return (
    <Layout>
      <Routes>
        {pages.filter((p) => !p.permission || can(p.permission)).map((p) => <Route key={p.path} path={p.path} element={p.element} />)}
        <Route path="*" element={<Navigate to="/" replace />} />
      </Routes>
    </Layout>
  );
}
