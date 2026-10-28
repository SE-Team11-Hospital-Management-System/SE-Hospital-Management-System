// Administrator only: create a backup, restore one after confirmation (HMS-F-061). The audit log is never rolled back.
import { Button, Card, Group, Modal, Stack, Table, Text } from '@mantine/core';
import { IconDatabasePlus } from '@tabler/icons-react';
import { useEffect, useState } from 'react';
import { api } from '../api';
import { EmptyState, notifyError, notifyOk, PageHeader } from '../components/ui';

const pretty = (n: string) => `${n.slice(0, 4)}-${n.slice(4, 6)}-${n.slice(6, 8)} ${n.slice(9, 11)}:${n.slice(11, 13)}:${n.slice(13, 15)}`;

export default function BackupPage() {
  const [list, setList] = useState<string[]>([]);
  const [restoring, setRestoring] = useState<string | null>(null);
  const load = () => api.get<string[]>('/api/backups').then(setList).catch(notifyError);
  useEffect(() => { load(); }, []);
  const create = async () => { try { const r = await api.post<{ name: string }>('/api/backups'); notifyOk(`Backup ${r.name} created.`); load(); } catch (e) { notifyError(e); } };
  const restore = async () => {
    try { await api.post(`/api/backups/${restoring}/restore`, { confirm: true }); notifyOk(`Backup ${restoring} restored. All screens now show the restored data.`); } catch (e) { notifyError(e); }
    setRestoring(null);
  };
  return (
    <>
      <PageHeader title="Backup and restore" subtitle="Backups are kept in the backup folder, one folder per backup"
        actions={<Button leftSection={<IconDatabasePlus size={16} />} onClick={create}>Create backup now</Button>} />
      <Card withBorder>
        {list.length === 0 ? <EmptyState>No backups yet.</EmptyState> : (
          <Table striped>
            <Table.Thead><Table.Tr><Table.Th>Backup</Table.Th><Table.Th>Taken at</Table.Th><Table.Th /></Table.Tr></Table.Thead>
            <Table.Tbody>
              {list.map((n) => (
                <Table.Tr key={n}><Table.Td>{n}</Table.Td><Table.Td>{pretty(n)}</Table.Td>
                  <Table.Td><Group justify="flex-end"><Button size="xs" variant="light" color="red" onClick={() => setRestoring(n)}>Restore</Button></Group></Table.Td></Table.Tr>
              ))}
            </Table.Tbody>
          </Table>
        )}
      </Card>
      <Modal opened={!!restoring} onClose={() => setRestoring(null)} title="Restore backup">
        <Stack>
          <Text>All current data will be replaced by backup <b>{restoring}</b>. The audit log is kept as it is. Are you sure?</Text>
          <Group justify="flex-end"><Button variant="default" onClick={() => setRestoring(null)}>No</Button><Button color="red" onClick={restore}>Yes, restore</Button></Group>
        </Stack>
      </Modal>
    </>
  );
}
