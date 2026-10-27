// SPRINT 2 PLACEHOLDER: Saurav's pull request (HMS-SEC-007) replaces this panel this week.
import { Alert } from '@mantine/core';

export function RecoveryPanel({ file }: { file: string; latestBackup: string; onDone: () => void }) {
  return <Alert color="red" title="E040: Data file damaged">The file {file || 'a data file'} is damaged. Restoring from the website arrives this week.</Alert>;
}
